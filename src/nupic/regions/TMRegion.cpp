/* ---------------------------------------------------------------------
 * Numenta Platform for Intelligent Computing (NuPIC)
 * Copyright (C) 2013-2018, Numenta, Inc.  Unless you have an agreement
 * with Numenta, Inc., for a separate license for this software code, the
 * following terms and conditions apply:
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Affero Public License for more details.
 *
 * You should have received a copy of the GNU Affero Public License
 * along with this program.  If not, see http://www.gnu.org/licenses.
 *
 * http://numenta.org/licenses/
 *
 * Author: David Keeney, June 2018
 * ---------------------------------------------------------------------
 */
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#include <nupic/engine/Input.hpp>
#include <nupic/engine/Output.hpp>
#include <nupic/engine/Region.hpp>
#include <nupic/engine/RegionImpl.hpp>
#include <nupic/engine/Spec.hpp>
#include <nupic/ntypes/Array.hpp>
#include <nupic/ntypes/ArrayBase.hpp>
#include <nupic/ntypes/ArrayRef.hpp>
#include <nupic/ntypes/Buffer.hpp> // ReadBuffer,WriteBuffer
#include <nupic/ntypes/BundleIO.hpp>
#include <nupic/ntypes/ObjectModel.hpp> // IWrite/ReadBuffer
#include <nupic/ntypes/Value.hpp>
#include <nupic/regions/TMRegion.hpp>
#include <nupic/utils/Log.hpp>

namespace nupic {

