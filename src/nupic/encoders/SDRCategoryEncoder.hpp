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

#ifndef NTA_sdrcategorycoder_HPP
#define NTA_sdrcategorycoder_HPP

#include <iostream>
#include <string> 
#include <vector>
#include <tuple>
#include <map> 

#include "Encoder.hpp"  
#include "DecodeResult.hpp"
#include "nupic/utils/Random.hpp" 
#include <nupic/types/Types.hpp>

using namespace std;

namespace encoders {

	/**
	* Encodes a list of discrete categories (described by strings), that aren't
	* related to each other.
	* Each  encoding is an SDR in which w out of n bits are turned on.
	* Unknown categories are encoded as a single
	* Internally we use a ScalarEncoder with a radius of 1, but since we only encode
	* integers, we never get mixture outputs.
	* The SDRCategoryEncoder uses a different method to encode categories
	*
	* @numenta.htm.java
	*/
	template<typename T>
	class SDRCategoryEncoder : public Encoder<T> {

	public:

		/***************** constructors and destructor *****************/
		SDRCategoryEncoder();
		~SDRCategoryEncoder();

		/***************** methods *****************/

		/**
		* @param n				 --	is  total bits in output
		* @param w				 -- number of bits to set in output
		* @param categoryList	 -- is a list of strings that define the categories.
		*						    If "none" then categories will automatically be added as they are encountered.
		* @param name			 -- an optional string which will become part of the description
		* @param encoderSeed	 -- The seed used for numpy's random number generator. If set to -1
		*							the generator will be initialized without a fixed seed.
		* @param forced			 -- default false, if true, skip some safety checks (for compatibility reasons)
		*
		*/
		void init(int n, int w, vector<string> categoryList, string name, int encoderSeed, bool forced);

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
		* Returns a list of EncoderResult describing the inputs for
		* each sub-field that correspond to the bucket indices passed in 'buckets'.
		* To get the associated field names for each of the values, call getScalarNames().
		*
		* @param buckets 	The list of bucket indices, one for each sub-field encoder.
		*              		These bucket indices for example may have been retrieved
		*              		from the getBucketIndices() call.
		*
		* @return A list of EncoderResult. Each EncoderResult has
		* @Override
		*/
		vector< tuple<boost::any, boost::any, vector<nupic::UInt>> > getBucketInfo(vector<nupic::UInt> buckets);

		/**
		* Return the interal _topDownMappingM matrix used for handling the
		* bucketInfo() and topDownCompute() methods. This is a matrix, one row per
		* category (bucket) where each row contains the encoded output for that
		* category.
		*
		*/
		map<nupic::UInt, vector<nupic::UInt>> getTopDownMapping();

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
		vector<double> getScalars(T input);

		/**
		* Should return the output width, in bits.
		*/
		virtual int getWidth() const override;
		
		/**
		* Returns list of registered SDRs for this encoder
		*/
		map<nupic::UInt, vector<nupic::UInt>> getSDRs();

	private:

		nupic::Random random_;

		nupic::UInt ncategories;

		map<string, nupic::UInt> categoryToIndex;

		vector<string> categories;

		map<nupic::UInt, vector<nupic::UInt> > sdrByCategory; 

		int thresholdOverlap;

		void addCategory(string category);

		// Generate a new and unique representation. Returns a numpy array
		// of shape (n,)
		vector<nupic::UInt> newRep();

		// replacement for Python sorted(self.random.sample(xrange(self.n), self.w))
		vector<nupic::UInt32> getSortedSample(int populationSize, int sampleLength);

		vector< tuple<boost::any, boost::any, vector<nupic::UInt>> > getEncoderResultsByIndex(map<nupic::UInt, vector<nupic::UInt>> topDownMapping, nupic::UInt categoryIndex);

	}; // end class SDRCategoryEncoder


	/***************** Start of Implementation ******************/
	template<class T>
	SDRCategoryEncoder<T>::SDRCategoryEncoder() {}

	template<class T>
	SDRCategoryEncoder<T>::~SDRCategoryEncoder() {}

