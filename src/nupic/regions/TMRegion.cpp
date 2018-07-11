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
#include <iomanip> // setprecision() in stream
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#include <nupic/algorithms/Anomaly.hpp>
#include <nupic/algorithms/BacktrackingTMCpp.hpp>
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
  // Note: the ValueMap gets destroyed on return so we need to get all of the
  // parameters
  //       out of the map and set aside so we can pass them to the SpatialPooler
  //       algorithm when we create it during initialization().
  args_.numberOfCols = params.getScalarT<UInt32>("numberOfCols", 0);
  args_.cellsPerColumn = params.getScalarT<UInt32>("cellsPerColumn", 10);
  args_.initialPerm = params.getScalarT<Real32>("initialPerm", 0.11f);
  args_.connectedPerm = params.getScalarT<Real32>("connectedPerm", 0.50f);
  args_.minThreshold = params.getScalarT<UInt32>("minThreshold", 8);
  args_.newSynapseCount = params.getScalarT<UInt32>("newSynapseCount", 15);
  args_.permanenceInc = params.getScalarT<Real32>("permanenceInc", 0.10f);
  args_.permanenceDec = params.getScalarT<Real32>("permanenceDec", 0.10f);
  args_.permanenceMax = params.getScalarT<Real32>("permanenceMax", 1.0f);
  args_.globalDecay = params.getScalarT<Real32>("globalDecay", 0.10f);
  args_.activationThreshold = params.getScalarT<UInt32>("activationThreshold", 12);
  args_.doPooling = params.getScalarT<bool>("doPooling", false);
  args_.segUpdateValidDuration = params.getScalarT<UInt32>("segUpdateValidDuration", 5);
  args_.burnIn = params.getScalarT<UInt32>("burnIn", 2);
  args_.collectStats = params.getScalarT<bool>("collectStats", false);
  args_.seed = params.getScalarT<Int32>("seed", 42);
  args_.verbosity = params.getScalarT<Int32>("verbosity", 0);
  args_.checkSynapseConsistency = params.getScalarT<bool>("checkSynapseConsistency", false);
  args_.pamLength = params.getScalarT<UInt32>("pamLength", 1);
  args_.maxInfBacktrack = params.getScalarT<UInt32>("maxInfBacktrack", 10);
  args_.maxLrnBacktrack = params.getScalarT<UInt32>("maxLrnBacktrack", 5);
  args_.maxAge = params.getScalarT<UInt32>("maxAge", 100000);
  args_.maxSeqLength = params.getScalarT<UInt32>("maxSeqLength", 32);
  args_.maxSegmentsPerCell = params.getScalarT<Int32>("maxSegmentsPerCell", -1);
  args_.maxSegmentsPerSegment = params.getScalarT<Int32>("maxSegmentsPerSegment", -1);
  memset((void *)args_.outputType, 0, sizeof(args_.outputType));
  strcpy_s(args_.outputType, sizeof(args_.outputType),params.getString("outputType", "normal").c_str());

  // variables used by this class and not passed on
  args_.cellsSavePath = params.getString("cellsSavePath", "");
  args_.logPathOutput = params.getString("logPathOutput", "");
  args_.temporalImp = params.getString("temporalImp", "");
  args_.learningMode = params.getScalarT<bool>("learningMode", true);
  args_.inferenceMode = params.getScalarT<bool>("inferenceMode", false);
  args_.anomalyMode = params.getScalarT<bool>("anomalyMode", false);
  args_.topDownMode = params.getScalarT<bool>("topDownMode", false);
  args_.storeDenseOutput = params.getScalarT<bool>("storeDenseOutput", false);
  args_.computePredictedActiveCellIndices = params.getScalarT<bool>("computePredictedActiveCellIndices", false);
  args_.orColumnOutputs = params.getScalarT<bool>("orColumnOutputs", false);

  args_.outputWidth = args_.numberOfCols * args_.cellsPerColumn;

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

  // All input links and buffers should have been initialized during
  // Network.initialize().
  //
  // If there are more than on input link, the input buffer will be the
  // concatination of all incomming buffers.
  UInt32 inputWidth = (UInt32)region_->getInputData("bottomUpIn").getCount();
  if (inputWidth == 0) {
    NTA_THROW << "TMRegion::initialize - No input was provided.\n";
  }

  tm_ = new nupic::algorithms::backtracking_tm::BacktrackingTMCpp(
                  args_.numberOfCols, 
                  args_.cellsPerColumn, 
                  args_.initialPerm,
                  args_.connectedPerm, 
                  args_.minThreshold, 
                  args_.newSynapseCount,
                  args_.permanenceInc, 
                  args_.permanenceDec, 
                  args_.permanenceMax,
                  args_.globalDecay, 
                  args_.activationThreshold, 
                  args_.doPooling,
                  args_.segUpdateValidDuration, 
                  args_.burnIn, 
                  args_.collectStats,
                  args_.seed, 
                  args_.verbosity, 
                  args_.checkSynapseConsistency,
                  args_.pamLength,
                  args_.maxInfBacktrack, 
                  args_.maxLrnBacktrack,
                  args_.maxAge, 
                  args_.maxSeqLength, 
                  args_.maxSegmentsPerCell,
                  args_.maxSegmentsPerSegment, 
                  args_.outputType);
  iter_ = 0;
  sequencePos_ = 0;

  // Setup the output buffer
  Array tpOutput(NTA_BasicType_Real32);
  tpOutput.allocateBuffer(args_.numberOfCols * args_.cellsPerColumn);
  tm_->setOutputBuffer((Real32*)tpOutput.getBuffer());
}

