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

#ifndef NTA_encoder_HPP
#define NTA_encoder_HPP

//STL includes
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <tuple>
#include <map>
#include <list>
#include <cmath>

#include "Utils.hpp" 
#include "DecodeResult.hpp" 
#include "RangeList.hpp" 

#include <boost/any.hpp>
#include <nupic/utils/Log.hpp> 

using namespace std;
using namespace nupic;

namespace encoders {	 

	// define NAN for VS2012
#ifndef _HUGE_ENUF
#define _HUGE_ENUF  1e+300	/* _HUGE_ENUF*_HUGE_ENUF must overflow */
#endif /* _HUGE_ENUF */

#ifndef INFINITY	
#define INFINITY   ((float)(_HUGE_ENUF * _HUGE_ENUF))  /* causes warning C4756: overflow in constant arithmetic (by design) */
#endif /* INFINITY */

#ifndef NAN
#define NAN        ((float)(INFINITY * 0.0F))
#endif /* NAN */

	/** Value used to represent no data */
	static double SENTINEL_VALUE_FOR_MISSING_DATA = NAN;

	template<typename T>										 						
	T round(T num) {
		return (num > 0.0) ? floor(num + 0.5) : ceil(num - 0.5);
	}


	/**
	* <pre>
	* An encoder takes a value and encodes it with a partial sparse representation
	* of bits.  The Encoder superclass implements:
	* - encode() - returns an array encoding the input; syntactic sugar
	*   on top of encodeIntoArray. If pprint, prints the encoding to the terminal
	* - pprintHeader() -- prints a header describing the encoding to the terminal
	* - pprint() -- prints an encoding to the terminal
	*
	* Methods/properties that must be implemented by subclasses:
	* - getDecoderOutputFieldTypes()   --  must be implemented by leaf encoders; returns
	*                                      [`nupic.data.fieldmeta.FieldMetaType.XXXXX`]
	*                                      (e.g., [nupic.data.fieldmetaFieldMetaType.float])
	* - getWidth()                     --  returns the output width, in bits
	* - encodeIntoArray()              --  encodes input and puts the encoded value into the output array,
	*                                      which is a 1-D array of length returned by getWidth()
	* - getDescription()               --  returns a list of (name, offset) pairs describing the
	*                                      encoded output
	* </pre>
	*
	* <P>
	* Typical usage is as follows:
	* <PRE>
	* CategoryEncoder.Builder builder =  ((CategoryEncoder.Builder)CategoryEncoder.builder())
	*      .w(3)
	*      .radius(0.0)
	*      .minVal(0.0)
	*      .maxVal(8.0)
	*      .periodic(false)
	*      .forced(true);
	*
	* CategoryEncoder encoder = builder.build();
	*
	* <b>Above values are <i>not</i> an example of "sane" values.</b>
	*
	* </PRE>
	*/ 
	template<class T>
	class Encoder {
	public:

		/***************** constructors and destructor *****************/
		Encoder(int w =0, int n =0);
		virtual ~Encoder();

		/***************** methods *****************/

		/**
		* Convenience wrapper for {@link #encodeIntoArray(double, int[])}
		*
		* @param inputData		the input scalar
		*
		* @return	an array with the encoded representation of inputData
		*/
		vector<nupic::UInt> encode(T inputData);

		/**
		* Returns an array containing the sum of the right
		* applied multiplications of each slice to the array passed in.
		*
		* @param encoded
		* @return
		*/
		vector<int> rightVecProd(map<nupic::UInt, vector<nupic::UInt>> matrix, vector<nupic::UInt> encoded);

		/**
		* Encodes inputData and puts the encoded value into the output array,
		* which is a 1-D array of length returned by {@link #getW()}.
		*
		* Note: The output array is reused, so clear it before updating it.
		*
		* @param inputData		Data to encode. This should be validated by the encoder.
		* @param output			1-D array of same length returned by {@link #getW()}
		*
		* @return
		*/
		virtual void encodeIntoArray(T inputData, vector<nupic::UInt>& output) = 0;

		virtual DecodeResult decode(vector<nupic::UInt> encoded, std::string parentFieldName) = 0;

		virtual vector< tuple<boost::any, boost::any, vector<nupic::UInt>> > topDownCompute(vector<nupic::UInt> encoded) = 0;

		////////////////

		/**
		* The number of bits in the output. Must be greater than or equal to w
		* @param n
		*/
		void setN(int n) { n_ = n; };

		/**
		* Sets the "w" or width of the output signal
		* <em>Restriction:</em> w must be odd to avoid centering problems.
		* @param w
		*/
		void setW(int w) { w_ = w; };

