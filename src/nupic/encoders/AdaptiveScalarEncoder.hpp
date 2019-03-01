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

#ifndef NTA_AdaptiveScalarEncoder_HPP
#define NTA_AdaptiveScalarEncoder_HPP

#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <tuple>
#include <map>
#include <list>
#include <array>         
#include <random>        
#include <chrono>
#include <stdexcept>
#include <cassert>
#include <cstring>
#include <cstdint>
#include <cmath> //std::isnan
#include <algorithm> 

#include "ScalarEncoder2.hpp"
#include "Encoder.hpp"
#include "Utils.hpp" 

using namespace std;

namespace encoders {

	/**
	* This is an implementation of the scalar encoder that adapts the min and
	* max of the scalar encoder dynamically. This is essential to the streaming
	* model of the online prediction framework.
	*
	* Initialization of an adaptive encoder using resolution or radius is not
	* supported; it must be initialized with n. This n is kept constant while
	* the min and max of the encoder changes.
	*
	* The adaptive encoder must be have periodic set to false.
	*
	* The adaptive encoder may be initialized with a minval and maxval or with
	* `None` for each of these. In the latter case, the min and max are set as
	* the 1st and 99th percentile over a window of the past 100 records.
	*
	* *Note:** the sliding window may record duplicates of the values in the
	* data set, and therefore does not reflect the statistical distribution of
	* the input data and may not be used to calculate the median, mean etc.
	*/

	template<typename T>
	class AdaptiveScalarEncoder : public ScalarEncoder2<T>
	{
	public:
		/***************** constructors and destructor *****************/
		AdaptiveScalarEncoder();
		~AdaptiveScalarEncoder();

		/***************** methods *****************/

		/**
		* @param w				 -- number of bits to set in output
		* @param n				 -- number of bits in the representation (must be > w)
		* @param minval			 -- minimum input value
		* @param maxval			 -- maximum input value (input is strictly less if periodic == True)
		*
		* Exactly one of n, radius, resolution must be set. "0" is a special
		* value that means "not set".
		*
		* @param name			 -- an optional string which will become part of the description
		* @param radius			 -- inputs separated by more than, or equal to this distance will have non-overlapping  representations
		* @param resolution		 -- inputs separated by more than, or equal to this distance will have different  representations
		* @param forced			 -- default false , if true, skip some safety checks (for compatibility reasons)
		*
		* @Override
		*/
		void init(int w, int n, double minVal, double maxVal, string name, double radius, double resolution, bool forced);

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
		vector< tuple<boost::any, boost::any, vector<nupic::UInt>> > topDownComputeA(vector<nupic::UInt> encoded);

		/**
		* Encodes inputData and puts the encoded value into the output array,
		* which is a 1-D array of length returned by {@link Connections#getW()}.
		*
		* Note: The output array is reused, so clear it before updating it.
		* @param inputData		Data to encode. This should be validated by the encoder.
		* @param output			1-D array of same length returned by {@link Connections#getW()}
		* @Override
		*/
		virtual void encodeIntoArray(T inputData, vector<nupic::UInt>& output);

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
		vector<nupic::UInt> getBucketIndicesA(double input);

		/**
		* Returns a list of {@link EncoderResult}s describing the inputs for
		* each sub-field that correspond to the bucket indices passed in 'buckets'.
		* To get the associated field names for each of the values, call getScalarNames().
		*
		* @param buckets 	The list of bucket indices, one for each sub-field encoder.
		*					These bucket indices for example may have been retrieved
		*					from the getBucketIndices() call.
		*
		* @return A list of {@link EncoderResult}s. Each EncoderResult has
		* @Override
		*/
		vector< tuple<boost::any, boost::any, vector<nupic::UInt>> > getBucketInfoA(vector<nupic::UInt> buckets);

	private:

		void setEncoderParams();
		void setMinAndMax(double input, bool learn);
		vector<double> deleteItem(vector<double> a, int i);
		vector<double> appendItem(vector<double> a, double input);
		vector<nupic::UInt> calculateBucketIndicesA(double input);

	protected:

		/** how many inputs have been sent to the encoder? */
		int recordNum_;

		unsigned int windowSize_;

		bool learningEnabled_;

		/** Invalidate the bucket values cache so that they get recomputed */
		double bucketValues_;

		/** the sliding window may record duplicates of the values in the dataset,
		and therefore does not reflect the statistical distribution of the input data
		and may not be used to calculate the median, mean etc. */
		vector<double> slidingWindow_;

	}; // end class AdaptiveScalarEncoder


	   /***************** Start of Implementation ******************/
	template<class T>
	AdaptiveScalarEncoder<T>::AdaptiveScalarEncoder() :
		recordNum_(0), windowSize_(300), learningEnabled_(true), bucketValues_(), slidingWindow_(0) {}

	template<class T>
	AdaptiveScalarEncoder<T>::~AdaptiveScalarEncoder() {}

