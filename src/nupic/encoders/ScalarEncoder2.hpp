/* ---------------------------------------------------------------------
* Numenta Platform for Intelligent Computing (NuPIC)
* Copyright (C) 2014-2016, Numenta, In  Unless you have an agreement
* with Numenta, In, for a separate license for this software code, the
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
* ---------------------------------------------------------------------
*/

#ifndef NTA_ScalarEncoder2_HPP
#define NTA_ScalarEncoder2_HPP

#include <cstring>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <tuple>
#include <map>
#include <list>
#include <cassert>
#include <cmath> //std::isnan

#include "Encoder.hpp" 
#include "RangeList.hpp" 
#include "DecodeResult.hpp"
#include "Utils.hpp" 

using namespace std;
using namespace nupic;

namespace encoders {

	/**
	* DOCUMENTATION TAKEN DIRECTLY FROM THE PYTHON VERSION:
	*
	* A scalar encoder encodes a numeric (floating point) value into an array
	* of bits. The output is 0's except for a contiguous block of 1's. The
	* location of this contiguous block varies continuously with the input value.
	*
	* The encoding is linear. If you want a nonlinear encoding, just transform
	* the scalar (e.g. by applying a logarithm function) before encoding.
	* It is not recommended to bin the data as a pre-processing step, e.g.
	* "1" = $0 - $.20, "2" = $.21-$0.80, "3" = $.81-$1.20, etc as this
	* removes a lot of information and prevents nearby values from overlapping
	* in the output. Instead, use a continuous transformation that scales
	* the data (a piecewise transformation is fine).
	*
	*
	* Parameters:
	* -----------------------------------------------------------------------------
	* w --        The number of bits that are set to encode a single value - the
	*             "width" of the output signal
	*             restriction: w must be odd to avoid centering problems.
	*
	* minval --   The minimum value of the input signal.
	*
	* maxval --   The upper bound of the input signal
	*
	* periodic -- If true, then the input value "wraps around" such that minval = maxval
	*             For a periodic value, the input must be strictly less than maxval,
	*             otherwise maxval is a true upper bound.
	*
	* There are three mutually exclusive parameters that determine the overall size of
	* of the output. Only one of these should be specified to the constructor:
	*
	* n      --      The number of bits in the output. Must be greater than or equal to w
	* radius --      Two inputs separated by more than the radius have non-overlapping
	*                representations. Two inputs separated by less than the radius will
	*                in general overlap in at least some of their bits. You can think
	*                of this as the radius of the input.
	* resolution --  Two inputs separated by greater than, or equal to the resolution are guaranteed
	*                to have different representations.
	*
	* Note: radius and resolution are specified w.r.t the input, not output. w is
	* specified w.r.t. the output.
	*
	* Example:
	* day of week.
	* w = 3
	* Minval = 1 (Monday)
	* Maxval = 8 (Monday)
	* periodic = true
	* n = 14
	* [equivalently: radius = 1.5 or resolution = 0.5]
	*
	* The following values would encode midnight -- the start of the day
	* monday (1)   -> 11000000000001
	* tuesday(2)   -> 01110000000000
	* wednesday(3) -> 00011100000000
	* ...
	* sunday (7)   -> 10000000000011
	*
	* Since the resolution is 12 hours, we can also encode noon, as
	* monday noon  -> 11100000000000
	* monday midnight-> 01110000000000
	* tuesday noon -> 00111000000000
	* etc
	*
	*
	* It may not be natural to specify "n", especially with non-periodic
	* data. For example, consider encoding an input with a range of 1-10
	* (inclusive) using an output width of 5.  If you specify resolution =
	* 1, this means that inputs of 1 and 2 have different outputs, though
	* they overlap, but 1 and 1.5 might not have different outputs.
	* This leads to a 14-bit representation like this:
	*
	* 1 ->  11111000000000  (14 bits total)
	* 2 ->  01111100000000
	* ...
	* 10->  00000000011111
	* [resolution = 1; n=14; radius = 5]
	*
	* You could specify resolution = 0.5, which gives
	* 1   -> 11111000... (22 bits total)
	* 1.5 -> 011111.....
	* 2.0 -> 0011111....
	* [resolution = 0.5; n=22; radius=2.5]
	*
	* You could specify radius = 1, which gives
	* 1   -> 111110000000....  (50 bits total)
	* 2   -> 000001111100....
	* 3   -> 000000000011111...
	* ...
	* 10  ->                           .....000011111
	* [radius = 1; resolution = 0.2; n=50]
	*
	*
	* An N/M encoding can also be used to encode a binary value,
	* where we want more than one bit to represent each state.
	* For example, we could have: w = 5, minval = 0, maxval = 1,
	* radius = 1 (which is equivalent to n=10)
	* 0 -> 1111100000
	* 1 -> 0000011111
	*
	*
	* Implementation details:
	* --------------------------------------------------------------------------
	* range = maxval - minval
	* h = (w-1)/2  (half-width)
	* resolution = radius / w
	* n = w * range/radius (periodic)
	* n = w * range/radius + 2 * h (non-periodic)
	*
	*/