		/**
		* If true, skip some safety checks (for compatibility reasons), default false
		* @param b
		*/
		void setForced(bool b) { forced_ = b; };

		/**
		* If true, non-periodic inputs smaller than minval or greater
		* than maxval will be clipped to minval/maxval
		* @param b
		*/
		void setClipInput(bool b) { clipInput_ = b; };

		/**
		* An optional string which will become part of the description
		* @param name
		*/
		virtual void setName(string name) { name_ = name; };

		/**
		* Half the width
		* @param hw
		*/
		void setHalfWidth(int hw) { halfWidth_ = hw; };

		/**
		* For non-periodic inputs, padding is the number of bits "outside" the range,
		* on each side. I.e. the representation of minval is centered on some bit, and
		* there are "padding" bits to the left of that centered bit; similarly with
		* bits to the right of the center bit of maxval
		*
		* @param padding
		*/
		void setPadding(int padding) { padding_ = padding; };

		/**
		* inputs separated by more than, or equal to this distance will have non-overlapping
		* representations
		*
		* @param radius
		*/
		void setRadius(double radius) { radius_ = radius; };

		/**
		* Sets the range
		* @param range
		*/
		void setRange(double range) { range_ = range; };

		/**
		* nInternal represents the output area excluding the possible padding on each side
		*
		* @param n
		*/
		void setNInternal(int n) { nInternal_ = n; };

		/**
		* If true, then the input value "wraps around" such that minval = maxval
		* For a periodic value, the input must be strictly less than maxval,
		* otherwise maxval is a true upper bound.
		*
		* @param b
		*/
		void setPeriodic(bool b) { periodic_ = b; };

		/**
		* Sets rangeInternal
		* @param r
		*/
		void setRangeInternal(double r) { rangeInternal_ = r; };

		/**
		* The minimum value of the input signal.
		* @param minVal
		*/
		void setMinVal(double minVal) { minVal_ = minVal; };

		/**
		* The maximum value of the input signal.
		* @param maxVal
		*/
		void setMaxVal(double maxVal) { maxVal_ = maxVal; };

		/**
		* inputs separated by more than, or equal to this distance will have different
		* representations
		*
		* @param resolution
		*/
		void setResolution(double resolution) { resolution_ = resolution; };

		/**
		* Range of values.
		* @param values
		*/
		void setTopDownValues(vector<double> values) { topDownValues_ = values; };

		/**
		* This matrix is used for the topDownCompute. We build it the first time
		* topDownCompute is called
		*
		* @param sm
		*/
		void setTopDownMapping(map<nupic::UInt, vector<nupic::UInt>> sm) { topDownMapping_ = sm; };

		/**
		* Sets the encoder flag indicating whether learning is enabled.
		*
		* @param	encLearningEnabled	true if learning is enabled, false if not
		*/
		void setLearningEnabled(bool encLearningEnabled) { encLearningEnabled_ = encLearningEnabled; };

		/**
		* Set whether learning is enabled.
		* @param 	learningEnabled		flag indicating whether learning is enabled
		*/
		void setLearning(bool learningEnabled) { setLearningEnabled(learningEnabled); };
		 
		/**
		* Returns w_
		* @return
		*/
		int getW() const { return w_; };

		/**
		* Returns n_
		* @return
		*/
		int getN() const { return n_; };

		/**
		* For non-periodic inputs, padding is the number of bits "outside" the range,
		* on each side. I.e. the representation of minval is centered on some bit, and
		* there are "padding" bits to the left of that centered bit; similarly with
		* bits to the right of the center bit of maxval
		*
		* @return
		*/
		int getPadding() const { return padding_; };

		/**
		* Return the half width value.
		* @return
		*/
		int getHalfWidth() const { return halfWidth_; };

		/**
		* nInternal represents the output area excluding the possible padding on each
		* side
		* @return
		*/
		int getNInternal() const { return nInternal_; };

		/**
		* Returns the optional name
		* @return
		*/
		string getName() const { return name_; };

		/**
		* Returns the range internal value
		* @return
		*/
		double getRange() const { return range_; };

		/**
		* Returns the resolution
		* @return
		*/
		double getResolution() const { return resolution_; };

		/**
		* Returns minval
		* @return
		*/
		double getMinVal() const { return minVal_; };

		/**
		* Returns maxval
		* @return
		*/
		double getMaxVal() const { return maxVal_; };

		/**
		* Returns the radius
		* @return
		*/
		double getRadius() const { return radius_; };

