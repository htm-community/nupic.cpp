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

#ifndef NTA_passthroughencoder_HPP
#define NTA_passthroughencoder_HPP

#include <algorithm>
#include <iostream>
#include <string> 
#include <vector>
#include <tuple>
#include <map> 

#include <nupic/encoders/Encoder.hpp>  
#include <nupic/encoders/Utils.hpp>
#include <nupic/encoders/RangeList.hpp>
#include <nupic/encoders/DecodeResult.hpp>  

using namespace std;

namespace encoders {

	/**
	* Pass an encoded SDR straight to the model.
	* Each encoding is an SDR in which w out of n bits are turned on.
	*
	* @author wilsondy (from Python original)
	*/
	template<typename T>
	class PassThroughEncoder : public Encoder<T> {

	public:

		/***************** constructors and destructor *****************/
		PassThroughEncoder(); 
		~PassThroughEncoder();

		/***************** methods *****************/

		/**		
		* @param n				 -- is the total #bits in output
		* @param w				 -- is used to normalize the sparsity of the output, exactly w bits ON,
		*						    if 1 (default) - do not alter the input, just pass it further.
		* @param name			 -- an optional string which will become part of the description
		*
		* @Override
		*/
		void init( int n, int w=1, string name="PassThru");

		/**
		* Check for length the same and copy input into output
		* If outputBitsOnCount (w) set, throw error if not true
		*
		* @param inputData		Data to encode. This should be validated by the encoder.
		* @param output			1-D array of same length returned by getWidth()
		*
		* @Override
		*/
		virtual void encodeIntoArray(T inputData, vector<nupic::UInt> &output) override;

		/**
		* Returns a RangeLists which is a list of range 
		* minimum and maximum objects in the first entry 
		* and descriptions for each range in the second entry,  
		* and the field name in the thrid entry
		*
		* @param encoded			the encoded bit vector
		* @param parentFieldName	the field the vector corresponds with
		* @return
		* @Override
		*/
		virtual DecodeResult decode(vector<nupic::UInt> encoded, string parentFieldName) override;

		/**
		* Does a bitwise compare of the two bitmaps and returns a fractional
		* value between 0 and 1 of how similar they are.
		* 1 => identical
		* 0 => no overlapping bits
		* IGNORES difference in length (only compares bits of shorter list)  e..g 11 and 1100101010 are "identical"
		* @see org.numenta.nupic.encoders.Encoder#closenessScores(gnu.trove.list.TDoubleList, gnu.trove.list.TDoubleList, boolean)
		*/
		vector<double> closenessScores(vector<double> expValues, vector<double> actValues, bool fractional); 

		/**
		* Should return the output width, in bits.
		* @Override
		*/
		virtual int getWidth() const override;
				
		virtual vector< tuple<boost::any, boost::any, vector<nupic::UInt>> > topDownCompute(vector<nupic::UInt> encoded) override;

	}; // end class PassThroughEncoder

	/***************** Start of Implementation ******************/
	template<class T>
	PassThroughEncoder<T>::PassThroughEncoder(){}

	template<class T>
	PassThroughEncoder<T>::~PassThroughEncoder() {} 

	template<class T>
	void PassThroughEncoder<T>::init( int n, int w, string name) {
		this->setW(w);
		this->setN(n);
		this->setName(name);
		this->setForced(false);	
	}

	template<class T>
	void PassThroughEncoder<T>::encodeIntoArray(T inputData, vector<nupic::UInt>& output) {
          NTA_ASSERT(inputData.size()==output.size()); 
          if(this->getW()>1) { //w=1 ignores the check
            NTA_ASSERT(/*sum*/  std::accumulate(inputData.begin(), inputData.end(), 0)  == (int)this->getW());
          }
		std::copy(begin(inputData), end(inputData), std::back_inserter(output)); //fill output with the input's content
	} 

	template<class T>
	DecodeResult PassThroughEncoder<T>::decode(vector<nupic::UInt> encoded, string parentFieldName){
		string fieldName;
		if (parentFieldName != "" && !parentFieldName.empty()) {
			fieldName =  parentFieldName + "."+ this->getName();
		}
		else {
			fieldName = this->getName();
		}

		//Not much real work to do here as this concept doesn't really apply.

#if (_MSC_VER > 1800)	// MSVC2015...
		// pair of <Min,Max>
		vector<pair<double, double>> ranges = { make_pair(0,0) };
		map<string, RangeList>  fieldsDict = { make_pair(fieldName, RangeList(ranges, "input")) };
		vector<string> listFieldName = { fieldName };
#else
		vector<pair<double, double>> ranges;
		ranges.push_back(make_pair(0, 0));		
		map<string, RangeList>  fieldsDict;
		fieldsDict.insert(make_pair(fieldName, RangeList(ranges, "input")));		
		vector<string> listFieldName;
		listFieldName.push_back(fieldName);
#endif

		return DecodeResult(fieldsDict, listFieldName);
	}

	template<class T>
	vector<double> PassThroughEncoder<T>::closenessScores(vector<double> expValues, vector<double> actValues, bool fractional){
		
        double ratio = 1.0;
        double expectedSum = Utils<double>::sum(expValues);							 
        double actualSum = Utils<double>::sum(actValues);

        if (actualSum > expectedSum) {
            double diff = actualSum - expectedSum;
            if (diff < expectedSum){
                ratio = 1 - diff / expectedSum;
			}else{
                ratio = 1 / diff;
			}
        }

        vector<unsigned int> expectedInts = Utils<unsigned int>::toIntArray(expValues);			 
        vector<unsigned int> actualInts = Utils<unsigned int>::toIntArray(actValues);

        vector<unsigned int> overlap =  Utils<unsigned int>::and_(expectedInts, actualInts);

        int overlapSum =  Utils<unsigned int>::sum(overlap);
        double r = 0.0;
        
		if (expectedSum != 0)  r = overlapSum / expectedSum;
		
        r = r * ratio;
		 
        return {r};
	}

	template<class T>
	int PassThroughEncoder<T>::getWidth() const{
		return this->getN();
	}

	template<class T>
	vector< tuple<boost::any, boost::any, vector<nupic::UInt>> > PassThroughEncoder<T>::topDownCompute(vector<nupic::UInt> encoded) {
		return vector< tuple<boost::any, boost::any, vector<nupic::UInt>> >();
	}

}; // end namespace 
#endif // NTA_passthroughencoder_HPP
