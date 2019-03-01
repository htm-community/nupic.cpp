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

#ifndef NTA_categorycoder_HPP
#define NTA_categorycoder_HPP

#include <iostream>
#include <string> 
#include <vector>
#include <tuple>
#include <map> 

#include "Encoder.hpp"  
#include "RangeList.hpp"  
#include "ScalarEncoder2.hpp" 

using namespace std;

namespace encoders {

	/**
	* Encodes a list of discrete categories (described by strings), that aren't
	* related to each other, so we never emit a mixture of categories.
	*
	* The value of zero is reserved for "unknown category"
	*
	* Internally we use a ScalarEncoder with a radius of 1, but since we only encode
	* integers, we never get mixture outputs.
	*
	* The SDRCategoryEncoder (not yet implemented in Java) uses a different method to encode categories
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
	*
	*/
	template<typename T>
	class CategoryEncoder : public Encoder<T> { 

	public:

		/***************** constructors and destructor *****************/
		CategoryEncoder(); 
		~CategoryEncoder();

		/***************** methods *****************/

		/**
		* @param w				 -- number of bits to set in output
		* @param radius			 -- inputs separated by more than, or equal to this distance will have non-overlapping  representations					
		* @param periodic		 -- If true, then the input value "wraps around" such that minval = maxval
		*							For a periodic value, the input must be strictly less than maxval,
		*							otherwise maxval is a true upper bound.
		* @param forced			 -- default false, if true, skip some safety checks (for compatibility reasons)
		*
		*/
		void init( int w, int radius, bool periodic, bool forced);

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
		* Returns a list of EncoderResult describing the inputs for
		* each sub-field that correspond to the bucket indices passed in 'buckets'.
		* To get the associated field names for each of the values, call getScalarNames().
		*
		* @param buckets 	The list of bucket indices, one for each sub-field encoder.
		*              		These bucket indices for example may have been retrieved
		*              		from the getBucketIndices() call.
		*
		* @return A list of EncoderResult . Each EncoderResult has
		* @Override
		*/	
		vector< tuple<boost::any, boost::any, vector<nupic::UInt>> > getBucketInfo(vector<nupic::UInt> buckets);

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
		* Returns a list of items, one for each bucket defined by this encoder.
		* Each item is the value assigned to that bucket, this is the same as the
		* EncoderResult.value that would be returned by getBucketInfo() for that
		* bucket and is in the same format as the input that would be passed to
		* encode().
		*
		* This call is faster than calling getBucketInfo() on each bucket individually
		* if all you need are the bucket values.
		*
		* @param	returnType 		class type parameter so that this method can return encoder
		* 							specific value types
		*
		* @return list of items, each item representing the bucket value for that
		*        bucket.
		*/
		vector<T> getBucketValues();
		 
		/**
		* Returns an array containing the sub-field bucket indices for
		* each sub-field of the inputData. To get the associated field names for each of
		* the buckets, call getScalarNames().
		* @param  	input 	The data from the source. This is typically a object with members.
		*
		* @return 	array of bucket indices
		*/
		vector<nupic::UInt> getBucketIndices(string input);
		  
		/**
		* Returns an vector<double> containing the sub-field scalar value(s) for
		* each sub-field of the inputData. To get the associated field names for each of
		* the scalar values, call getScalarNames().
		*
		* For a simple scalar encoder, the scalar value is simply the input unmodified.
		* For category encoders, it is the scalar representing the category string
		* that is passed in.
		*
		* TODO This is not correct for DateEncoder:
		*
		* For the datetime encoder, the scalar value is the
		* the number of seconds since epoch.
		*
		* The intent of the scalar representation of a sub-field is to provide a
		* baseline for measuring error differences. You can compare the scalar value
		* of the inputData with the scalar value returned from topDownCompute() on a
		* top-down representation to evaluate prediction accuracy, for example.
		*
		* @param input  the specifically typed input object
		*
		* @return
		*/
		vector<double> getScalars(T d);

		/**
		* Should return the output width, in bits.
		*/
		virtual int getWidth() const override;