	template<class T>
	void AdaptiveScalarEncoder<T>::init(int w, int n, double minVal, double maxVal, string name, double radius, double resolution, bool forced) {
		this->setW(w);
		this->setN(n);
		this->setMinVal(minVal);
		this->setMaxVal(maxVal);
		this->setName(name);
		this->setRadius(radius);
		this->setResolution(resolution);
		this->setForced(forced);

		this->setPeriodic(false);
		this->encLearningEnabled_ = true;

		if (this->periodic_) {
			// Adaptive scalar encoders take non-periodic inputs only
			NTA_THROW << "Adaptive scalar encoder does not encode periodic inputs";
		}
		// An adaptive encoder can only be intialized using n
		assert(n != 0);
		ScalarEncoder2<T>::init(w, n, minVal, maxVal, name, radius, resolution, this->periodic_, forced);
	}


	template<class T>
	vector< tuple<boost::any, boost::any, vector<nupic::UInt>> > AdaptiveScalarEncoder<T>::topDownComputeA(vector<nupic::UInt> encoded) {
		if (this->getMinVal() == 0 || this->getMaxVal() == 0) {
			vector< tuple<boost::any, boost::any, vector<nupic::UInt>> >  res;
			vector<nupic::UInt> enArray(this->getN(), 0);
			return { tuple<boost::any, boost::any, vector<nupic::UInt>> (0.0, 0.0, enArray) };
		}
		else return  ScalarEncoder2<T>::topDownCompute(encoded);
	}

	template<class T>
	void AdaptiveScalarEncoder<T>::encodeIntoArray(T inputData, vector<nupic::UInt>& output) {
		recordNum_ += 1;
		bool learn = false;
		if (!this->encLearningEnabled_) {
			learn = true;
		}
		if (boost::any_cast<double>(inputData) == SENTINEL_VALUE_FOR_MISSING_DATA) {
			std::fill(output.begin(), output.end(), 0);
		}
		else if (!isnan(boost::any_cast<double>(inputData))) {
			setMinAndMax(boost::any_cast<double>(inputData), learn);
		}

		ScalarEncoder2<T>::encodeIntoArray(inputData, output);
	}

	template<class T>
	void AdaptiveScalarEncoder<T>::setMinAndMax(double input, bool learn) {
		if (slidingWindow_.size() >= windowSize_) {
			slidingWindow_ = deleteItem(slidingWindow_, 0);
		}
		slidingWindow_ = appendItem(slidingWindow_, input);
		if (this->minVal_ == this->maxVal_) {
			this->minVal_ = input;
			this->maxVal_ = input + 1;
			setEncoderParams();
		}
		else {
			vector<double> sorted = Utils<double>::copyOf(slidingWindow_, slidingWindow_.size());
			std::sort(sorted.begin(), sorted.end());
			double minOverWindow = sorted[0];
			double maxOverWindow = sorted[sorted.size() - 1];
			if (minOverWindow < this->minVal_) {
				this->minVal_ = minOverWindow;
				setEncoderParams();
			}
			if (maxOverWindow > this->maxVal_) {
				this->maxVal_ = maxOverWindow;
				setEncoderParams();
			}
		}
	}

	template<class T>
	vector<double> AdaptiveScalarEncoder<T>::deleteItem(vector<double> a, int i) {
		a = Utils<double>::copyOfRange(a, 1, a.size() - 1);
		return a;
	}

	template<class T>
	vector<double> AdaptiveScalarEncoder<T>::appendItem(vector<double> a, double input) {
		a = Utils<double>::copyOf(a, a.size() + 1);
		a[a.size() - 1] = input;
		return a;
	}

	template<class T>
	void AdaptiveScalarEncoder<T>::setEncoderParams() {
		this->rangeInternal_	= double(this->maxVal_ - this->minVal_);
		this->resolution_		= double(this->rangeInternal_ / (this->n_ - this->w_));
		this->radius_			= this->w_ * this->resolution_;
		this->range_			= this->rangeInternal_ + this->resolution_;
		this->nInternal_		= this->n_ - 2 * this->padding_;
		this->bucketValues_		= 0;
	}

	template<class T>
	vector<nupic::UInt> AdaptiveScalarEncoder<T>::getBucketIndicesA(double input) {
		return this->calculateBucketIndices(input);
	}

	template<class T>
	vector<nupic::UInt> AdaptiveScalarEncoder<T>::calculateBucketIndicesA(double input) {
		recordNum_ += 1;
		bool learn = false;
		if (!this->encLearningEnabled_) {
			learn = true;
		}
		if ((isnan(input))) {
			input = SENTINEL_VALUE_FOR_MISSING_DATA;
		}
		if (input == SENTINEL_VALUE_FOR_MISSING_DATA) {
			return  vector<nupic::UInt>(this->n_, 0);
		}
		else {
			setMinAndMax(input, learn);
		}
		return ScalarEncoder2<T>::getBucketIndices(input);
	}

	template<class T>
	vector< tuple<boost::any, boost::any, vector<nupic::UInt>> > AdaptiveScalarEncoder<T>::getBucketInfoA(vector<nupic::UInt> buckets) {
		if (this->getMinVal() == 0 || this->getMaxVal() == 0) {
			vector<nupic::UInt> initialBuckets(this->getN(), 0);
			return { tuple<boost::any, boost::any, vector<nupic::UInt>>(0, 0, initialBuckets) };
		}
		else return  ScalarEncoder2<T>::getBucketInfo(buckets);
	}

}; // end namespace 
#endif // NTA_AdaptiveScalarEncoder_HPP