		/**
		* Returns the range internal value
		* @return
		*/
		double getRangeInternal() const { return rangeInternal_; };

		/**
		* Returns the top down range of values
		* @return
		*/
		vector<double> getTopDownValues() const { return topDownValues_; };

		/**
		* This returns a list of tuples, each containing (name, offset).
		* The 'name' is a string description of each sub-field, and offset is the bit
		* offset of the sub-field for that encoder.
		*
		* For now, only the 'multi' and 'date' encoders have multiple (name, offset)
		* pairs. All other encoders have a single pair, where the offset is 0.
		*
		* @return		list of tuples, each containing (name, offset)
		*/
		virtual vector<tuple<string, int> > getDescription() const { return description_; };

		/**
		* Returns the periodic flag
		* @return
		*/
		bool isPeriodic() const { return periodic_; };

		/**
		* Returns the forced flag
		* @return
		*/
		bool isForced() const { return forced_; };

		/**
		* Returns the clip input flag
		* @return
		*/
		bool clipInput() const { return clipInput_; };
		 
		/**
		* Should return the output width, in bits.
		* @Override
		*/
		virtual int getWidth() const = 0;

	protected:

		/** The number of bits that are set to encode a single value - the
		* "width" of the output signal
		*/
		int w_;

		/** number of bits in the representation (must be >= w) */
		int n_;

		/** the half width value */
		int halfWidth_;

		/**For non-periodic inputs, padding is the number of bits "outside" the range,
		* on each side */
		int padding_;

		/** nInternal represents the output area excluding the possible padding on each side */
		int nInternal_;

		/**
		* If true, then the input value "wraps around" such that minval = maxval
		* For a periodic value, the input must be strictly less than maxval,
		* otherwise maxval is a true upper bound.
		*/
		bool periodic_;

		/** if true, non-periodic inputs smaller than minval or greater
		than maxval will be clipped to minval/maxval */
		bool clipInput_;

		/** if true, skip some safety checks (for compatibility reasons), default false */
		bool forced_;

		/** the encoder flag indicating whether learning is enabled. */
		bool encLearningEnabled_;

		/** inputs separated by more than, or equal to this distance will have different representations */
		double resolution_;

		/**
		* inputs separated by more than, or equal to this distance will have non-overlapping
		* representations
		*/
		double radius_;

		/** The minimum value of the input signal.  */
		double minVal_;

		/** The maximum value of the input signal. */
		double maxVal_;

		/** The range internal value */
		double rangeInternal_;

		double range_;

		/** Encoder name - an optional string which will become part of the description */
		string name_;

		/** The names of the fields */
		list<string> scalarName_;

		/** List of tuples, each containing (name, offset) */
		vector<tuple<string, int> > description_;

		/** the top down range of values */
		vector<double> topDownValues_;

		/**
		* This matrix is used for the topDownCompute. We build it the first time
		* topDownCompute is called
		*/
		map<nupic::UInt,vector<nupic::UInt>> topDownMapping_;

		vector<T> bucketValues;
		 
	}; // end class Encoder


	/***************** Start of Implementation ******************/
	template<class T>
	Encoder<T>::Encoder(int w, int n) :
		w_(w), n_(n), periodic_(true), resolution_(0), radius_(0), minVal_(0), maxVal_(0), name_("") {
	}

	template<class T>
	Encoder<T>::~Encoder() {}
 
	template<class T>
	vector<nupic::UInt> Encoder<T>::encode(T inputData) {
		vector<nupic::UInt> output(getN(), 0);

		encodeIntoArray(inputData, output);
		return output;
	}

	template<class T>
	vector<int> Encoder<T>::rightVecProd(map<nupic::UInt, vector<nupic::UInt>> matrix, vector<nupic::UInt> encoded){
		// get MaxIndex in matrix
		UInt matrix_maxIndex = 0;
		for (map<nupic::UInt, vector<nupic::UInt>>::const_iterator it = matrix.begin(); it != matrix.end(); ++it) {
			if (it->first > matrix_maxIndex) {
				matrix_maxIndex = it->first;
			}			
		}

		vector<int> retVal;
		retVal.reserve(matrix_maxIndex + 1);
		for (size_t i = 0; i < matrix_maxIndex + 1; i++) {
			vector<nupic::UInt> slice = matrix.find(i)->second;
			int tmp = 0;
			for (size_t j = 0; j < slice.size(); j++)
				tmp += (slice[j] * encoded[j]);				
			retVal.push_back(tmp);
		}
		return retVal;
	}
	
}; // end namespace Encoder
#endif // NTA_Encoder_HPP