  TMRegion::TMRegion(const ValueMap &params, Region *region)
      : RegionImpl(region), computeCallback_(nullptr) {
  // Note: the ValueMap gets destroyed on return so we need to get all of the parameters
  //       out of the map and set aside so we can pass them to the SpatialPooler
  //       algorithm when we create it during initialization().
  args_.numberOfCols    = params.getScalarT<UInt32>("columnCount", 0); // can use either name.
  args_.numberOfCols    = params.getScalarT<UInt32>("numberOfCols", 0);
  args_.cellsPerColumn  = params.getScalarT<UInt32>("cellsPerColumn", 10);
  args_.initialPerm     = params.getScalarT<Real32>("initialPerm", 0.11);
  args_.connectedPerm   = params.getScalarT<Real32>("connectedPerm", 0.50);
  args_.minThreshold    = params.getScalarT<UInt32>("minThreshold", 8);
  args_.newSynapseCount = params.getScalarT<UInt32>("newSynapseCount", 15);
  args_.permanenceInc   = params.getScalarT<Real32>("permanenceInc", 0.10);
  args_.permanenceDec   = params.getScalarT<Real32>("permanenceDec", 0.10);
  args_.permanenceMax   = params.getScalarT<UInt32>("permanenceMax", 1);
  args_.globalDecay     = params.getScalarT<Real32>("globalDecay", 0.10);
  args_.activationThreshold = params.getScalarT<UInt32>("activationThreshold", 12);
  args_.doPooling       = params.getScalarT<bool>("doPooling", false);
  args_.segUpdateValidDuration = params.getScalarT<UInt32>("segUpdateValidDuration", 5);
  args_.burnIn          = params.getScalarT<UInt32>("burnIn", 2);
  args_.collectStats    = params.getScalarT<bool>("collectStats", false);
  args_.seed            = params.getScalarT<Int32>("seed", 42);
  args_.verbosity       = params.getScalarT<bool>("verbosity", 0);
  args_.checkSynapseConsistency = params.getScalarT<bool>("checkSynapseConsistency", false);
  args_.pamLength       = params.getScalarT<UInt32>("pamLength", 1);
  args_.maxInfBacktrack = params.getScalarT<UInt32>("maxInfBacktrack", 10);
  args_.maxLrnBacktrack = params.getScalarT<UInt32>("maxLrnBacktrack", 5);
  args_.maxAge          = params.getScalarT<UInt32>("maxAge", 100000);
  args_.maxSeqLength    = params.getScalarT<UInt32>("maxSeqLength", 32);
  args_.maxSegmentsPerCell = params.getScalarT<Int32>("maxSegmentsPerCell", -1);
  args_.maxSegmentsPerSegment = params.getScalarT<Int32>("maxSegmentsPerSegment", -1);
  args_.outputType = params.getString("outputType", "normal");

  // variables used by this class and not passed on 
  args_.cellsSavePath = params.getString("cellsSavePath", "");
  args_.logPathOutput = params.getString("logPathOutput", "");
  args_.temporalImp   = params.getString("temporalImp", "");
  args_.learningMode  = params.getScalarT<bool>("learningMode", true);
  args_.inferenceMode = params.getScalarT<bool>("inferenceMode", false); 
  args_.anomalyMode   = params.getScalarT<bool>("anomalyMode", false); 
  args_.topDownMode   = params.getScalarT<bool>("topDownMode", false);
  args_.storeDenseOutput = params.getScalarT<bool>("storeDenseOutput", false);
  args_.computePredictedActiveCellIndices =
      params.getScalarT<bool>("computePredictedActiveCellIndices", false); 

  tm_ = nullptr;
  }

TMRegion::TMRegion(BundleIO &bundle, Region *region) : RegionImpl(region) {
  tm_ = nullptr;
  deserialize(bundle);
}

TMRegion::~TMRegion() {
  if (tm_)
    delete tm_;
}

void TMRegion::initialize() {
  // Before calling region.initialize() the dimensions for a region should have
  // been set. They may have been set manually --- by app calling
  // setDimensions() for a region, or they may have been set automatically due
  // to known dimentions on the other end of a link. See Network::initialize()
  // and Input::evaluateLinks() for details.
  UInt32 columnCount = (UInt32)region_->getOutputData("bottomUpOut").getCount();
  if (columnCount == 0) {
    NTA_THROW << "SPRegion::initialize - Output buffer size is not set.\n";
  }
  if (columnCount == 1) // marked as 'don't care'
  {
    columnCount = args_.columnCount;
  }
  spatialPoolerOutput_.allocateBuffer(columnCount);
  spatialPoolerOutput_.zeroBuffer();

  // All input links should have been initialized during Network.initialize().
  // However, if nothing is connected it might not be. The SpatialPooler
  // algorithm requires input.
  //
  // If there are more than on input link, the input buffer will be the concatination
  // of all incomming buffers.
  UInt32 inputWidth = (UInt32)region_->getInputData("bottomUpIn").getCount();
  if (inputWidth == 0) {
    NTA_THROW << "SPRegion::initialize - No input was provided.\n";
  }

  // Note: if inputWidth is provided in the parameters then the SpatialPooler
  // will only look at that many bits regardless as to how many output buffers
  // are concatinated to form the input buffer.
  if (args_.inputWidth && args_.inputWidth != inputWidth) {
    if (inputWidth > args_.inputWidth)
      inputWidth = args_.inputWidth;  // truncate the buffer to configured width.
  }
  // start with an empty buffer until we start running.
  spatialPoolerInput_.allocateBuffer(inputWidth);
  spatialPoolerInput_.zeroBuffer();

  tm_ = new BacktrackingTMCCP(
      args_.numberOfCols, args_.cellsPerColumn, args_.initialPerm,
      args_.connectedPerm, args_.minThreshold, args_.newSynapseCount,
      args_.permanenceInc, args_.permanenceDec, args_.permanenceMax,
      args_.globalDecay, args_.activationThreshold, args_.doPooling,
      args_.segUpdateValidDuration, args_.burnIn, args_.collectStats,
      args_.seed, args_.verbosity, args_.checkSynapseConsistency,
      args_.pamLength, args_.maxInfBacktrack, args_.maxLrnBacktrack,
      args_.maxAge, args_.maxSeqLength, args_.maxSegmentsPerCell,
      args_.maxSegmentsPerSegment, args_.outputType);
}

void TMRegion::compute() {
  // Note: the Python code has a hook at this point to activate profiling with
  // hotshot.
  //       This version does not provide this hook although there are several
  //       C++ profilers that could be used.

  NTA_ASSERT(sp_) << "SP not initialized";

  if (topDownMode_) {
    // TOP-DOWN inference mode
    NTA_THROW << "Top Down Inference mode is not implemented.";
  } else {
    // BOTTOM-UP compute mode
    iter_++;

    // Note: The Input and Output objects are Real32 types.
    //       The SpatialPooler algorithm expects UInt32 (containing 1's and 0's)
    //       So we must perform a copy with a type conversion.
    // Prepare the input
    spatialPoolerInput_ = getInput("bottomUpIn")->getData().as(NTA_BasicType_UInt32);
    inputValid_ = true;
    nzInputValid_ = false;


    /**************************************************  Not used.
    // check for reset
    bool resetSignal = false;
    Input* resetIn = region_->getInput("resetIn");
    if (resetIn)
    {
      // if there is a resetIn array and its first element is not 0 then the
    resetSignal is true. const Array& resetArray = resetIn->getData();
      NTA_CHECK(resetArray.getCount() == 1);
      resetSignal = (*((Real32*)resetArray.getBuffer()) != 0.0);
      

    }
    ***************************************************/

    // Call SpatialPooler
    UInt32 *inputVector = (UInt32 *)spatialPoolerInput_.getBuffer();
    UInt32 *outputVector = (UInt32 *)spatialPoolerOutput_.getBuffer();
    sp_->compute(inputVector, learningMode_, outputVector);

    // Prepare the output
    getOutput("bottomUpOut")->getData().convertInto(spatialPoolerOutput_);
    outputValid_ = true;
    nzOutputValid_ = false;

    size_t size = sp_->getNumColumns();
    // Direct logging of SP non-zero outputs if requested
    if (!logPathOutput_.empty()) {
      FILE *fp = fopen(logPathOutput_.c_str(), "a");
      if (fp) {
        for (int i = 0; i < size; i++) {
          if (outputVector[i])
            fprintf(fp, "%d ", i);
        }
        fprintf(fp, "\n");
        fclose(fp);
      }
    }

    // Direct logging of SP non-zero inputs if requested
    if (!logPathInput_.empty()) {
      FILE *fp = fopen(logPathInput_.c_str(), "a");
      if (fp) {
        for (int i = 0; i < size; i++) {
          if (outputVector[i])
            fprintf(fp, "%d ", i);
        }
        fprintf(fp, "\n");
        fclose(fp);
      }
    }
  }
}

std::string TMRegion::executeCommand(const std::vector<std::string> &args,
                                     Int64 index) {
  // The TM does not execute any Commands.
  return "";
}

// This is the per-node output size. This determines how big the output buffers
// should be allocated to during Region::initialization(). NOTE: Some outputs
// are optional, return 0 if not used.
size_t TMRegion::getNodeOutputElementCount(const std::string &outputName) {
  if (outputName ==
      "bottomUpOut") // This is the only output link we actually use.
  {
    const Array &out = getOutput("bottomUpOut")->getData();
    if (out.getCount())
      return out.getCount();
    else
      return args_.columnCount; // in case it was specified in the args.
  }
  return 0; // an optional output that we don't use.
}

Spec *TMRegion::createSpec() {
  auto ns = new Spec;

  ns->description =
    "TMRegion. Class implementing the temporal memory algorithm as described in "
	  "'BAMI <https://numenta.com/biological-and-machine-intelligence/>'.  The "
	  "implementation here attempts to closely match the pseudocode in the "
	  "documentation. This implementation does contain several additional bells and "
	  "whistles such as a column confidence measure.";

  ns->singleNodeOnly = true; // this means we don't care about dimensions;

  /* ---- parameters ------ */

  /* constructor arguments */
  ns->parameters.add(
	  "numberOfCols",
	  ParameterSpec("(int) Number of mini-columns in the region. This values "
           "needs to be the same as the number of columns in the SP, if one is "
           "used.",
	  NTA_BasicType_UInt32, // type
	  1,                    // elementCount
	  "",                   // constraints
	  "0",                   // defaultValue
    ParameterSpec::CreateAccess)); // access

 ns->parameters.add(
	  "cellsPerColumn",
	  ParameterSpec("(int) The number of cells per mini-column.",
	  NTA_BasicType_UInt32, // type
	  1,                    // elementCount
	  "",                   // constraints
	  "10",                   // defaultValue
    ParameterSpec::CreateAccess)); // access

 ns->parameters.add(
	  "initialPerm",
	  ParameterSpec("(float) Initial permanence for newly created synapses.",
	  NTA_BasicType_Real32, // type
	  1,                    // elementCount
	  "",                   // constraints
	  "0.11",                // defaultValue
    ParameterSpec::ReadWriteAccess)); // access

 ns->parameters.add(
	  "connectedPerm",
	  ParameterSpec("(float) ",
	  NTA_BasicType_Real32, // type
	  1,                    // elementCount
	  "",                   // constraints
	  "0.5",                // defaultValue
    ParameterSpec::ReadWriteAccess)); // access

 ns->parameters.add(
	  "minThreshold",
	  ParameterSpec(" (int)  Minimum number of active synapses for a segment to "
         "be considered during search for the best-matching segments. ",
	  NTA_BasicType_UInt32, // type
	  1,                    // elementCount
	  "",                   // constraints
	  "8",                  // defaultValue
    ParameterSpec::ReadWriteAccess)); // access

 ns->parameters.add(
	  "newSynapseCount",
	  ParameterSpec("(int) The max number of synapses added to a segment during learning.",
	  NTA_BasicType_UInt32, // type
	  1,                    // elementCount
	  "",                   // constraints
	  "15",                  // defaultValue
    ParameterSpec::ReadWriteAccess)); // access

 ns->parameters.add(
	  "permanenceInc",   // permInc in Cells4
	  ParameterSpec("(float) Active synapses get their permanence counts "
         "incremented by this value.",
	  NTA_BasicType_Real32, // type
	  1,                    // elementCount
	  "",                   // constraints
	  "0.1",                  // defaultValue
    ParameterSpec::ReadWriteAccess)); // access

 ns->parameters.add(
	  "permanenceDec",   // permDec in Cells4
	  ParameterSpec("(float) All other synapses get their permanence counts "
         "decremented by this value.",
	  NTA_BasicType_Real32, // type
	  1,                    // elementCount
	  "",                   // constraints
	  "0.1",                  // defaultValue
    ParameterSpec::ReadWriteAccess)); // access

 ns->parameters.add(
	  "permanenceMax",   // permMax in Cells4
	  ParameterSpec("(float) ",
	  NTA_BasicType_Real32, // type
	  1,                    // elementCount
	  "",                   // constraints
	  "1",                  // defaultValue
    ParameterSpec::ReadWriteAccess)); // access

 ns->parameters.add(
	  "globalDecay",         
	  ParameterSpec("(float) Value to decrease permanences when the global "
         "decay process runs. Global decay will remove synapses if their "
         "permanence value reaches 0. It will also remove segments when they no "
         "longer have synapses. \n"
         "Note:: Global decay is applied after 'maxAge' iterations, after "
         "which it will run every `'maxAge' iterations.",
	  NTA_BasicType_Real32, // type
	  1,                    // elementCount
	  "",                   // constraints
	  "0.10",                // defaultValue
    ParameterSpec::ReadWriteAccess)); // access

 ns->parameters.add(
	  "activationThreshold",         
	  ParameterSpec("(int) Number of synapses that must be active to "
         "activate a segment.",
	  NTA_BasicType_UInt32, // type
	  1,                    // elementCount
	  "",                   // constraints
	  "12",                  // defaultValue
    ParameterSpec::ReadWriteAccess)); // access

 ns->parameters.add(
	  "doPooling",         
	  ParameterSpec("(bool) If True, pooling is enabled. False is the default.",
	  NTA_BasicType_Bool,   // type
	  1,                    // elementCount
	  "bool",               // constraints
	  "false",              // defaultValue
    ParameterSpec::ReadWriteAccess)); // access

 ns->parameters.add(
	  "segUpdateValidDuration",         
	  ParameterSpec("(int) ",
	  NTA_BasicType_UInt32, // type
	  1,                    // elementCount
	  "",                   // constraints
	  "5",                  // defaultValue
    ParameterSpec::ReadWriteAccess)); // access

 ns->parameters.add(
	  "burnIn",      // not in Cells4     
	  ParameterSpec("(int) Used for evaluating the prediction score. Default is 2. ",
	  NTA_BasicType_UInt32, // type
	  1,                    // elementCount
	  "",                   // constraints
	  "2",                  // defaultValue
    ParameterSpec::ReadWriteAccess)); // access

 ns->parameters.add(
	  "collectStats",      // not in Cells4     
	  ParameterSpec("(bool) If True, collect training / inference stats.  Default is False. ",
	  NTA_BasicType_Bool,   // type
	  1,                    // elementCount
	  "bool",               // constraints
	  "false",              // defaultValue
    ParameterSpec::ReadWriteAccess)); // access

 ns->parameters.add(
	  "seed",       
	  ParameterSpec("(int)  Random number generator seed. The seed affects the random "
         "aspects of initialization like the initial permanence values. A fixed "
         "value ensures a reproducible result.",
	  NTA_BasicType_Int32,  // type
	  1,                    // elementCount
	  "",                   // constraints
	  "42",                 // defaultValue
    ParameterSpec::ReadWriteAccess)); // access

 ns->parameters.add(
	  "verbosity",      // not in Cells4     
	  ParameterSpec("(int) Controls the verbosity of the TM diagnostic output: \n"
         "- verbosity == 0: silent \n"
         "- verbosity in [1..6]: increasing levels of verbosity.",
	  NTA_BasicType_UInt32, // type
	  1,                    // elementCount
	  "",                   // constraints
	  "0",                  // defaultValue
    ParameterSpec::ReadWriteAccess)); // access

  ns->parameters.add("checkSynapseConsistency", // not in Cells4
    ParameterSpec("(bool)   Default is False. ",
                  NTA_BasicType_Bool, // type
                  1,                  // elementCount
                  "bool",             // constraints
                  "false",            // defaultValue
                  ParameterSpec::ReadWriteAccess)); // access


 ns->parameters.add(
	  "pamLength",      // not in Cells4     
	  ParameterSpec("(int) Number of time steps to remain in \"Pay Attention Mode\" "
         "after we detect we've reached the end of a learned sequence. Setting "
         "this to 0 disables PAM mode. When we are in PAM mode, we do not burst "
         "unpredicted columns during learning, which in turn prevents us from "
         "falling into a previously learned sequence for a while (until we run "
         "through another 'pamLength' steps). \n"
         "\n"
         "The advantage of PAM mode is that it requires fewer presentations to "
         "learn a set of sequences which share elements. The disadvantage of PAM "
         "mode is that if a learned sequence is immediately followed by set set "
         "of elements that should be learned as a 2nd sequence, the first "
         "'pamLength' elements of that sequence will not be learned as part of "
         "that 2nd sequence.",
	  NTA_BasicType_UInt32, // type
	  1,                    // elementCount
	  "",                   // constraints
	  "1",                  // defaultValue
    ParameterSpec::ReadWriteAccess)); // access

 ns->parameters.add(
	  "maxInfBacktrack",      // not in Cells4     
	  ParameterSpec("(int) How many previous inputs to keep in a buffer for "
         "inference backtracking.",
	  NTA_BasicType_UInt32, // type
	  1,                    // elementCount
	  "",                   // constraints
	  "10",                  // defaultValue
    ParameterSpec::ReadWriteAccess)); // access

 ns->parameters.add(
	  "maxLrnBacktrack",      // not in Cells4     
	  ParameterSpec("(int) How many previous inputs to keep in a buffer for "
         "learning backtracking.",
	  NTA_BasicType_UInt32, // type
	  1,                    // elementCount
	  "",                   // constraints
	  "5",                  // defaultValue
    ParameterSpec::ReadWriteAccess)); // access

 ns->parameters.add(
     "maxAge", // not in Cells4
     ParameterSpec(
         "(int) Number of iterations before global decay takes effect. "
         "Also the global decay execution interval. After global decay starts, "
         "it "
         "will will run again every 'maxAge' iterations. If 'maxAge==1', "
         "global decay is applied to every iteration to every segment. \n"
         "Note: Using 'maxAge > 1' can significantly speed up the TM when "
         "global decay is used. Default=100000.",
         NTA_BasicType_UInt32,             // type
         1,                                // elementCount
         "",                               // constraints
         "100000",                         // defaultValue
         ParameterSpec::ReadWriteAccess)); // access

 ns->parameters.add(
	  "maxSeqLength",      // not in Cells4     
	  ParameterSpec("(int) If not 0, we will never learn more than "
         "'maxSeqLength' inputs in a row without starting over at start cells. "
         "This sets an upper bound on the length of learned sequences and thus is "
         "another means (besides `'maxAge' and 'globalDecay') by which to "
         "limit how much the TM tries to learn. ",
	  NTA_BasicType_UInt32, // type
	  1,                    // elementCount
	  "",                   // constraints
	  "32",                  // defaultValue
    ParameterSpec::ReadWriteAccess)); // access

 ns->parameters.add(
	  "maxSegmentsPerCell",      // not in Cells4     
	  ParameterSpec("(int) The maximum number of segments allowed on a "
         "cell. This is used to turn on 'fixed size CLA' mode. When in effect, "
         "'globalDecay' is not applicable and must be set to 0 and 'maxAge' "
         "must be set to 0. When this is used (> 0), 'maxSynapsesPerSegment' "
         "must also be > 0. ",
	  NTA_BasicType_Int32, // type
	  1,                    // elementCount
	  "",                   // constraints
	  "-1",                  // defaultValue
    ParameterSpec::ReadWriteAccess)); // access

 ns->parameters.add(
	  "maxSynapsesPerSegment",      // not in Cells4     
	  ParameterSpec("(int) The maximum number of synapses allowed in "
         "a segment. This is used to turn on 'fixed size CLA' mode. When in "
         "effect, 'globalDecay' is not applicable and must be set to 0, and "
         "'maxAge' must be set to 0. When this is used (> 0), "
         "'maxSegmentsPerCell' must also be > 0.",
	  NTA_BasicType_Int32, // type
	  1,                    // elementCount
	  "",                   // constraints
	  "-1",                  // defaultValue
    ParameterSpec::ReadWriteAccess)); // access

 ns->parameters.add(
    "outputType",    // not in Cells4
    ParameterSpec(
        "(string) Can be one of the following (default 'normal'):\n"
        " - 'normal': output the OR of the active and predicted state. \n"
        " - 'activeState': output only the active state. \n"
        " - 'activeState1CellPerCol': output only the active state, and at most "
        "1 cell/column. If more than 1 cell is active in a column, the one "
        " with the highest confidence is sent up.  ",
        NTA_BasicType_Byte,               // type
        0,                                // elementCount
        "",                               // constraints
        "normal",                         // defaultValue
        ParameterSpec::ReadWriteAccess)); // access



///////// Parameters not part of the calling arguments //////////

 ns->parameters.add(
	  "predictedSegmentDecrement",  
	  ParameterSpec("(float) Predicted segment decrement",
	  NTA_BasicType_Real32, // type
	  1,                    // elementCount
	  "",                   // constraints
	  "",                   // defaultValue
    ParameterSpec::ReadOnlyAccess)); // access

 ns->parameters.add(
	  "orColumnOutputs",    
	  ParameterSpec("(bool) OR together the cell outputs from each column to produce "
      "the temporal memory output. When this mode is enabled, the number of "
      "cells per column must also be specified and the output size of the region "
      "should be set the same as columnCount",
	  NTA_BasicType_Bool,   // type
	  1,                    // elementCount
	  "bool",               // constraints
	  "false",              // defaultValue
    ParameterSpec::ReadOnlyAccess)); // access

 ns->parameters.add(
    "cellsSavePath",   
    ParameterSpec(
        "(string) Optional path to file in which large temporal memory cells "
                    "data structure is to be saved. ",
        NTA_BasicType_Byte,               // type
        0,                                // elementCount
        "",                               // constraints
        "",                               // defaultValue
        ParameterSpec::ReadWriteAccess)); // access

 ns->parameters.add(
    "temporalImp",   
    ParameterSpec(
        "(string) Which temporal memory implementation to use. ",
        NTA_BasicType_Byte,               // type
        0,                                // elementCount
        "",                               // constraints
        "",                               // defaultValue
        ParameterSpec::ReadWriteAccess)); // access


  /* The last group is for parameters that aren't specific to spatial pooler */
  ns->parameters.add(
     "learningMode",
     ParameterSpec("1 if the node is learning (default true).",
      NTA_BasicType_Bool, // type
      1,                    // elementCount
      "bool",               // constraints
      "true",                  // defaultValue
      ParameterSpec::CreateAccess)); // access

  ns->parameters.add(
      "inferenceMode",
      ParameterSpec("True if the node is inferring (default false).  obsolete.",
        NTA_BasicType_Bool,               // type
        1,                                // elementCount
        "bool",                           // constraints
        "false",                          // defaultValue
        ParameterSpec::CreateAccess)); // access

  ns->parameters.add(
      "anomalyMode",
      ParameterSpec("True if an anomaly score is being computed. obsolete.",
        NTA_BasicType_Bool,               // type
        1,                                // elementCount
        "bool",                           // constraints
        "false",                          // defaultValue
        ParameterSpec::CreateAccess)); // access

  ns->parameters.add(
      "topDownMode",
      ParameterSpec("True if the node should do top down compute on the next call "
                    "to compute into topDownOut (default false).",
        NTA_BasicType_Bool,             // type
        1,                                // elementCount
        "bool",                           // constraints
        "false",                         // defaultValue
        ParameterSpec::CreateAccess)); // access

  ns->parameters.add(
      "computePredictedActiveCellIndices",
      ParameterSpec("True if active and predicted active indices should be computed (default false).",
        NTA_BasicType_Bool,             // type
        1,                                // elementCount
        "bool",                           // constraints
        "false",                         // defaultValue
        ParameterSpec::CreateAccess)); // access

 ns->parameters.add(
	  "activeOutputCount",         
	  ParameterSpec("(int)Number of active elements in bottomUpOut output.",
	  NTA_BasicType_UInt32, // type
	  1,                    // elementCount
	  "",                   // constraints
	  "0",                  // defaultValue
    ParameterSpec::ReadOnlyAccess)); // access

 ns->parameters.add(
	  "storeDenseOutput",         
	  ParameterSpec("(bool) Whether to keep the dense column output (needed for "
                     "denseOutput parameter).",
	  NTA_BasicType_Bool,       // type
	  1,                        // elementCount
	  "bool",                   // constraints
	  "false",                  // defaultValue
    ParameterSpec::ReadWriteAccess)); // access

 ns->parameters.add(
    "logPathOutput",   
    ParameterSpec("(string) Optional name of output log file. If set, every output vector "
                  " will be logged to this file as a sparse vector. ",
        NTA_BasicType_Byte,               // type
        0,                                // elementCount
        "",                               // constraints
        "",                               // defaultValue
        ParameterSpec::ReadWriteAccess)); // access


///////////// Inputs and Outputs ////////////////
   /* ----- inputs ------- */
  ns->inputs.add(
    "bottomUpIn",
    InputSpec("The input signal, conceptually organized as an image pyramid "
      "data structure, but internally organized as a flattened vector.", 
              NTA_BasicType_Real32, // type
              0,                    // count.
              true,                 // required?
              false,                // isRegionLevel,
              true,                 // isDefaultInput
              false                 // requireSplitterMap
              ));

  ns->inputs.add(
      "resetIn",
      InputSpec("A boolean flag that indicates whether "
                "or not the input vector received in this compute cycle "
                "represents the first training presentation in new temporal sequence.",
                NTA_BasicType_Real32, // type
                1,                    // count.
                false,                // required?
                true,                 // isRegionLevel,
                false,                // isDefaultInput
                false                 // requireSplitterMap
                ));

  ns->inputs.add(
    "sequenceIdIn", 
    InputSpec("Sequence ID",
              NTA_BasicType_UInt64, // type
              1,                    // count.
              false,                // required?
              true,  // isRegionLevel,
              false, // isDefaultInput
              false  // requireSplitterMap
              ));

  /* ----- outputs ------ */
  ns->outputs.add(
      "bottomUpOut",
      OutputSpec("The output signal generated from the bottom-up inputs "
                 "from lower levels.",
                 NTA_BasicType_Real32, // type
                 0,                    // count 0 means is dynamic
                 true,                 // isRegionLevel
                 true                  // isDefaultOutput
                 ));

  ns->outputs.add(
    "topDownOut",
      OutputSpec("The top-down output signal, generated from "
                  "feedback from upper levels.  ",
                  NTA_BasicType_Real32, // type
                  0,                    // count 0 means is dynamic
                  true,                 // isRegionLevel
                  false                 // isDefaultOutput
                  ));

  ns->outputs.add(
    "activeCells",
    OutputSpec("The cells that are active",
                  NTA_BasicType_Real32, // type
                  0,                    // count 0 means is dynamic
                  true,                 // isRegionLevel
                  false                 // isDefaultOutput
                  ));

  ns->outputs.add(
    "predictedActiveCells",
    OutputSpec("The cells that are active and predicted",
                  NTA_BasicType_Real32, // type
                  0,                    // count 0 means is dynamic
                  true,                 // isRegionLevel
                  false                 // isDefaultOutput
                  ));

  ns->outputs.add(
    "anomalyScore",
    OutputSpec("The score for how 'anomalous' (i.e. rare) the current "
               "sequence is. Higher values are increasingly rare.",
                  NTA_BasicType_Real32, // type
                  1,                    // count 0 means is dynamic
                  true,                 // isRegionLevel
                  false                 // isDefaultOutput
                  ));

  ns->outputs.add(
    "lrnActiveStateT",
    OutputSpec("Active cells during learn phase at time t.  This is "
               "used for anomaly classification.",
                  NTA_BasicType_Real32, // type
                  0,                    // count 0 means is dynamic
                  true,                 // isRegionLevel
                  false                 // isDefaultOutput
                  ));


  /* ----- commands ------ */
  // commands TBD

  return ns;
}

////////////////////////////////////////////////////////////////////////
//           Parameters
//
// Most parameters are handled automatically by getParameterFromBuffer().
// The ones that need special treatment are explicitly handled here.
//
////////////////////////////////////////////////////////////////////////

UInt32 TMRegion::getParameterUInt32(const std::string &name, Int64 index) {
  switch (name[0]) {
  case 'a':
    if (name == "activeOutputCount") {
      return (UInt32)spatialPoolerOutput_.getCount();
    }
    if (name == "anomalyMode") {
      return anomalyMode_;
    }
    break;
  case 'c':
    if (name == "columnCount") {
      if (sp_)
        return sp_->getNumColumns();
      else
        return (Int32)args_.columnCount;
    }
    break;
  case 'd':
    if (name == "dutyCyclePeriod") {
      if (sp_)
        return sp_->getDutyCyclePeriod();
      else
        return args_.dutyCyclePeriod;
    }
    break;
  case 'i':
    if (name == "inputWidth") {
      if (sp_)
        return sp_->getNumInputs();
      else
        return (Int32)args_.inputWidth;
    }
    if (name == "inferenceMode") {
      return inferenceMode_;
    }
    break;
  case 'l':
    if (name == "learningMode") {
      return learningMode_;
    }
    break;
  case 'n':
    if (name == "numActiveColumnsPerInhArea") {
      if (sp_)
        return sp_->getNumActiveColumnsPerInhArea();
      else
        return args_.numActiveColumnsPerInhArea;
    }
    break;
  case 'p':
    if (name == "potentialRadius") {
      if (sp_)
        return sp_->getPotentialRadius();
      return args_.potentialRadius;
    }
    break;
  case 's':
    if (name == "stimulusThreshold") {
      if (sp_)
        return sp_->getStimulusThreshold();
      else
        return args_.stimulusThreshold;
    }
    if (name == "spVerbosity") {
      if (sp_)
        return sp_->getSpVerbosity();
      else
        return args_.spVerbosity;
    }
    break;
  case 't':
    if (name == "topDownMode") {
      return topDownMode_;
    }
    break;
  }                                                         // end switch
  return this->RegionImpl::getParameterUInt32(name, index); // default
}

Int32 SPRegion::getParameterInt32(const std::string &name, Int64 index) {
  if (name == "seed") {
    return args_.seed;
  }
  return this->RegionImpl::getParameterInt32(name, index); // default
}

Real32 SPRegion::getParameterReal32(const std::string &name, Int64 index) {
  switch (name[0]) {
  case 'b':
    if (name == "boostStrength") {
      if (sp_)
        return sp_->getBoostStrength();
      else
        return args_.boostStrength;
    }
    break;
  case 'l':
    if (name == "localAreaDensity") {
      if (sp_)
        return sp_->getLocalAreaDensity();
      else
        return args_.localAreaDensity;
    }
    break;
  case 'm':
    if (name == "minPctOverlapDutyCycles") {
      if (sp_)
        return sp_->getMinPctOverlapDutyCycles();
      else
        return args_.minPctOverlapDutyCycles;
    }
    break;
  case 'p':
    if (name == "potentialPct") {
      if (sp_)
        return sp_->getPotentialPct();
      else
        return args_.potentialPct;
    }
    break;
  case 's':
    if (name == "synPermInactiveDec") {
      if (sp_)
        return sp_->getSynPermInactiveDec();
      else
        return args_.synPermInactiveDec;
    }
    if (name == "synPermActiveInc") {
      if (sp_)
        return sp_->getSynPermActiveInc();
      else
        return args_.synPermActiveInc;
    }
    if (name == "synPermConnected") {
      if (sp_)
        return sp_->getSynPermConnected();
      else
        return args_.synPermConnected;
    }
    break;
  }
  return this->RegionImpl::getParameterReal32(name, index); // default
}

bool SPRegion::getParameterBool(const std::string &name, Int64 index) {
  if (name == "globalInhibition") {
    if (sp_)
      return sp_->getGlobalInhibition();
    else
      return args_.globalInhibition;
  }
  if (name == "wrapAround") {
    if (sp_)
      return sp_->getWrapAround();
    else
      return args_.wrapAround;
  }
  return this->RegionImpl::getParameterBool(name, index); // default
}

void TMRegion::getParameterArray(const std::string &name, Int64 index,
                                 Array &array) {
  if (name == "spatialPoolerInput") {
    if (!inputValid_) {
      const Array &incoming = getInput("bottomUpIn")->getData();
      Real32 *in1 = (Real32 *)incoming.getBuffer();
      UInt32 *in2 = (UInt32 *)spatialPoolerInput_.getBuffer();
      for (size_t i = 0; i < incoming.getCount(); i++) {
        *in2++ = (UInt32)floor(*in1++); // convert from Real32 to UInt32
      }
      inputValid_ = true;
    }
    array = spatialPoolerInput_;
  } else if (name == "spatialPoolerOutput") {
    array = spatialPoolerOutput_;
  } else if (name == "spInputNonZeros") {
    if (!nzInputValid_) {
      const Array &incoming = getInput("bottomUpIn")->getData();
      nzInput_ = makeNonZeroArray(incoming);
      nzInputValid_ = true;
    }
    array = nzInput_;
  } else if (name == "spOutputNonZeros") {
    if (!nzOutputValid_) {
      const Array &output = getOutput("bottomUpOut")->getData();
      nzOutput_ = makeNonZeroArray(output);
      nzOutputValid_ = true;
    }
    array = nzOutput_;
  }
  //  spOverlapDistribution not found
  //  sparseCoincidenceMatrix not found
  //  denseOutput not found
  else {
    this->RegionImpl::getParameterArray(name, index, array);
  }
}

size_t TMRegion::getParameterArrayCount(const std::string &name, Int64 index) {
  if (name == "spatialPoolerInput") {
    return getInput("bottomUpIn")->getData().getCount();
  } else if (name == "spatialPoolerOutput") {
    return spatialPoolerOutput_.getCount();
  } else if (name == "spInputNonZeros") {
    if (!nzInputValid_) {
      Array a;
      getParameterArray(name, index, a);  // This forces nzInput_ to be valid.
    }
    return nzInput_.getCount();
  } else if (name == "spOutputNonZeros") {
    if (!nzOutputValid_) {
      Array a;
      getParameterArray(name, index, a);  // This forces nzOutput_ to be valid.
    }
    return nzOutput_.getCount();
  }
  return 0;
}

std::string TMRegion::getParameterString(const std::string &name, Int64 index) {
  if (name == "logPathInput") {
    return logPathInput_;
  }
  if (name == "logPathOutput") {
    return logPathOutput_;
  }
  if (name == "logPathOutputDense") {
    return logPathOutputDense_;
  }
  if (name == "spatialImp") {
    return args_.spatialImp;
  }
  // "spLearningStatsStr"  not found
  return this->RegionImpl::getParameterString(name, index);
}

void TMRegion::setParameterUInt32(const std::string &name, Int64 index,
                                  UInt32 value) {
  switch (name[0]) {
  case 'a':
    if (name == "anomalyMode") {
      anomalyMode_ = (value != 0);
      return;
    }
    break;
  case 'd':
    if (name == "dutyCyclePeriod") {
      if (sp_)
        sp_->setDutyCyclePeriod(value);
      args_.dutyCyclePeriod = value;
      return;
    }
    break;
  case 'i':
    if (name == "inferenceMode") {
      inferenceMode_ = (value != 0);
      return;
    }
    break;
  case 'l':
    if (name == "learningMode") {
      learningMode_ = (value != 0);
      return;
    }
    break;
  case 'n':
    if (name == "numActiveColumnsPerInhArea") {
      if (sp_)
        sp_->setNumActiveColumnsPerInhArea(value);
      args_.numActiveColumnsPerInhArea = value;
      return;
    }
    break;
  case 'p':
    if (name == "potentialRadius") {
      if (sp_)
        sp_->setPotentialRadius(value);
      args_.potentialRadius = value;
      return;
    }
    break;
  case 's':
    if (name == "stimulusThreshold") {
      if (sp_)
        sp_->setStimulusThreshold(value);
      args_.stimulusThreshold = value;
      return;
    }
    if (name == "spVerbosity") {
      if (sp_)
        sp_->setSpVerbosity(value);
      args_.spVerbosity = value;
      return;
    }
    break;
  case 't':
    if (name == "topDownMode") {
      topDownMode_ = (value != 0);
      return;
    }
    break;

    RegionImpl::setParameterUInt32(name, index, value);
  } // switch
}

void TMRegion::setParameterInt32(const std::string &name, Int64 index,
                                 Int32 value) {
  RegionImpl::setParameterInt32(name, index, value);
}

void TMRegion::setParameterReal32(const std::string &name, Int64 index,
                                  Real32 value) {
  switch (name[0]) {
  case 'b':
    if (name == "boostStrength") {
      if (sp_)
        sp_->setBoostStrength(value);
      args_.boostStrength = value;
      return;
    }
    break;
  case 'l':
    if (name == "localAreaDensity") {
      if (sp_)
        sp_->setLocalAreaDensity(value);
      args_.localAreaDensity = value;
      return;
    }
    break;
  case 'm':
    if (name == "minPctOverlapDutyCycles") {
      if (sp_)
        sp_->setMinPctOverlapDutyCycles(value);
      args_.minPctOverlapDutyCycles = value;
      return;
    }
    break;
  case 'p':
    if (name == "potentialPct") {
      if (sp_)
        sp_->setPotentialPct(value);
      args_.potentialPct = value;
      return;
    }
    break;

  case 's':
    if (name == "synPermInactiveDec") {
      if (sp_)
        sp_->setSynPermInactiveDec(value);
      args_.synPermInactiveDec = value;
      return;
    }
    if (name == "synPermActiveInc") {
      if (sp_)
        sp_->setSynPermActiveInc(value);
      args_.synPermActiveInc = value;
      return;
    }
    if (name == "synPermConnected") {
      if (sp_)
        sp_->setSynPermConnected(value);
      args_.synPermConnected = value;
      return;
    }
    break;
  } // switch
  RegionImpl::setParameterReal32(name, index, value);
}

void TMRegion::setParameterBool(const std::string &name, Int64 index,
                                bool value) {
  if (name == "globalInhibition") {
    if (sp_)
      sp_->setGlobalInhibition(value);
    args_.globalInhibition = value;
    return;
  }
  if (name == "wrapAround") {
    if (sp_)
      sp_->setWrapAround(value);
    args_.wrapAround = value;
    return;
  }

  RegionImpl::setParameterBool(name, index, value);
}

void TMRegion::setParameterString(const std::string &name, Int64 index,
                                  const std::string &s) {
  if (name == "logPathInput") {
    logPathInput_ = s;
  } else if (name == "logPathOutput") {
    logPathOutput_ = s;
  } else if (name == "logPathOutputDense") {
    logPathOutputDense_ = s;
  } else
    this->RegionImpl::setParameterString(name, index, s);
}

void TMRegion::getParameterFromBuffer(const std::string &name, Int64 index,
                                      IWriteBuffer &value) {
  if (name == "inputDimensions") {
    if (sp_) {
      for (auto elem : sp_->getInputDimensions()) {
        value.write(elem);
      }
    } else
      value.write(0);
  } else if (name == "columnDimensions") {
    if (sp_) {
      for (auto &elem : sp_->getColumnDimensions()) {
        value.write(elem);
      }
    } else
      value.write(0);
  } else {
    NTA_THROW << "SPRegion::getParameterFromBuffer() -- unknown name " << name;
  }
}

void TMRegion::setParameterFromBuffer(const std::string &name, Int64 index,
                                      IReadBuffer &value) {
  NTA_THROW << "SPRegion::setParameter -- Unknown parameter " << name;
}

void TMRegion::serialize(BundleIO &bundle) {
  std::ofstream &f = bundle.getOutputStream("SPRegion");
  // There is more than one way to do this. We could serialize to YAML, which
  // would make a readable format, or we could serialize directly to the stream
  // Choose the easier one.
  bool init = ((sp_) ? true : false);
  f << "SPRegion" << std::endl
    << std::setprecision(std::numeric_limits<Real32>::max_digits10)
    << args_.inputWidth << " " << args_.columnCount << " "
    << args_.potentialRadius << " " << args_.potentialPct << " "
    << args_.globalInhibition << " " << args_.localAreaDensity << " "
    << args_.numActiveColumnsPerInhArea << " " << args_.stimulusThreshold << " "
    << args_.synPermInactiveDec << " " << args_.synPermActiveInc << " "
    << args_.synPermConnected << " " << args_.minPctOverlapDutyCycles << " "
    << args_.dutyCyclePeriod << " " << args_.boostStrength << " " << args_.seed
    << " " << args_.spVerbosity << " " << args_.wrapAround << " "
    << learningMode_ << " " << inferenceMode_ << " " << anomalyMode_ << " "
    << topDownMode_ << " " << init << " " << iter_ << " \"" << args_.spatialImp
    << "\" " << std::endl;
  f << (nupic::ArrayBase &)spatialPoolerOutput_ << std::endl;
  if (sp_)
    sp_->save(f);
  f.close();
}

void TMRegion::deserialize(BundleIO &bundle) {
  std::ifstream &f = bundle.getInputStream("SPRegion");
  // There is more than one way to do this. We could serialize to YAML, which
  // would make a readable format, or we could serialize directly to the stream
  // Choose the easier one.
  bool init;
  std::string signatureString;
  f >> signatureString;
  if (signatureString != "SPRegion") {
    NTA_THROW << "Bad serialization for region '" << region_->getName()
              << "' of type SPRegion. Main serialization file must start "
              << "with \"SPRegion\" but instead it starts with '"
              << signatureString << "'";
  }
  f >> args_.inputWidth;
  f >> args_.columnCount;
  f >> args_.potentialRadius;
  f >> args_.potentialPct;
  f >> args_.globalInhibition;
  f >> args_.localAreaDensity;
  f >> args_.numActiveColumnsPerInhArea;
  f >> args_.stimulusThreshold;
  f >> args_.synPermInactiveDec;
  f >> args_.synPermActiveInc;
  f >> args_.synPermConnected;
  f >> args_.minPctOverlapDutyCycles;
  f >> args_.dutyCyclePeriod;
  f >> args_.boostStrength;
  f >> args_.seed;
  f >> args_.spVerbosity;
  f >> args_.wrapAround;
  f >> learningMode_;
  f >> inferenceMode_;
  f >> anomalyMode_;
  f >> topDownMode_;
  f >> init;
  f >> iter_;
  f >> args_.spatialImp;
  f >> spatialPoolerOutput_;

  if (args_.spatialImp[0] == '"')
    args_.spatialImp =
        args_.spatialImp.substr(1, args_.spatialImp.length() - 2);

  if (init) {
    sp_ = new SpatialPooler();
    sp_->load(f);
  } else
    sp_ = nullptr;
  f.close();

  // Note:  The Arrays spatialPoolerInput_ and spatialPoolerOutput_
  //        will be re-instantiated on next call to compute().
}

} // namespace nupic