	template<typename T>
	class ScalarEncoder2 : public Encoder<T> {

	public:

		/***************** constructors and destructor *****************/
		ScalarEncoder2();
		~ScalarEncoder2();

		/***************** methods *****************/

		/**
		* @param w				 -- number of bits to set in output
		* @param n				   number of bits in the representation (must be > w)
		*                       if n > 0, then update radius and resolution using n
		*                       else      using radius (resolution) for calculating n and resolution (radius)
		* @param minval			 -- minimum input value
		* @param maxval			 -- maximum input value (input is strictly less if periodic == True)
		*
		* Exactly one of n, radius, resolution must be set. "0" is a special
		* value that means "not set".
		*
		* @param name			 -- an optional string which will become part of the description
		* @param radius			 -- inputs separated by more than, or equal to this distance will have non-overlapping  representations
		* @param resolution		 -- inputs separated by more than, or equal to this distance will have different  representations
		* @param periodic		 -- If true, then the input value "wraps around" such that minval = maxval
		*							For a periodic value, the input must be strictly less than maxval,
		*							otherwise maxval is a true upper bound.
		* @param forced			 -- default false, if true, skip some safety checks (for compatibility reasons)
		*
		* @Override
		*/
		void init(int w, int n, double minVal, double maxVal, string name="", double radius=0., double resolution=0., bool periodic=0, bool forced=0);

		/**
		* Init the encoder using the number of bits in the representation
		*    radius and resolution are automatically calculated
		*
		* @param w				number of bits to set in output
		* @param minVal			minimum input value
		* @param maxVal			maximum input value (input is strictly less if periodic == True)
		* @param n				number of bits in the representation (must be > w)
		* @param periodic		input is periodic
		* @param name			an optional string which will become part of the description
		* @param forced			default false, if true, skip some safety checks (for compatibility reasons)
		*/
		void initEncoderN(int w, double minVal, double maxVal, int n, bool periodic = false, string name = "", bool forced = 0);

		/**
		* Init the encoder using resolution
		*    number of bits in the representation and resolution are automatically calculated
		*
		* @param w				number of bits to set in output
		* @param minVal			minimum input value
		* @param maxVal			maximum input value (input is strictly less if periodic == True)
		* @param resolution		inputs separated by more than, or equal to this distance will have different representations
		* @param periodic		input is periodic
		*/
		void initEncoderR(int w, double minVal, double maxVal, double resolution, bool periodic = false, string name = "", bool forced = 0);

		/**
		* Return the bit offset of the first bit to be set in the encoder output.
		* For periodic encoders, this can be a negative number when the encoded output
		* wraps around.
		*
		* @param c			the memory
		* @param input		the input data
		*
		* @return			an encoded array
		*/
		int getFirstOnBit(double input) const;

		/**
		* Check if the settings are reasonable for the SpatialPooler to work
		*/
		void checkReasonableSettings();

		/**
		* Returns an array containing the sub-field bucket indices for
		* each sub-field of the inputData. To get the associated field names for each of
		* the buckets, call getScalarNames().
		*
		* @param  	input 	The data from the source. This is typically a object with members.
		*
		* @return 	array of bucket indices
		* @Override
		*/
		vector<nupic::UInt> getBucketIndices(double input);

		/**
		* Returns a RangeLists which is a list of range
		* minimum and maximum objects in the first entry
		* and descriptions for each range in the second entry,
		* and the field name in the third entry
		*
		* @param encoded			the encoded bit vector
		* @param parentFieldName	the field the vector corresponds with
		* @return
		* @Override
		*/
		virtual DecodeResult decode(vector<nupic::UInt> encoded, std::string parentFieldName) override;