void TMRegion::compute() {
  // Note: the Python code has a hook at this point to activate profiling with
  // hotshot.
  //       This version does not provide this hook although there are several
  //       C++ profilers that could be used.

  NTA_ASSERT(tm_) << "TM not initialized";
  iter_++;

  // Handle reset signal
  Array &reset = getInput("resetIn")->getData();
  if (reset.getCount() == 1 && ((Real32 *)(reset.getBuffer()))[0] != 0) {
    tm_->reset();
    sequencePos_ = 0; // Position within the current sequence
  }

  if (args_.computePredictedActiveCellIndices) {
    prevPredictedState_ = tm_->getPredictedState(); // returns a vector<UInt32>
  }
  if (args_.anomalyMode) {
    Array p(NTA_BasicType_Real32, tm_->topDownCompute(), args_.numberOfCols);
    prevPredictedColumns_ = p.nonZero().asVector();
  }

  // Perform inference and / or learning
  Array &bottomUpIn = getInput("bottomUpIn")->getData();
  Array &tmOutput = getOutput("bottomUpOut")->getData();


  // Perform Bottom up compute()
  Real *output = tm_->compute((Real32 *)bottomUpIn.getBuffer(),
                              args_.learningMode, args_.inferenceMode);
  sequencePos_++;

  if (args_.orColumnOutputs) {
    // OR'ing together the output cells in each column?
    // This reduces the dimentions to [columnCount] otherwise
    // The size is [columnCount X cellsPerColumn].
    Real *ptr = (Real32 *)tmOutput.getBuffer();
    for (size_t i = 0; i < args_.numberOfCols; i++) {
      for (size_t j = 0; j < args_.cellsPerColumn; j++) {
        ptr[i] = std::max(output[i], output[(i * args_.cellsPerColumn) + j]);
      }
    }
    tmOutput.setCount(args_.numberOfCols);
  } else {
    // copy tm buffer to bottomUpOut buffer.  (and type conversion)
    Real *ptr = (Real32 *)tmOutput.getBuffer();
    for (size_t i = 0; i < args_.numberOfCols * args_.cellsPerColumn; i++) {
      ptr[i] = output[i];
    }
  }

  // Direct logging of TM non-zero outputs if requested
  if (!args_.logPathOutput.empty()) {
    Real32 *ptr = (Real32 *)tmOutput.getBuffer();
    Size size = tmOutput.getCount();
    FILE *fp = fopen(args_.logPathOutput.c_str(), "a");
    if (fp) {
      for (int i = 0; i < size; i++) {
        if (ptr[i])
          fprintf(fp, "%d ", i);
      }
      fprintf(fp, "\n");
      fclose(fp);
    }
  }

  if (args_.topDownMode) {
    // Top - down compute
    Real *tdout = tm_->topDownCompute();
    getOutput("topDownOut")->getData().copyFrom(NTA_BasicType_Real32, tdout, args_.numberOfCols);
  }

  // Set output for use with anomaly classification region if in anomalyMode
  if (args_.anomalyMode) {
    Byte* lrn = tm_->getLearnActiveStateT();
    Size size = args_.numberOfCols * args_.cellsPerColumn;
    getOutput("lrnActiveStateT")->getData().copyFrom(NTA_BasicType_Byte, lrn, size);

    std::vector<UInt32> activeColumns = bottomUpIn.nonZero().asVector();
    Real32 anomalyScore = algorithms::anomaly::computeRawAnomalyScore(
        activeColumns, prevPredictedColumns_);
    getOutput("anomalyScore")->getData().copyFrom(NTA_BasicType_Real32, &anomalyScore, 1);
  }

  if (args_.computePredictedActiveCellIndices) {
    Output *activeCells = getOutput("activeCells");
    Output *predictedActiveCells = getOutput("predictedActiveCells");
    Byte* activeState = tm_->getActiveState();
    Size nCells = args_.numberOfCols * args_.cellsPerColumn;
    NTA_ASSERT(activeCells != nullptr);
    NTA_ASSERT(predictedActiveCells != nullptr);
    NTA_ASSERT(args_.outputWidth == nCells);
    NTA_ASSERT(args_.outputWidth == activeCells->getData().getCount());
    NTA_ASSERT(args_.outputWidth == predictedActiveCells->getData().getCount());

    Real32 *activeCellsPtr = (Real32 *)activeCells->getData().getBuffer();
    Real32 *predictedActiveCellsPtr =(Real32 *)predictedActiveCells->getData().getBuffer();
    for (size_t idx = 0; idx < nCells; idx++) {
      activeCellsPtr[idx] = (activeState[idx]) ? 1.0f : 0.0f;
      predictedActiveCellsPtr[idx] = (prevPredictedState_[idx] && activeState[idx]) ? 1.0f : 0.0f;
    }
  }
}

std::string TMRegion::executeCommand(const std::vector<std::string> &args,Int64 index) 
{
  // The TM does not execute any Commands.
  return "";
}

