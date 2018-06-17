/* ---------------------------------------------------------------------
 * Numenta Platform for Intelligent Computing (NuPIC)
 * Copyright (C) 2018, Numenta, Inc.  Unless you have an agreement
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
 * Author: David Keeney, June, 2018
 * ---------------------------------------------------------------------
 */

/** @file
 * Declarations for TMRegion class
 */

//----------------------------------------------------------------------

#ifndef NTA_TMREGION_HPP
#define NTA_TMREGION_HPP

#include <nupic/engine/RegionImpl.hpp>
//----------------------------------------------------------------------

namespace nupic
{
	class TMRegion  : public RegionImpl
	{
		typedef void (*computeCallbackFunc)(const std::string&);
		typedef std::map<std::string, Spec> SpecMap;
		
	public:
		TMRegion(const ValueMap& params, Region *region);
		TMRegion(BundleIO& bundle, Region* region);
		virtual ~TMRegion();


		/* -----------  Required RegionImpl Interface methods ------- */

		// Used by RegionImplFactory to create and cache
		// a nodespec. Ownership is transferred to the caller.
		static Spec* createSpec();

		std::string getNodeType() { return "SPRegion"; };

		// Compute outputs from inputs and internal state
		void compute() override;
		std::string executeCommand(const std::vector<std::string>& args, Int64 index) override;

		/**
		* Inputs/Outputs are made available in initialize()
		* It is always called after the constructor (or load from serialized state)
		*/
		void initialize() override;

		void serialize(BundleIO& bundle) override;
		void deserialize(BundleIO& bundle) override;


		// Per-node size (in elements) of the given output.
		// For per-region outputs, it is the total element count.
		// This method is called only for outputs whose size is not
		// specified in the spec.
		size_t getNodeOutputElementCount(const std::string& outputName) override;
		void getParameterFromBuffer(const std::string& name, Int64 index, IWriteBuffer& value) override;
		void setParameterFromBuffer(const std::string& name, Int64 index, IReadBuffer& value) override;

			/* -----------  Optional RegionImpl Interface methods ------- */
		UInt32 getParameterUInt32(const std::string& name, Int64 index) override;
		Int32 getParameterInt32(const std::string& name, Int64 index) override;
		Real32 getParameterReal32(const std::string& name, Int64 index) override;
		bool   getParameterBool(const std::string& name, Int64 index) override;
		std::string getParameterString(const std::string& name, Int64 index) override;
		void getParameterArray(const std::string& name, Int64 index, Array & array) override;
		size_t getParameterArrayCount(const std::string &name, Int64 index) override;


		void setParameterUInt32(const std::string& name, Int64 index, UInt32 value) override;
		void setParameterInt32(const std::string& name, Int64 index, Int32 value) override;
		void setParameterReal32(const std::string& name, Int64 index, Real32 value) override;
		void setParameterBool(const std::string& name, Int64 index, bool value) override;
		void setParameterString(const std::string& name, Int64 index, const std::string& s) override;

	
	private:
		TMRegion();  // empty constructor not allowed

		struct {
       UInt32 numberOfCols;
       UInt32 cellsPerColumn;
       Real32 initialPerm;
       Real32 connectedPerm;
       UInt32 minThreshold;
       UInt32 newSynapseCount;
       Real32 permanenceInc;
       Real32 permanenceDec;
       UInt32 permanenceMax;
       Real32 globalDecay;
       UInt32 activationThreshold;
       bool   doPooling;
       UInt32 segUpdateValidDuration;
       UInt32 burnIn;
       bool   collectStats;
       Int32  seed;
       bool   verbosity;
       bool   checkSynapseConsistency;
       UInt32 pamLength;
       UInt32 maxInfBacktrack;
       UInt32 maxLrnBacktrack;
       UInt32 maxAge;
       UInt32 maxSeqLength;
       Int32  maxSegmentsPerCell;
       Int32  maxSegmentsPerSegment;
       std::string outputType;

       // variables used by this class and not passed on
       std::string cellsSavePath;
       std::string logPathOutput;
       std::string temporalImp;
       bool learningMode;
       bool inferenceMode;
       bool anomalyMode;
       bool topDownMode;
       bool storeDenseOutput;
       bool computePredictedActiveCellIndices;
       UInt32 outputWidth; // columnCount *cellsPerColumn
    } args_;


		computeCallbackFunc computeCallback_;
		BacktrackingTMCCP* tm_;
	};


} // namespace nupic

#endif // NTA_SPREGION_HPP