		/**
		* Generate description from a text description of the ranges
		*
		* @param	ranges		A list of MinMax objects.
		*/
		std::string generateRangeDescription(vector<pair<double, double>> ranges);

		/**
		* Returns a list of EncoderResult named tuples describing the top-down
		* best guess inputs for each sub-field given the encoded output. These are the
		* values which are most likely to generate the given encoded output.
		* To get the associated field names for each of the values, call
		* getScalarNames().
		*
		* @param encoded	The encoded output. Typically received from the topDown outputs
		*					from the spatial pooler just above us.
		*
		* @returns A list of EncoderResult named tuples. Each EncoderResult has
		*        three attributes:
		*
		*        -# value:         This is the best-guess value for the sub-field
		*                          in a format that is consistent with the type
		*                          specified by getDecoderOutputFieldTypes().
		*                          Note that this value is not necessarily
		*                          numeric.
		*
		*        -# scalar:        The scalar representation of this best-guess
		*                          value. This number is consistent with what
		*                          is returned by getScalars(). This value is
		*                          always an int or float, and can be used for
		*                          numeric comparisons.
		*
		*        -# encoding       This is the encoded bit-array
		*                          that represents the best-guess value.
		*                          That is, if 'value' was passed to
		*                          encode(), an identical bit-array should be
		*                          returned.
		* @Override
		*/
		virtual vector< tuple<boost::any, boost::any, vector<nupic::UInt>> > topDownCompute(vector<nupic::UInt> encoded) override;

		/**
		* Returns a list of {@link EncoderResult}s describing the inputs for
		* each sub-field that correspond to the bucket indices passed in 'buckets'.
		* To get the associated field names for each of the values, call getScalarNames().
		*
		* @param buckets 	The list of bucket indices, one for each sub-field encoder.
		*              		These bucket indices for example may have been retrieved
		*              		from the getBucketIndices() call.
		*
		* @return A list of {@link EncoderResult}s. Each EncoderResult has
		* @Override
		*/
		vector< tuple<boost::any, boost::any, vector<nupic::UInt>> > getBucketInfo(vector<nupic::UInt> buckets);

		/**
		* Return the internal topDownMapping matrix used for handling the
		* bucketInfo() and topDownCompute() methods. This is a matrix, one row per
		* category (bucket) where each row contains the encoded output for that
		* category.
		*
		* @return		the internal topDownMapping
		*/
		map<nupic::UInt, vector<nupic::UInt>> getTopDownMapping();

		/**
		* Encodes inputData and puts the encoded value into the output array,
		* which is a 1-D array of length returned by {@link Connections#getW()}.
		*
		* Note: The output array is reused, so clear it before updating it.
		*
		* @param inputData		Data to encode. This should be validated by the encoder.
		* @param output			1-D array of same length returned by {@link Connections#getW()}
		*
		* @Override
		*/
		virtual void encodeIntoArray(T inputData, vector<nupic::UInt> &output) override;

		/**
		* Should return the output width, in bits.
		* @Override
		*/
		virtual int getWidth() const override;

		// check if scalar encoder class is NULL
		bool isnull() const { return null_; }
		void setNotNull() { null_ = false; }

	protected:
		bool null_;

	}; // end class ScalarEncoder2

	/***************** Start of Implementation ******************/
	template<class T>
	ScalarEncoder2<T>::ScalarEncoder2() : null_(true) {}

	template<class T>
	ScalarEncoder2<T>::~ScalarEncoder2() {}