		/**
		* Should return the category list.
		*/
		vector<string> getCategoryList();

		/**
		* Set CategoryEncoder specific field
		*/
		void setCategoryList(vector<string> categoryList);


	protected:

		int ncategories;

		map<string, nupic::UInt> categoryToIndex;

		map<nupic::UInt, string> indexToCategory; 

		vector<string> categoryList_;

		int width;

		ScalarEncoder2<double> scalarEncoder;

	}; // end class CategoryEncoder

	/***************** Start of Implementation ******************/
	template<class T>
	CategoryEncoder<T>::CategoryEncoder(){}

	template<class T>
	CategoryEncoder<T>::~CategoryEncoder() {} 

	template<class T>
	void CategoryEncoder<T>::init( int w, int radius, bool periodic, bool forced) {

		this->setW(w);
		this->setRadius(radius);
		this->setPeriodic(periodic);
		this->setForced(forced);	

		ncategories = categoryList_.empty() ? 0 : categoryList_.size() + 1;
		this->name_ = "category";
		this->minVal_ = 0;
		this->maxVal_ = ncategories - 1;

		scalarEncoder.init( this->w_ , this->n_ , this->minVal_,  this->maxVal_, this->name_, this->radius_, this->resolution_, this->periodic_, this->forced_); 

		indexToCategory[0] = "<UNKNOWN>";
		if( !categoryList_.empty() && categoryList_.size() != 0) {
			int len = categoryList_.size();
			for(int i = 0;i < len;i++) {
				categoryToIndex.insert(make_pair(categoryList_[i], static_cast<nupic::UInt>( i + 1 )));
				indexToCategory.insert(make_pair(static_cast<nupic::UInt>(i + 1), categoryList_[i]));
			}
		}
		width = this->n_ = this->w_ * ncategories;

		if(getWidth() != width) {
			NTA_THROW << "Width != w (num bits to represent output item) * #categories";
			//exit(-1);
		}

		this->description_.push_back(make_tuple( this->name_, 0));
	}

	template<class T>
	void CategoryEncoder<T>::encodeIntoArray(T inputData, vector<nupic::UInt>& output) {
		string val = "";
		double value = 0;
		if(boost::any_cast<string>(inputData) == "") {
			val = "<missing>";
		}else{
			value = categoryToIndex[boost::any_cast<string>(inputData)]; 
			scalarEncoder.encodeIntoArray(value, output);
		}
	} 

	template<class T>
	DecodeResult CategoryEncoder<T>::decode(vector<nupic::UInt> encoded, string parentFieldName){

		// Get the scalar values from the underlying scalar encoder
		DecodeResult result = scalarEncoder.decode(encoded, parentFieldName);

		if(result.getFields().size() == 0) {
			return result;
		}

		// Expect only 1 field
		if(result.getFields().size() != 1) {
			NTA_THROW << "Expecting only one field";
			//exit(-1);
		}

		// Get the list of categories the scalar values correspond to and
		// generate the description from the category name(s).
		map<string, RangeList> fieldRanges = result.getFields();
		vector<pair<double, double>> outRanges;
		stringstream  desc;

		for (auto& descripStr : fieldRanges){ 
			pair< double, double> minMax = fieldRanges[descripStr.first].getRange(0); 
			int minV = (int) round(minMax.first);
			int maxV = (int) round(minMax.second);
			outRanges.push_back(make_pair(minV, maxV));

			while(minV <= maxV) {
				if(desc.str().size() > 0) {
					desc << ", ";
				}
				desc << indexToCategory[minV];
				minV += 1;
			}
		}	

		//Return result
		string fieldName;
		if(!parentFieldName.empty()) {
			fieldName =  parentFieldName + "." + this->getName();
		}
		else{
			fieldName = this->getName();
		}
		 
#if (_MSC_VER > 1800)	// MSVC2015...
		map<string, RangeList>  fieldsDict = { make_pair(fieldName, RangeList(outRanges, desc.str())) };
		vector<string> listFieldName = { fieldName };
#else
		map<string, RangeList>  fieldsDict;
		fieldsDict.insert(make_pair(fieldName, RangeList(outRanges, desc.str())));
		vector<string> listFieldName;
		listFieldName.push_back(fieldName);
#endif

		return DecodeResult(fieldsDict, listFieldName);
	}