// This is the per-node output size. This determines how big the output
// buffers should be allocated to during Region::initialization(). NOTE: Some
// outputs are optional, return 0 if not used.
size_t TMRegion::getNodeOutputElementCount(const std::string &outputName) {
  if (outputName == "bottomUpOut")
    return args_.outputWidth;
  if (outputName == "topDownOut")
    return args_.numberOfCols;
  if (outputName == "lrnActiveStateT")
    return args_.outputWidth;
  if (outputName == "activeCells")
    return args_.outputWidth;
  if (outputName == "predictedActiveCells")
    return args_.outputWidth;
  return 0; // an optional output that we don't use.
}

Spec *TMRegion::createSpec() {
  auto ns = new Spec;

  ns->description =
      "TMRegion. Class implementing the temporal memory algorithm as "
      "described in "
      "'BAMI <https://numenta.com/biological-and-machine-intelligence/>'.  "
      "The implementation here attempts to closely match the pseudocode in the "
      "documentation. This implementation does contain several additional "
      "bells and whistles such as a column confidence measure.";

  ns->singleNodeOnly = true; // this means we don't care about dimensions;

  /* ---- parameters ------ */

  /* constructor arguments */
  ns->parameters.add(
      "numberOfCols",
      ParameterSpec("(int) Number of mini-columns in the region. This values "
                    "needs to be the same as the number of columns in the "
                    "SP, if one is "
                    "used.",
                    NTA_BasicType_UInt32,          // type
                    1,                             // elementCount
                    "",                            // constraints
                    "0",                           // defaultValue
                    ParameterSpec::CreateAccess)); // access

  ns->parameters.add(
      "cellsPerColumn",
      ParameterSpec("(int) The number of cells per mini-column.",
                    NTA_BasicType_UInt32, // type
                    1,                    // elementCount
                    "",                   // constraints
                    "10",                 // defaultValue
                    ParameterSpec::CreateAccess)); // access

  ns->parameters.add(
      "initialPerm",
      ParameterSpec("(float) Initial permanence for newly created synapses.",
                    NTA_BasicType_Real32,             // type
                    1,                                // elementCount
                    "",                               // constraints
                    "0.11",                           // defaultValue
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
      ParameterSpec(
                    " (int)  Minimum number of active synapses for a segment to "
                    "be considered during search for the best-matching segments. ",
                    NTA_BasicType_UInt32,             // type
                    1,                                // elementCount
                    "",                               // constraints
                    "8",                              // defaultValue
                    ParameterSpec::ReadWriteAccess)); // access

  ns->parameters.add("newSynapseCount",
      ParameterSpec("(int) The max number of synapses added "
                    "to a segment during learning.",
                    NTA_BasicType_UInt32, // type
                    1,                    // elementCount
                    "",                   // constraints
                    "15",                 // defaultValue
                    ParameterSpec::ReadWriteAccess)); // access

  ns->parameters.add(
      "permanenceInc", // permInc in Cells4
      ParameterSpec("(float) Active synapses get their permanence counts "
                    "incremented by this value.",
                    NTA_BasicType_Real32,             // type
                    1,                                // elementCount
                    "",                               // constraints
                    "0.1",                            // defaultValue
                    ParameterSpec::ReadWriteAccess)); // access

  ns->parameters.add(
      "permanenceDec", // permDec in Cells4
      ParameterSpec("(float) All other synapses get their permanence counts "
                    "decremented by this value.",
                    NTA_BasicType_Real32,             // type
                    1,                                // elementCount
                    "",                               // constraints
                    "0.1",                            // defaultValue
                    ParameterSpec::ReadWriteAccess)); // access

  ns->parameters.add(
      "permanenceMax", // permMax in Cells4
      ParameterSpec("(float) ",
                    NTA_BasicType_Real32, // type
                    1,                    // elementCount
                    "",                   // constraints
                    "1",                  // defaultValue
                    ParameterSpec::ReadWriteAccess)); // access

  ns->parameters.add(
      "globalDecay",
      ParameterSpec(
                    "(float) Value to decrease permanences when the global "
                    "decay process runs. Global decay will remove synapses if their "
                    "permanence value reaches 0. It will also remove segments when "
                    "they no "
                    "longer have synapses. \n"
                    "Note:: Global decay is applied after 'maxAge' iterations, after "
                    "which it will run every `'maxAge' iterations.",
                    NTA_BasicType_Real32,             // type
                    1,                                // elementCount
                    "",                               // constraints
                    "0.10",                           // defaultValue
                    ParameterSpec::ReadWriteAccess)); // access

  ns->parameters.add(
      "activationThreshold",
      ParameterSpec("(int) Number of synapses that must be active to "
                    "activate a segment.",
                    NTA_BasicType_UInt32,             // type
                    1,                                // elementCount
                    "",                               // constraints
                    "12",                             // defaultValue
                    ParameterSpec::ReadWriteAccess)); // access

  ns->parameters.add(
      "doPooling",
      ParameterSpec("(bool) If True, pooling is enabled. False is the default.",
                    NTA_BasicType_Bool,               // type
                    1,                                // elementCount
                    "bool",                           // constraints
                    "false",                          // defaultValue
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
      "burnIn", // not in Cells4
      ParameterSpec(
                    "(int) Used for evaluating the prediction score. Default is 2. ",
                    NTA_BasicType_UInt32,             // type
                    1,                                // elementCount
                    "",                               // constraints
                    "2",                              // defaultValue
                    ParameterSpec::ReadWriteAccess)); // access

  ns->parameters.add(
      "collectStats", // not in Cells4
      ParameterSpec("(bool) If True, collect training / "
                    "inference stats.  Default is False. ",
                    NTA_BasicType_Bool, // type
                    1,                  // elementCount
                    "bool",             // constraints
                    "false",            // defaultValue
                    ParameterSpec::ReadWriteAccess)); // access

  ns->parameters.add(
      "seed",
      ParameterSpec(
                  "(int)  Random number generator seed. The seed affects the random "
                  "aspects of initialization like the initial permanence values. A "
                  "fixed "
                  "value ensures a reproducible result.",
                  NTA_BasicType_Int32,              // type
                  1,                                // elementCount
                  "",                               // constraints
                  "42",                             // defaultValue
                  ParameterSpec::ReadWriteAccess)); // access

  ns->parameters.add(
      "verbosity", // not in Cells4
      ParameterSpec(
                  "(int) Controls the verbosity of the TM diagnostic output: \n"
                  "- verbosity == 0: silent \n"
                  "- verbosity in [1..6]: increasing levels of verbosity.",
                  NTA_BasicType_UInt32,             // type
                  1,                                // elementCount
                  "",                               // constraints
                  "0",                              // defaultValue
                  ParameterSpec::ReadWriteAccess)); // access

  ns->parameters.add(
      "checkSynapseConsistency", // not in Cells4
      ParameterSpec("(bool)   Default is False. ",
                  NTA_BasicType_Bool, // type
                  1,                  // elementCount
                  "bool",             // constraints
                  "false",            // defaultValue
                  ParameterSpec::ReadWriteAccess)); // access

  ns->parameters.add(
      "pamLength", // not in Cells4
      ParameterSpec(
                "(int) Number of time steps to remain in \"Pay Attention Mode\" "
                "after we detect we've reached the end of a learned sequence. "
                "Setting this to 0 disables PAM mode. When we are in PAM mode, we do not "
                "burst unpredicted columns during learning, which in turn prevents us "
                "from falling into a previously learned sequence for a while (until we "
                "run through another 'pamLength' steps). \n"
                "\n"
                "The advantage of PAM mode is that it requires fewer presentations "
                "to learn a set of sequences which share elements. The disadvantage "
                "of PAM mode is that if a learned sequence is immediately followed by set "
                "set of elements that should be learned as a 2nd sequence, the first "
                "'pamLength' elements of that sequence will not be learned as part "
                "of that 2nd sequence.",
                NTA_BasicType_UInt32,             // type
                1,                                // elementCount
                "",                               // constraints
                "1",                              // defaultValue
                ParameterSpec::ReadWriteAccess)); // access

  ns->parameters.add(
      "maxInfBacktrack", // not in Cells4
      ParameterSpec("(int) How many previous inputs to keep in a buffer for "
                "inference backtracking.",
                NTA_BasicType_UInt32,             // type
                1,                                // elementCount
                "",                               // constraints
                "10",                             // defaultValue
                ParameterSpec::ReadWriteAccess)); // access

  ns->parameters.add(
      "maxLrnBacktrack", // not in Cells4
      ParameterSpec("(int) How many previous inputs to keep in a buffer for "
                "learning backtracking.",
                NTA_BasicType_UInt32,             // type
                1,                                // elementCount
                "",                               // constraints
                "5",                              // defaultValue
                ParameterSpec::ReadWriteAccess)); // access

  ns->parameters.add(
      "maxAge", // not in Cells4
      ParameterSpec(
              "(int) Number of iterations before global decay takes effect. "
              "Also the global decay execution interval. After global decay "
              "starts, it will run again every 'maxAge' iterations. If 'maxAge==1', "
              "global decay is applied to every iteration to every segment. \n"
              "Note: Using 'maxAge > 1' can significantly speed up the TM when "
              "global decay is used. Default=100000.",
              NTA_BasicType_UInt32,             // type
              1,                                // elementCount
              "",                               // constraints
              "100000",                         // defaultValue
              ParameterSpec::ReadWriteAccess)); // access

  ns->parameters.add(
      "maxSeqLength", // not in Cells4
      ParameterSpec(
              "(int) If not 0, we will never learn more than "
              "'maxSeqLength' inputs in a row without starting over at start "
              "cells. This sets an upper bound on the length of learned sequences and "
              "thus is another means (besides `'maxAge' and 'globalDecay') by which to "
              "limit how much the TM tries to learn. ",
              NTA_BasicType_UInt32,             // type
              1,                                // elementCount
              "",                               // constraints
              "32",                             // defaultValue
              ParameterSpec::ReadWriteAccess)); // access

  ns->parameters.add(
      "maxSegmentsPerCell", // not in Cells4
      ParameterSpec(
            "(int) The maximum number of segments allowed on a "
            "cell. This is used to turn on 'fixed size CLA' mode. When in "
            "effect, 'globalDecay' is not applicable and must be set to 0 and 'maxAge' "
            "must be set to 0. When this is used (> 0), "
            "'maxSynapsesPerSegment' "
            "must also be > 0. ",
            NTA_BasicType_Int32,              // type
            1,                                // elementCount
            "",                               // constraints
            "-1",                             // defaultValue
            ParameterSpec::ReadWriteAccess)); // access

  ns->parameters.add(
      "maxSynapsesPerSegment", // not in Cells4
      ParameterSpec(
            "(int) The maximum number of synapses allowed in "
            "a segment. This is used to turn on 'fixed size CLA' mode. When in "
            "effect, 'globalDecay' is not applicable and must be set to 0, and "
            "'maxAge' must be set to 0. When this is used (> 0), "
            "'maxSegmentsPerCell' must also be > 0.",
            NTA_BasicType_Int32,              // type
            1,                                // elementCount
            "",                               // constraints
            "-1",                             // defaultValue
            ParameterSpec::ReadWriteAccess)); // access

  ns->parameters.add(
      "outputType", // not in Cells4
      ParameterSpec(
            "(string) Can be one of the following (default 'normal'):\n"
            " - 'normal': output the OR of the active and predicted state. \n"
            " - 'activeState': output only the active state. \n"
            " - 'activeState1CellPerCol': output only the active state, and at "
            "most "
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
      ParameterSpec(
            "(bool) OR together the cell outputs from each column to produce "
            "the temporal memory output. When this mode is enabled, the number "
            "of "
            "cells per column must also be specified and the output size of "
            "the region "
            "should be set the same as columnCount",
            NTA_BasicType_Bool,              // type
            1,                               // elementCount
            "bool",                          // constraints
            "false",                         // defaultValue
            ParameterSpec::ReadOnlyAccess)); // access

  ns->parameters.add(
      "cellsSavePath",
      ParameterSpec("(string) Optional path to file in which "
            "large temporal memory cells "
            "data structure is to be saved. ",
            NTA_BasicType_Byte, // type
            0,                  // elementCount
            "",                 // constraints
            "",                 // defaultValue
            ParameterSpec::ReadWriteAccess)); // access

  ns->parameters.add(
      "temporalImp",
      ParameterSpec("(string) Which temporal memory implementation to use. ",
            NTA_BasicType_Byte,               // type
            0,                                // elementCount
            "",                               // constraints
            "",                               // defaultValue
            ParameterSpec::ReadWriteAccess)); // access

  /* The last group is for parameters that aren't specific to spatial pooler
   */
  ns->parameters.add(
      "learningMode",
      ParameterSpec("1 if the node is learning (default true).",
            NTA_BasicType_Bool, // type
            1,                  // elementCount
            "bool",             // constraints
            "true",             // defaultValue
            ParameterSpec::CreateAccess)); // access

  ns->parameters.add(
      "inferenceMode",
      ParameterSpec("True if the node is inferring (default false).  obsolete.",
            NTA_BasicType_Bool,            // type
            1,                             // elementCount
            "bool",                        // constraints
            "false",                       // defaultValue
            ParameterSpec::CreateAccess)); // access

  ns->parameters.add(
      "anomalyMode",
      ParameterSpec("True if an anomaly score is being computed. obsolete.",
            NTA_BasicType_Bool,            // type
            1,                             // elementCount
            "bool",                        // constraints
            "false",                       // defaultValue
            ParameterSpec::CreateAccess)); // access

  ns->parameters.add(
      "topDownMode",
      ParameterSpec(
            "True if the node should do top down compute on the next call "
            "to compute into topDownOut (default false).",
            NTA_BasicType_Bool,            // type
            1,                             // elementCount
            "bool",                        // constraints
            "false",                       // defaultValue
            ParameterSpec::CreateAccess)); // access

  ns->parameters.add(
      "computePredictedActiveCellIndices",
      ParameterSpec("True if active and predicted active indices should be "
            "computed (default false).",
            NTA_BasicType_Bool,            // type
            1,                             // elementCount
            "bool",                        // constraints
            "false",                       // defaultValue
            ParameterSpec::CreateAccess)); // access

  ns->parameters.add(
      "activeOutputCount",
      ParameterSpec("(int)Number of active elements in bottomUpOut output.",
            NTA_BasicType_UInt32,            // type
            1,                               // elementCount
            "",                              // constraints
            "0",                             // defaultValue
            ParameterSpec::ReadOnlyAccess)); // access

  ns->parameters.add(
      "storeDenseOutput",
      ParameterSpec(
            "(bool) Whether to keep the dense column output (needed for "
            "denseOutput parameter).",
            NTA_BasicType_Bool,               // type
            1,                                // elementCount
            "bool",                           // constraints
            "false",                          // defaultValue
            ParameterSpec::ReadWriteAccess)); // access

  ns->parameters.add(
      "logPathOutput",
      ParameterSpec(
            "(string) Optional name of output log file. If set, "
              "every output vector will be logged to this file "
              "as a sparse vector. ",
              NTA_BasicType_Byte,               // type
              0,                                // elementCount
              "",                               // constraints
              "",                               // defaultValue
              ParameterSpec::ReadWriteAccess)); // access

  ///////////// Inputs and Outputs ////////////////
  /* ----- inputs ------- */
  ns->inputs.add(
      "bottomUpIn",
      InputSpec(
              "The input signal, conceptually organized as an image pyramid "
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
                "represents the first training presentation in new temporal "
                "sequence.",
                NTA_BasicType_Real32, // type
                1,                    // count.
                false,                // required?
                true,                 // isRegionLevel,
                false,                // isDefaultInput
                false                 // requireSplitterMap
                ));

  ns->inputs.add("sequenceIdIn", InputSpec("Sequence ID",
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
                  0,    // count 0 means is dynamic
                  true, // isRegionLevel
                  false // isDefaultOutput
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
// Most parameters are explicitly handled here until initialization.
// After initialization they are passed on to the tm_ for processing.
//
////////////////////////////////////////////////////////////////////////

UInt32 TMRegion::getParameterUInt32(const std::string &name, Int64 index) 
{

  if (tm_)
      return tm_->getParameterUInt32(name);

  switch (name[0]) {
  case 'a':
    if (name == "activationThreshold") {
      return args_.activationThreshold;
    }
    break;
  case 'b':
    if (name == "burnIn") {
      return args_.burnIn;
    }
    break;

  case 'c':
    if (name == "cellsPerColumn")
      return args_.cellsPerColumn;
    if (name == "numberOfCols")
      return args_.numberOfCols;
    break;

  case 'm':
    if (name == "maxAge")
      return args_.maxAge;
    if (name == "maxInfBacktrack")
      return args_.maxInfBacktrack;
    if (name == "maxLrnBacktrack")
      return args_.maxLrnBacktrack;
    if (name == "minThreshold")
      return args_.minThreshold;
    if (name == "maxSeqLength")
      return args_.maxSeqLength;
    break;

  case 'n':
    if (name == "newSynapseCount")
      return args_.newSynapseCount;
    break;

  case 'o':
    if (name == "outputWidth")
      return args_.outputWidth;

  case 'p':
    if (name == "pamLength")
      return args_.pamLength;
    break;

  case 's':
    if (name == "segUpdateValidDuration")
      return args_.segUpdateValidDuration;
    break;
  }                                                         // end switch
  return this->RegionImpl::getParameterUInt32(name, index); // default
}

Int32 TMRegion::getParameterInt32(const std::string &name, Int64 index) {
  if (tm_)
    return tm_->getParameterInt32(name);

  if (name == "maxSegmentsPerCell")
    return args_.maxSegmentsPerCell;
  if (name == "maxSegmentsPerSegment")
    return args_.maxSegmentsPerSegment;
  if (name == "seed") {
    return args_.seed;
    if (name == "verbosity")
      return args_.verbosity;
  }
  return this->RegionImpl::getParameterInt32(name, index); // default
}

Real32 TMRegion::getParameterReal32(const std::string &name, Int64 index) {
  if (tm_)
    return tm_->getParameterReal32(name);

  switch (name[0]) {
  case 'c':
    if (name == "initialPerm")
      return args_.initialPerm;
    break;
  case 'g':
    if (name == "globalDecay")
      return args_.globalDecay;
    break;

  case 'i':
    if (name == "initialPerm")
      return args_.initialPerm;
    break;
  case 'p':
    if (name == "permanenceInc")
      return args_.permanenceInc;
    if (name == "permanenceDec")
      return args_.permanenceDec;
    if (name == "connectedPerm")
      return args_.connectedPerm;
    if (name == "permanenceMax")
      return args_.permanenceMax;
    break;
  }
  return this->RegionImpl::getParameterReal32(name, index); // default
}

bool TMRegion::getParameterBool(const std::string &name, Int64 index) {
  if (name == "anomalyMode")
    return args_.anomalyMode;
  if (name == "collectStats") {
    if (tm_)
      return tm_->getParameterBool(name);
    return args_.collectStats;
  }
  if (name == "checkSynapseConsistency") {
    if (tm_)
      return tm_->getParameterBool(name);
    return args_.checkSynapseConsistency;
  }
  if (name == "computePredictedActiveCellIndices")
    return args_.computePredictedActiveCellIndices;
  if (name == "doPooling") {
    if (tm_)
      return tm_->getParameterBool(name);
    return args_.doPooling;
  }
  if (name == "learningMode")
    return args_.learningMode;
  if (name == "inferenceMode")
    return args_.inferenceMode;
  if (name == "orColumnOutputs")
    return args_.orColumnOutputs;
  if (name == "topDownMode")
    return args_.topDownMode;
  if (name == "storeDenseOutput")
    return args_.storeDenseOutput;

  return this->RegionImpl::getParameterBool(name, index); // default
}

void TMRegion::getParameterArray(const std::string &name, Int64 index,
                                 Array &array) {
  if (name == "") {
  } else {
    this->RegionImpl::getParameterArray(name, index, array);
  }
}

size_t TMRegion::getParameterArrayCount(const std::string &name, Int64 index) {
  if (name == "") {
    return 0;
  }
  return 0;
}

std::string TMRegion::getParameterString(const std::string &name, Int64 index) {
  if (name == "cellsSavePath") {
    return args_.cellsSavePath;
  }
  if (name == "logPathOutput") {
    return args_.logPathOutput;
  }
  if (name == "outputType") {
    if (tm_)
      return tm_->getParameterString(name);
    return args_.outputType;
  }
  if (name == "temporalImp") {
    return args_.temporalImp;
  }
  return this->RegionImpl::getParameterString(name, index);
}

void TMRegion::setParameterUInt32(const std::string &name, Int64 index,  UInt32 value) 
{
  switch (name[0]) {
  case 'a':
    if (name == "activationThreshold") {
      if (tm_) tm_->setParameter(name, value);
      args_.activationThreshold = value;
      return;
    }
    break;
  case 'b':
    if (name == "burnIn") {
      if (tm_)  tm_->setParameter(name, value);
      args_.burnIn = value;
      return;
    }
    break;
  case 'm':
    if (name == "minThreshold") {
      if (tm_)
        tm_->setParameter(name, value);
      args_.minThreshold = value;
      return;
    }
    if (name == "maxInfBacktrack") {
      if (tm_)
        tm_->setParameter(name, value);
      args_.maxInfBacktrack = value;
      return;
    }
    if (name == "maxLrnBacktrack") {
      if (tm_)
        tm_->setParameter(name, value);
      args_.maxLrnBacktrack = value;
      return;
    }
    if (name == "maxAge") {
      if (tm_)
        tm_->setParameter(name, value);
      args_.maxAge = value;
      return;
    }
    if (name == "maxSeqLength") {
      if (tm_)
        tm_->setParameter(name, value);
      args_.maxSeqLength = value;
      return;
    }
    break;
  case 'n':
    if (name == "newSynapseCount") {
      if (tm_)
        tm_->setParameter(name, value);
      args_.newSynapseCount = value;
      return;
    }
    break;
  case 'p':
    if (name == "pamLength") {
      if (tm_)
        tm_->setParameter(name, value);
      args_.pamLength = value;
      return;
    }
    break;
  case 's':
    if (name == "segUpdateValidDuration") {
      if (tm_)
        tm_->setParameter(name, value);
      args_.segUpdateValidDuration = value;
      return;
    }
    break;

    RegionImpl::setParameterUInt32(name, index, value);
  } // switch
}

void TMRegion::setParameterInt32(const std::string &name, Int64 index,
                                 Int32 value) {
  if (name == "maxSegmentsPerCell") {
    if (tm_)
      tm_->setParameter(name, value);
    args_.maxSegmentsPerCell = value;
    return;
  }
  if (name == "maxSegmentsPerSegment") {
    if (tm_)
      tm_->setParameter(name, value);
    args_.maxSegmentsPerSegment = value;
    return;
  }
  if (name == "verbosity") {
    if (tm_)
      tm_->setParameter(name, value);
    args_.verbosity = value;
    return;
  }
  RegionImpl::setParameterInt32(name, index, value);
}

void TMRegion::setParameterReal32(const std::string &name, Int64 index,
                                  Real32 value) {
  switch (name[0]) {
  case 'c':
    if (name == "connectedPerm") {
      if (tm_)
        tm_->setParameter(name, value);
      args_.connectedPerm = value;
      return;
    }
    break;
  case 'i':
    if (name == "initialPerm") {
      if (tm_)
        tm_->setParameter(name, value);
      args_.initialPerm = value;
      return;
    }
    break;
  case 'p':
    if (name == "permanenceInc") {
      if (tm_)
        tm_->setParameter(name, value);
      args_.permanenceInc = value;
      return;
    }
    if (name == "permanenceDec") {
      if (tm_)
        tm_->setParameter(name, value);
      args_.permanenceDec = value;
      return;
    }
    if (name == "permanenceMax") {
      if (tm_)
        tm_->setParameter(name, value);
      args_.permanenceMax = value;
      return;
    }
    if (name == "globalDecay") {
      if (tm_)
        tm_->setParameter(name, value);
      args_.globalDecay = value;
      return;
    }
    break;
  } // switch
  RegionImpl::setParameterReal32(name, index, value);
}

void TMRegion::setParameterBool(const std::string &name, Int64 index,
                                bool value) {
  if (name == "doPooling") {
    if (tm_)
      tm_->setParameter(name, value);
    args_.doPooling = value;
    return;
  }
  if (name == "collectStats") {
    if (tm_)
      tm_->setParameter(name, value);
    args_.collectStats = value;
    return;
  }

  if (name == "checkSynapseConsistency") {
    if (tm_)
      tm_->setParameter(name, value);
    args_.checkSynapseConsistency = value;
    return;
  }
  if (name == "learningMode") {
    args_.learningMode = value;
    return;
  }
  if (name == "inferenceMode") {
    args_.inferenceMode = value;
    return;
  }
  if (name == "anomalyMode") {
    args_.anomalyMode = value;
    return;
  }
  if (name == "topDownMode") {
    args_.topDownMode = value;
    return;
  }
  if (name == "storeDenseOutput") {
    args_.storeDenseOutput = value;
    return;
  }
  if (name == "computePredictedActiveCellIndices") {
    args_.computePredictedActiveCellIndices = value;
    return;
  }
  if (name == "orColumnOutputs") {
    args_.orColumnOutputs = value;
    return;
  }

  RegionImpl::setParameterBool(name, index, value);
}

void TMRegion::setParameterString(const std::string &name, Int64 index,
                                  const std::string &value) {
  if (name == "cellsSavePath") {
    args_.cellsSavePath = value;
  } else if (name == "logPathOutput") {
    args_.logPathOutput = value;
  } else if (name == "outputType") {
    if (tm_)
      tm_->setParameter(name, value);
    strncpy(args_.outputType, value.c_str(), sizeof(args_.outputType));
  } else
    this->RegionImpl::setParameterString(name, index, value);
}

void TMRegion::getParameterFromBuffer(const std::string &name, Int64 index,
                                      IWriteBuffer &value) {
  NTA_THROW << "SPRegion::getParameterFromBuffer() -- unknown name " << name;
}

void TMRegion::setParameterFromBuffer(const std::string &name, Int64 index,
                                      IReadBuffer &value) {
  NTA_THROW << "SPRegion::setParameter -- Unknown parameter " << name;
}

void TMRegion::serialize(BundleIO &bundle) {
  std::ofstream &f = bundle.getOutputStream("TMRegion");
  // There is more than one way to do this. We could serialize to YAML, which
  // would make a readable format, or we could serialize directly to the
  // stream Choose the easier one.
  bool init = ((tm_) ? true : false);
  f << "TMRegion" << std::endl
    << std::setprecision(std::numeric_limits<Real32>::max_digits10)
    << args_.numberOfCols << " " << args_.cellsPerColumn << " "
    << args_.initialPerm << " " << args_.connectedPerm << " "
    << args_.minThreshold << " " << args_.newSynapseCount << " "
    << args_.permanenceInc << " " << args_.permanenceDec << " "
    << args_.permanenceMax << " " << args_.globalDecay << " "
    << args_.activationThreshold << " " << args_.doPooling << " "
    << args_.segUpdateValidDuration << " " << args_.burnIn << " "
    << args_.collectStats << " " << args_.seed << " " << args_.verbosity << " "
    << args_.checkSynapseConsistency << " " << args_.pamLength << " "
    << args_.maxInfBacktrack << " " << args_.maxLrnBacktrack << " "
    << args_.maxAge << " " << args_.maxSeqLength << " "
    << args_.maxSegmentsPerCell << " " << args_.maxSegmentsPerSegment << " "
    << "\"" << args_.outputType << "\" "
    << "\"" << args_.cellsSavePath << "\" "
    << "\"" << args_.logPathOutput << "\" "
    << "\"" << args_.temporalImp << "\" " << args_.learningMode << " "
    << args_.inferenceMode << " " << args_.anomalyMode << " "
    << args_.topDownMode << " " << args_.storeDenseOutput << " "
    << args_.computePredictedActiveCellIndices << " " << args_.orColumnOutputs
    << " " << args_.outputWidth << " " << iter_ << " " << init << " "
    << sequencePos_ << " " << std::endl;
  if (tm_)
    tm_->save(f);
  f.close();
}

void TMRegion::deserialize(BundleIO &bundle) {
  std::ifstream &f = bundle.getInputStream("TMRegion");
  // There is more than one way to do this. We could serialize to YAML, which
  // would make a readable format, or we could serialize directly to the
  // stream. Choose the easier one.
  bool init;
  std::string outputType;
  std::string signatureString;
  f >> signatureString;
  if (signatureString != "TMRegion") {
    NTA_THROW << "Bad serialization for region '" << region_->getName()
              << "' of type TMRegion. Main serialization file must start "
              << "with \"TMRegion\" but instead it starts with '"
              << signatureString << "'";
  }
  f >> args_.numberOfCols;
  f >> args_.cellsPerColumn;
  f >> args_.initialPerm;
  f >> args_.connectedPerm;
  f >> args_.minThreshold;
  f >> args_.newSynapseCount;
  f >> args_.permanenceInc;
  f >> args_.permanenceDec;
  f >> args_.permanenceMax;
  f >> args_.globalDecay;
  f >> args_.activationThreshold;
  f >> args_.doPooling;
  f >> args_.segUpdateValidDuration;
  f >> args_.burnIn;
  f >> args_.collectStats;
  f >> args_.seed;
  f >> args_.verbosity;
  f >> args_.checkSynapseConsistency;
  f >> args_.pamLength;
  f >> args_.maxInfBacktrack;
  f >> args_.maxLrnBacktrack;
  f >> args_.maxAge;
  f >> args_.maxSeqLength;
  f >> args_.maxSegmentsPerCell;
  f >> args_.maxSegmentsPerSegment;
  f >> outputType;
  f >> args_.cellsSavePath;
  f >> args_.logPathOutput;
  f >> args_.temporalImp;
  f >> args_.learningMode;
  f >> args_.inferenceMode;
  f >> args_.anomalyMode;
  f >> args_.topDownMode;
  f >> args_.storeDenseOutput;
  f >> args_.computePredictedActiveCellIndices;
  f >> args_.orColumnOutputs;
  f >> args_.outputWidth;
  f >> iter_;
  f >> init;
  f >> sequencePos_;

  if (outputType[0] == '"')
    outputType =
        outputType.substr(1, outputType.length() - 2);
  strncpy(args_.outputType, outputType.c_str(), sizeof(args_.outputType));
  if (args_.cellsSavePath[0] == '"')
    args_.cellsSavePath =
        args_.cellsSavePath.substr(1, args_.cellsSavePath.length() - 2);
  if (args_.logPathOutput[0] == '"')
    args_.logPathOutput =
        args_.logPathOutput.substr(1, args_.logPathOutput.length() - 2);
  if (args_.temporalImp[0] == '"')
    args_.temporalImp =
        args_.temporalImp.substr(1, args_.temporalImp.length() - 2);

  if (init) {
    tm_ = new nupic::algorithms::backtracking_tm::BacktrackingTMCpp();
    tm_->load(f);
  } else
    tm_ = nullptr;
  f.close();
}

} // namespace nupic