	template<class T>
	void ScalarEncoder2<T>::init(int w, int n, double minVal, double maxVal, string name, double radius, double resolution, bool periodic, bool forced) {
		this->setW(w);
		this->setN(n);
		this->setMinVal(minVal);
		this->setMaxVal(maxVal);
		this->setName(name);
		this->setResolution(resolution);
		this->setRadius(radius);
		this->setPeriodic(periodic);
		this->setForced(forced);

		if (this->getW() % 2 == 0) {
			NTA_THROW << "W must be an odd number (to eliminate centering difficulty)" ;
			//exit(-1);
		}

		this->setHalfWidth((this->getW() - 1) / 2);

		// For non-periodic inputs, padding is the number of bits "outside" the range,
		// on each side. I.e. the representation of minval is centered on some bit, and
		// there are "padding" bits to the left of that centered bit; similarly with
		// bits to the right of the center bit of maxval
		this->setPadding(this->isPeriodic() ? 0 : this->getHalfWidth());

		// There are three different ways of thinking about the representation. Handle
		// each case here.
		if (!isnan(this->getMinVal()) && !isnan(this->getMaxVal())) {
			if (this->getMinVal() >= this->getMaxVal()) {
				NTA_THROW << "maxVal must be > minVal";
				//exit(-1);
			}
			this->setRangeInternal(this->getMaxVal() - this->getMinVal());
		}

		if (n != 0) {
			if (!isnan(minVal) && !isnan(maxVal)) {
				if (!this->isPeriodic()) this->setResolution(this->getRangeInternal() / (this->getN() - this->getW()));
				else               this->setResolution(this->getRangeInternal() / this->getN());

				this->setRadius(this->getW() * this->getResolution());

				if (this->isPeriodic()) this->setRange(this->getRangeInternal());
				else              this->setRange(this->getRangeInternal() + this->getResolution());
			}
		}
		else {
			if (radius != 0) {
				this->setResolution(this->getRadius() / w);
			}
			else if (resolution != 0) {
				this->setRadius(this->getResolution() * w);
			}
			else {
				NTA_THROW << "One of n, radius, resolution must be specified for a ScalarEncoder2";
				//exit(-1);
			}

			if (this->isPeriodic()) this->setRange(this->getRangeInternal());
			else              this->setRange(this->getRangeInternal() + this->getResolution());

			const double nFloat = w * (this->getRange() / this->getRadius()) + 2 * this->getPadding();
			this->setN((int)ceil(nFloat));
		}

		// nInternal represents the output area excluding the possible padding on each side
		if (!isnan(this->getMinVal()) && !isnan(this->getMaxVal())) {
			this->setNInternal(this->getN() - 2 * this->getPadding());
		}

		stringstream ss;
		ss << "[" << this->getMinVal() << ":" << this->getMaxVal() << "]";

		// Our name
		if (this->getName() == "") this->setName(ss.str());

		// checks for likely mistakes in encoder settings
		if (!this->isForced()) checkReasonableSettings();

		this->description_.push_back(std::make_tuple(this->getName() == "" ? ss.str() : this->getName(), 0));

		// scalar encoder class is not NULL
		setNotNull();
	}


	// 2 practical functions:
	template<class T>
	void ScalarEncoder2<T>::initEncoderN(int w, double minVal, double maxVal, int n, bool periodic, string name, bool forced)
	{
		init(w, n, minVal, maxVal, name, 0., 0., periodic, forced);
	}


	template<class T>
	void ScalarEncoder2<T>::initEncoderR(int w, double minVal, double maxVal, double resolution, bool periodic, string name, bool forced)
	{
		init(w, 0, minVal, maxVal, name , 0., resolution, periodic, forced);
	}


	template<class T>
	void ScalarEncoder2<T>::checkReasonableSettings() {
		if (this->getW() < 21) {
			NTA_THROW <<  "Number of bits in the SDR must be greater than 2, and recommended >= 21 (use forced=True to override)";
			//exit(-1);
		}
	}


	template<class T>
	int ScalarEncoder2<T>::getFirstOnBit(double input) const{
		if (input == SENTINEL_VALUE_FOR_MISSING_DATA) {
			return 0;
		}
		else {
			if (input < this->getMinVal()) {
				if (this->clipInput() && !this->isPeriodic()) {
					input = this->getMinVal();
				}
				else {
					NTA_THROW << "input (" << input << ") less than range (" << this->getMinVal() << " - " << this->getMaxVal();
					//exit(-1);
				}
			}
		}
		if (this->isPeriodic()) {
			if (input >= this->getMaxVal()) {
				NTA_THROW << "input (" << input << ") greater than periodic range (" << this->getMinVal() << " - " << this->getMaxVal();
				//exit(-1);
			}
		}
		else {
			if (input > this->getMaxVal()) {
				if (this->clipInput()) {
					input = this->getMaxVal();
				}
				else {
					NTA_THROW << "input (" << input << ") greater than periodic range (" << this->getMinVal() << " - " << this->getMaxVal();
					//exit(-1);
				}
			}
		}
		int centerbin = 0;
		if (this->isPeriodic())
			centerbin = ((int)((input - this->getMinVal()) *  this->getNInternal() / this->getRange())) + this->getPadding();
		else
			centerbin = ((int)(((input - this->getMinVal()) + this->getResolution() / 2) / this->getResolution())) + this->getPadding();

		return (centerbin - this->getHalfWidth());
	}