	template<class T>
	vector<double> CategoryEncoder<T>::closenessScores(vector<double> expValues, vector<double> actValues, bool fractional){
		double expValue = expValues[0];
		double actValue = actValues[0];

		double closeness = expValue == actValue ? 1.0 : 0;
		if(!fractional) closeness = 1.0 - closeness;
		 
		return { closeness };			 
	}

	template<class T>
	vector< tuple<boost::any, boost::any, vector<nupic::UInt>> > CategoryEncoder<T>::getBucketInfo(vector<nupic::UInt> buckets){
		vector< tuple<boost::any, boost::any, vector<nupic::UInt>> > bucketTemp= scalarEncoder.getBucketInfo(buckets);

		double bucket_value				=  boost::any_cast<double>(get<0>(bucketTemp[0]));
		vector<nupic::UInt> bucket_vec	=  get<2>(bucketTemp[0]); 

		int categoryIndex = (int) round(bucket_value);
		string category = indexToCategory[categoryIndex];

#if (_MSC_VER > 1800)	// MSVC2015...
		return{ make_tuple(category, static_cast<nupic::UInt>(categoryIndex), bucket_vec) };
#else
		vector< tuple<boost::any, boost::any, vector<nupic::UInt>> >  bucketInfo;
		bucketInfo.push_back(make_tuple(category, static_cast<nupic::UInt>(categoryIndex), bucket_vec));
		return bucketInfo;
#endif
	}

	template<class T>
	vector< tuple<boost::any, boost::any, vector<nupic::UInt>> > CategoryEncoder<T>::topDownCompute(vector<nupic::UInt> encoded) {
		//Get generate the topDown mapping table
		map<nupic::UInt, vector<nupic::UInt>> topDownMapping = scalarEncoder.getTopDownMapping();

		// See which "category" we match the closest.
		int category = Utils<int>::argmax(this->rightVecProd(topDownMapping, encoded));
		 
		return CategoryEncoder<T>::getBucketInfo({ static_cast<nupic::UInt>(category) });
	}

	template<class T>
	vector<T> CategoryEncoder<T>::getBucketValues(){ 

		if(this->bucketValues.empty()) {
			map<nupic::UInt, vector<nupic::UInt>> topDownMapping = scalarEncoder.getTopDownMapping();

			// get max index of topDownMapping
			nupic::UInt numBuckets = 0;
			for(auto it : topDownMapping){
				if (it.first > numBuckets) {
					numBuckets = it.first;
				}
			}
			numBuckets += 1; 

			for(nupic::UInt i = 0;i < numBuckets;i++) {
				vector<nupic::UInt> temp; 	temp.push_back(i);		 
				string bucket_value			= boost::any_cast<string>(get<0>(getBucketInfo(temp)[0]));
				this->bucketValues.push_back(bucket_value);
			}
		}
		return this->bucketValues;
	} 

	template<class T>
	void CategoryEncoder<T>::setCategoryList(vector<string> categoryList){
		categoryList_ = categoryList;
	}

	template<class T>
	vector<string> CategoryEncoder<T>::getCategoryList(){
		return categoryList_;
	}

	template<class T>
	vector<nupic::UInt> CategoryEncoder<T>::getBucketIndices(string input){
		if(input == "") return vector<nupic::UInt>();

		return scalarEncoder.getBucketIndices(categoryToIndex[input]);
	}

	template<class T>
	vector<double> CategoryEncoder<T>::getScalars(T d){  		
		return { (double) categoryToIndex[d] };
	}

	template<class T>
	int CategoryEncoder<T>::getWidth() const{
		return this->getN();
	}

}; // end namespace 
#endif // NTA_categorycoder_HPP