	template<class T>
	void SDRCategoryEncoder<T>::init(int n, int w, vector<string> categoryList, string name, int encoderSeed, bool forced) {

		this->n_ = n;
		this->w_ = w;
		this->encLearningEnabled_ = true;

		// initialize the random number generators
		if (encoderSeed != -1) {
			random_ = nupic::Random(encoderSeed);
		}

		if (!forced) {
			if (n / w < 2) {
				NTA_THROW << "Number of ON bits in SDR " << w << " must be much smaller than the output width " << n ;
				//exit(-1);
			}
			if (w < 21) {
				NTA_THROW << "Number of bits in the SDR " << w << " must be greater than 2, and should be >= 21, pass forced=True to init() to override this check " ;
				//exit(-1);
			}
		}

		/*
		#Calculate average overlap of SDRs for decoding
		#Density is fraction of bits on, and it is also the
		#probability that any individual bit is on.
		*/
		double density = (double)(this->w_ / this->n_);
		double averageOverlap = w * density;

		/*
		# We can do a better job of calculating the threshold. For now, just
		# something quick and dirty, which is the midway point between average
		# and full overlap. averageOverlap is always < w,  so the threshold
		# is always < w.
		*/
		thresholdOverlap = (int)(averageOverlap + this->w_) / 2;

		/*
		#  1.25 -- too sensitive for decode test, so make it less sensitive
		*/
		if (thresholdOverlap < this->w_ - 3) {
			thresholdOverlap = this->w_ - 3;
		}

		this->description_.push_back(make_tuple(name, 0));
		this->name_ = name;
		ncategories = 0;

		/*
		# Always include an 'unknown' category for
		# edge cases
		*/
		addCategory("<UNKNOWN>");

		if (categoryList.empty() || categoryList.size() == 0) {
			this->encLearningEnabled_ = true;
		}
		else {
			this->encLearningEnabled_ = false;
			for (string category : categoryList) {
				addCategory(category);
			}
		}
	}

	template<class T>
	void SDRCategoryEncoder<T>::addCategory(string category) {

		//check if categories contains a certain category
		if (find(categories.begin(), categories.end(), category) != categories.end()) {
			throw std::invalid_argument("Attempt to add encoder category  that already exists");
		}

		sdrByCategory.insert(make_pair(ncategories, newRep()));
		categories.push_back(category);
		categoryToIndex.insert(make_pair(category, ncategories));
		ncategories += 1;

		//reset topDown mapping
		this->topDownMapping_ = map<nupic::UInt, vector<nupic::UInt>>();
	}

	template<class T>
	vector<nupic::UInt> SDRCategoryEncoder<T>::newRep() {
		int maxAttempts = 1000;
		bool foundUnique = true;
		vector<nupic::UInt32> oneBits;
		vector<nupic::UInt> sdr(this->n_, 0);

		for (int index = 0; index < maxAttempts; index++) {
			foundUnique = true;
			oneBits = getSortedSample(this->n_, this->w_);
			//sdr.resize(n_, 0);

			for (size_t i = 0; i < oneBits.size(); i++) {
				nupic::UInt32 oneBitInx = oneBits[i];
				sdr[oneBitInx] = 1;
			}
			for (auto &it : sdrByCategory) {
				vector<nupic::UInt> existingSdr = it.second;
				if (equal(sdr.begin(), sdr.end(), existingSdr.begin())) {
					foundUnique = false;
					break;
				}
			}
			if (foundUnique) {
				break;
			}
		}
		if (!foundUnique) {
			NTA_THROW << "Error, could not find unique pattern " << sdrByCategory.size() << " after " << maxAttempts << " attempts" ;
			//exit(-1);
		}
		return sdr;
	}

	template<class T>
	vector<nupic::UInt32> SDRCategoryEncoder<T>::getSortedSample(int populationSize, int sampleLength) {

		//  set container object without duplicate radom values
		set<nupic::UInt32> resultSet;
		while (resultSet.size() < ((size_t)sampleLength) ) {
			resultSet.insert(random_.getUInt32(populationSize)); 
		}

		vector<nupic::UInt32> result(resultSet.begin(), resultSet.end());
		sort(result.begin(), result.end());
		return result;
	}

	template<class T>
	void SDRCategoryEncoder<T>::encodeIntoArray(T inputData, vector<nupic::UInt>& output) {
		nupic::UInt index;
		if (boost::any_cast<string>(inputData) == "" || inputData.empty()) {
			fill(output.begin(), output.end(), 0);
			index = 0;
		}
		else {
			index = getBucketIndices(boost::any_cast<string>(inputData))[0];
			output = sdrByCategory[index];
		}
	}

	template<class T>
	vector<nupic::UInt> SDRCategoryEncoder<T>::getBucketIndices(string input) {
		return { (nupic::UInt) getScalars(input)[0] };
	}

	template<class T>
	vector<double> SDRCategoryEncoder<T>::getScalars(T input) {

		int index = 0;
		vector<double> result;

		if (boost::any_cast<string>(input) == "" || input.empty()) return { 0 };

		if (categoryToIndex.find(boost::any_cast<string>(input)) == categoryToIndex.end())
		{
			if (this->encLearningEnabled_) {
				addCategory(boost::any_cast<string>(input));
				index = ncategories - 1;
			}
		}
		else {			
			// in nupic python: if not found, we encode category 0 => index = 0;
			index = categoryToIndex[boost::any_cast<string>(input)];
		}

		return { (double) index };
	}