	template<class T>
	void ScalarEncoder2<T>::encodeIntoArray(T inputData, vector<nupic::UInt>& output) {
		output.resize(this->getN(), 0);
		if (isnan(boost::any_cast<double>(inputData))) {
			return;
		}
		int bucketVal = getFirstOnBit(boost::any_cast<double>(inputData));
		if (!isnan(bucketVal)) {
			int minbin = bucketVal;
			int maxbin = minbin + 2 * this->getHalfWidth();
			if (this->isPeriodic()) {
				if (maxbin >= this->getN()) {
					int bottombins = maxbin - this->getN() + 1;
					vector<int> rangeVec = Utils<int>::range(0, bottombins);
					Utils<nupic::UInt>::setIndexesTo(output, rangeVec, 1);
					maxbin = this->getN() - 1;
				}
				if (minbin < 0) {
					int topbins = -minbin;
					Utils<unsigned int>::setIndexesTo(output, Utils<int>::range(this->getN() - topbins, this->getN()), 1);
					minbin = 0;
				}
			}
			Utils<nupic::UInt>::setIndexesTo(output, Utils<int>::range(minbin, maxbin + 1), 1);
		}
	}


	template<class T>
	DecodeResult ScalarEncoder2<T>::decode(vector<nupic::UInt> encoded, std::string parentFieldName) {

		// For now, we simply assume any top-down output greater than 0
		// is ON. Eventually, we will probably want to incorporate the strength
		// of each top-down output.
		if (encoded.empty() || encoded.size() < 1) {
			return DecodeResult();
		}
		vector<nupic::UInt> tmpOutput = encoded;

		// ------------------------------------------------------------------------
		// First, assume the input pool is not sampled 100%, and fill in the
		//  "holes" in the encoded representation (which are likely to be present
		//  if this is a coincidence that was learned by the SP).

		// Search for portions of the output that have "holes"
		int maxZerosInARow = this->getHalfWidth();

		for (int i = 0; i < maxZerosInARow; i++) {
			const int subLen = i + 3;
			vector<int>  searchStr(subLen, 1);
			Utils<int>::setRangeTo(searchStr, 1, -1, 0);

			// Does this search string appear in the output?
			if (this->isPeriodic()) {
				for (int j = 0; j < this->getN(); j++) {
					vector<int> outputIndices = Utils<int>::range(j, j + subLen);
					outputIndices = Utils<int>::modulo(outputIndices, this->getN());
					vector<nupic::UInt> temp = Utils<nupic::UInt>::sub(tmpOutput, outputIndices);
					if (std::equal(searchStr.begin(), searchStr.end(), temp.begin())) {
						Utils<nupic::UInt>::setIndexesTo(tmpOutput, outputIndices, 1);
					}
				}
			}
			else {
				for (int j = 0; j < this->getN() - subLen + 1; j++) {
					vector<nupic::UInt> temp = Utils<nupic::UInt>::sub(tmpOutput, Utils<int>::range(j, j + subLen));
					if (std::equal(searchStr.begin(), searchStr.end(), temp.begin())) {
						Utils<nupic::UInt>::setRangeTo(tmpOutput, j, j + subLen, 1);
					}
				}
			}
		}
		// Find each run of 1's.
		vector<unsigned int> nz = Utils<nupic::UInt>::where(tmpOutput, Utils<nupic::UInt>::WHERE_GREATER_THAN_0);

		//will be tuples of (startIdx, runLength)
		vector<tuple<int, int>> runs;
		runs.reserve(nz.size());
		std::sort(nz.begin(), nz.end());
		UInt run[] = { (UInt)nz[0], 1 };
		for (size_t i = 1; i < nz.size(); ++i) {
			if (nz[i] == (run[0] + run[1])) {
				run[1] += 1;
			}
			else {
				runs.push_back(tuple<int, int>(run[0], run[1]));
				run[0] = nz[i];
				run[1] = 1;
			}
		}

		runs.push_back(tuple<int, int>(run[0], run[1]));

		// If we have a periodic encoder, merge the first and last run if they
		// both go all the way to the edges
		if (this->isPeriodic() && runs.size() > 1) {
			int l = runs.size() - 1;

			if (((int)std::get<0>(runs[0])) == 0 && ((int)std::get<0>(runs[1])) + ((int)std::get<1>(runs[1])) == this->getN()) {

				runs[l] = tuple<int, int>((int)std::get<0>(runs[1]), ((int)std::get<1>(runs[1]) + (int)std::get<1>(runs[0])));

				// subList of runs
				vector<tuple<int, int>> subList(runs.begin() + 1, runs.end());
				runs = subList;
			}
		}

		// ------------------------------------------------------------------------
		// Now, for each group of 1's, determine the "left" and "right" edges, where
		// the "left" edge is inset by halfwidth and the "right" edge is inset by
		// halfwidth.
		// For a group of width w or less, the "left" and "right" edge are both at
		// the center position of the group.
		int left = 0;
		int right = 0;

		// pair of <Min,Max>
		vector<pair<double, double>> ranges;
		ranges.reserve(2*runs.size()); // double reservation for covering the case of periodic signal
		for (tuple<int, int> tupleRun : runs) {
			int start = (int)get<0>(tupleRun);
			int runLen = (int)get<1>(tupleRun);
			if (runLen <= this->getW()) {
				left = right = start + runLen / 2;
			}
			else {
				left = start + this->getHalfWidth();
				right = start + runLen - 1 - this->getHalfWidth();
			}

			double inMin, inMax;

			// Convert to input space.
			if (!this->isPeriodic()) {
				inMin = (left - this->getPadding()) * this->getResolution() + this->getMinVal();
				inMax = (right - this->getPadding()) * this->getResolution() + this->getMinVal();
			}
			else {
				inMin = (left - this->getPadding()) * this->getRange() / this->getNInternal() + this->getMinVal();
				inMax = (right - this->getPadding()) * this->getRange() / this->getNInternal() + this->getMinVal();
			}

			// Handle wrap-around if periodic
			if (this->isPeriodic()) {
				if (inMin >= this->getMaxVal()) {
					inMin -= this->getRange();
					inMax -= this->getRange();
				}
			}

			// Clip low end
			if (inMin < this->getMinVal()) inMin = this->getMinVal();
			if (inMax < this->getMinVal()) inMax = this->getMinVal();

			// If we have a periodic encoder, and the max is past the edge, break into
			// 	2 separate ranges
			if (this->isPeriodic() && inMax >= this->getMaxVal()) {
				ranges.push_back(std::make_pair(inMin, this->getMaxVal()));
				ranges.push_back(std::make_pair(this->getMinVal(), (inMax - this->getRange())));
			}
			else {
				if (inMax > this->getMaxVal()) inMax = this->getMaxVal();				
				if (inMin > this->getMaxVal()) inMin = this->getMaxVal();  // ??? BINH: getMinVal()????

				ranges.push_back(std::make_pair(inMin, inMax));
			}
		}

		std::string desc = generateRangeDescription(ranges);
		std::string fieldName;

		// Return result
		if (parentFieldName != "" && !parentFieldName.empty()) {
			fieldName = parentFieldName + "." + this->getName();
		}
		else {
			fieldName = this->getName();
		}

#if (_MSC_VER > 1800)	// MSVC2015...
		map<string, RangeList>  fieldsDict = { make_pair(fieldName, RangeList(ranges, desc)) };
		vector<string> listFieldName = { fieldName };
#else
		map<string, RangeList>  fieldsDict;
		fieldsDict.insert(make_pair(fieldName, RangeList(ranges, desc)));
		vector<string> listFieldName;
		listFieldName.push_back(fieldName);
#endif

		return DecodeResult(fieldsDict, listFieldName);
	}


	template<class T>
	std::string ScalarEncoder2<T>::generateRangeDescription(vector<pair<double, double>> ranges) {
		std::stringstream desc;
		size_t numRanges = ranges.size();

		for (size_t i = 0; i < numRanges; i++) {
			if (ranges[i].first != ranges[i].second) {
				desc << " " << ranges[i].first << ranges[i].second;
			}
			else {
				desc << " " << ranges[i].first;
			}
			if (i < numRanges - 1) {
				desc << ", ";
			}
		}
		return desc.str();
	}


	template<class T>
	vector<nupic::UInt> ScalarEncoder2<T>::getBucketIndices(double input) {
	
		//for non-periodic encoders, the bucket index is the index of the left bit
		int bucketIdx = getFirstOnBit(input);
		if (this->isPeriodic()) {
			//For periodic encoders, the bucket index is the index of the center bit
			bucketIdx += this->getHalfWidth();
			if (bucketIdx < 0) bucketIdx += this->getN();
		}
		
#if (_MSC_VER > 1800)	// MSVC2015...
      return vector<nupic::UInt>({ (nupic::UInt)bucketIdx });
#else
		vector<nupic::UInt> res;
		res.push_back(static_cast<nupic::UInt>(bucketIdx) );
		return res;
#endif
	}