	template<class T>
	DecodeResult SDRCategoryEncoder<T>::decode(vector<nupic::UInt> encoded, string parentFieldName) {

		// python: assert (encoded[0:self.n] <= 1.0).all()
		assert(all_of(encoded.begin(), encoded.end(), [](nupic::UInt i) {return i <= 1; }));

		// python: overlaps =  (self.sdrs * encoded[0:self.n]).sum(axis=1)
		vector<nupic::UInt> overlap(sdrByCategory.size(), 0);

		for (unsigned int i = 0; i < sdrByCategory.size(); i++) {
			vector<nupic::UInt> sdr = sdrByCategory[i];
			for (unsigned int j = 0; j < sdr.size(); j++) {
				if (sdr[j] == encoded[j] && encoded[j] == 1) {
					overlap[i]++;
				}
			}
		}
		// print "Overlaps for decoding:"
		/*if (verbosity >= 2) {
		int inx = 0;
		for (string category : categories) {
		cout << overlap[inx] << " " << category << endl;
		inx++;
		}
		}*/

		//matchingCategories =  (overlaps > self.thresholdOverlap).nonzero()[0]
		vector<int> matchingCategories;
		for (unsigned int i = 0; i < overlap.size(); i++) {
			if (overlap[i] > (unsigned int)thresholdOverlap) {
				matchingCategories.push_back(i);
			}
		}

		stringstream resultString;
		vector<pair<double, double>> resultRanges;
		string fieldName;
		for (int index : matchingCategories) {
			if (resultString.str().size() != 0) {
				resultString << " ";
			}
			resultString << categories[index];
			resultRanges.push_back(make_pair(index, index));
		}

		if (parentFieldName == "" || parentFieldName.empty()) {
			fieldName = this->getName();
		}
		else {
			fieldName = parentFieldName + "." + this->getName();
		}
		
#if (_MSC_VER > 1800)	// MSVC2015...
		map<string, RangeList>  fieldsDict = { make_pair(fieldName, RangeList(resultRanges, resultString.str())) };
		vector<string> listFieldName = { fieldName };
#else
		map<string, RangeList>  fieldsDict;
		fieldsDict.insert(make_pair(fieldName, RangeList(resultRanges, resultString.str())));
		vector<string> listFieldName;
		listFieldName.push_back(fieldName);
#endif

		return DecodeResult(fieldsDict, listFieldName);
	}

	template<class T>
	vector< tuple<boost::any, boost::any, vector<nupic::UInt>> > SDRCategoryEncoder<T>::topDownCompute(vector<nupic::UInt> encoded) {

		if (ncategories == 0) {
			return  vector< tuple<boost::any, boost::any, vector<nupic::UInt>> >();
		}
		//Get generate the topDown mapping table
		map<nupic::UInt, vector<nupic::UInt>> topDownMapping = getTopDownMapping();

		//TODO the rightVecProd method belongs to SparseBinaryMatrix in Nupic Core, In python this method call stack: topDownCompute [sdrcategory.py:317]/rightVecProd [math.py:4474] -->return _math._SparseMatrix32_rightVecProd(self, *args)
		int categoryIndex = Utils<int>::argmax(this->rightVecProd(topDownMapping, encoded));
		return getEncoderResultsByIndex(getTopDownMapping(), static_cast<nupic::UInt>(categoryIndex));
	}

	template<class T>
	vector< tuple<boost::any, boost::any, vector<nupic::UInt>> > SDRCategoryEncoder<T>::getEncoderResultsByIndex(map<nupic::UInt, vector<nupic::UInt> > topDownMapping, nupic::UInt categoryIndex) {
	 
		string category = categories[categoryIndex];
		vector<nupic::UInt> encoding = topDownMapping[categoryIndex];
 
#if (_MSC_VER > 1800)	// MSVC2015...
		return { make_tuple(category, categoryIndex, encoding) };
#else
		vector< tuple<boost::any, boost::any, vector<nupic::UInt>> > result;
		result.push_back(make_tuple(category, categoryIndex, encoding));

		return result;
#endif
	}

	template<class T>
	vector< tuple<boost::any, boost::any, vector<nupic::UInt>> > SDRCategoryEncoder<T>::getBucketInfo(vector<nupic::UInt> buckets) {
		if (ncategories == 0) {
			return  vector< tuple<boost::any, boost::any, vector<nupic::UInt>> >();
		}
		int categoryIndex = buckets[0];
		return getEncoderResultsByIndex(getTopDownMapping(), static_cast<nupic::UInt>(categoryIndex));
	}

	template<class T>
	map<nupic::UInt, vector<nupic::UInt>> SDRCategoryEncoder<T>::getTopDownMapping() {
		if (this->topDownMapping_.empty()) {
			vector<nupic::UInt> outputSpace(this->getN(), 0);

			for (unsigned int i = 0; i < ncategories; i++) {
				SDRCategoryEncoder<T>::encodeIntoArray(categories[i], outputSpace);
				this->topDownMapping_.insert(pair<nupic::UInt, vector<nupic::UInt> >(i, outputSpace));  
			}
		}

		return this->topDownMapping_; 
	}

	template<class T>
	int SDRCategoryEncoder<T>::getWidth() const{
		return this->getN();
	}

	template<class T>
	map<nupic::UInt, vector<nupic::UInt>> SDRCategoryEncoder<T>::getSDRs() {
		return sdrByCategory;
	}

}; // end namespace 
#endif // NTA_sdrcategorycoder_HPP