	template<class T>
	vector< tuple<boost::any, boost::any, vector<nupic::UInt>> > ScalarEncoder2<T>::topDownCompute(vector<nupic::UInt> encoded) {
		//Get/generate the topDown mapping table
		map<nupic::UInt, vector<nupic::UInt>> topDownMapping = getTopDownMapping();

		// See which "category" we match the closest.
		int category = Utils<int>::argmax(this->rightVecProd(topDownMapping, encoded));

#if (_MSC_VER > 1800)	// MSVC2015...
        return ScalarEncoder2<T>::getBucketInfo({ static_cast<nupic::UInt>(category) });
#else
		vector<nupic::UInt> res;
		res.push_back(static_cast<nupic::UInt>(category) );
		return ScalarEncoder2<T>::getBucketInfo( res );
#endif
	}


	template<class T>
	vector< tuple<boost::any, boost::any, vector<nupic::UInt>> > ScalarEncoder2<T>::getBucketInfo(vector<nupic::UInt> buckets) {

		//Get/generate the topDown mapping table
		map<nupic::UInt, vector<nupic::UInt>> topDownMapping = getTopDownMapping();

		//The "category" is simply the bucket index
		int category = buckets[0];
		vector<nupic::UInt> encoding = topDownMapping.find(category)->second;

		//Which input value does this correspond to?
		double inputVal;
		if (this->isPeriodic()) {
			inputVal = this->getMinVal() + this->getResolution() / 2 + category * this->getResolution();
		}
		else {
			inputVal = this->getMinVal() + category * this->getResolution();
		}

		//A list of EncoderResult named tuples. Each EncoderResult has three attributes:
		//value, scalar, encoding (encoded bit-array)
#if (_MSC_VER > 1800)	// MSVC2015...
        return { make_tuple(inputVal, inputVal, encoding) };
#else
		vector< tuple<boost::any, boost::any, vector<nupic::UInt>> >  res;
		res.push_back(make_tuple(inputVal,inputVal,encoding));
		return res;
#endif
	}


	template<class T>
	map<nupic::UInt, vector<nupic::UInt>> ScalarEncoder2<T>::getTopDownMapping() {
		if (this->topDownMapping_.empty()) {
			//The input scalar value corresponding to each possible output encoding
			if (this->isPeriodic()) {
				this->setTopDownValues(Utils<double>::arange(this->getMinVal() + this->getResolution() / 2.0,	this->getMaxVal(), this->getResolution()));
			}
			else {
				//Number of values is (max-min)/resolutions
				this->setTopDownValues(Utils<double>::arange(this->getMinVal(), this->getMaxVal() + this->getResolution() / 2.0, this->getResolution()));
			}
		}

		//This is a matrix, one row per category (bucket) 
		//where each row contains the encoded output for that category
		map<nupic::UInt, vector<nupic::UInt>>  topDownMapping;
		//setTopDownMapping(topDownMapping);	// BINH
		vector<double> topDownValues = this->getTopDownValues();
		const double minVal = this->getMinVal();
		const double maxVal = this->getMaxVal();
		for (size_t i = 0; i < topDownValues.size(); i++) {
			//Each row represents an encoded output pattern
			double value = std::min(std::max(topDownValues[i], minVal), maxVal);
			vector<nupic::UInt> outputSpace(this->getN());
			ScalarEncoder2<T>::encodeIntoArray(value, outputSpace);
			topDownMapping.insert(pair<nupic::UInt, vector<nupic::UInt> >(i, outputSpace));
		}
		return topDownMapping;
	}

	template<class T>
	int ScalarEncoder2<T>::getWidth() const {
		return this->getN();
	}

}; // end namespace 
#endif // NTA_ScalarEncoder2_HPP
