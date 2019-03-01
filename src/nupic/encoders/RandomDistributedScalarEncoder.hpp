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

#ifndef NTA_randomdistributedscalarencoder_HPP
#define NTA_randomdistributedscalarencoder_HPP

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

#include "Encoder.hpp"
#include "nupic/utils/Random.hpp" 

using namespace std;
using namespace nupic;

namespace encoders { 

	/**
	* <p>
	* A scalar encoder encodes a numeric (floating point) value into an array of
	* bits.
	*
	* This class maps a scalar value into a random distributed representation that
	* is suitable as scalar input into the spatial pooler. The encoding scheme is
	* designed to replace a simple ScalarEncoder. It preserves the important
	* properties around overlapping representations. Unlike ScalarEncoder the min
	* and max range can be dynamically increased without any negative effects. The
	* only required parameter is resolution, which determines the resolution of
	* input values.
	*
	* Scalar values are mapped to a bucket. The class maintains a random
	* distributed encoding for each bucket. The following properties are maintained
	* by RandomDistributedEncoder:
	* </p>
	* <ol>
	* <li>Similar scalars should have high overlap. Overlap should decrease
	* smoothly as scalars become less similar. Specifically, neighboring bucket
	* indices must overlap by a linearly decreasing number of bits.
	*
	* <li>Dissimilar scalars should have very low overlap so that the SP does not
	* confuse representations. Specifically, buckets that are more than w indices
	* apart should have at most maxOverlap bits of overlap. We arbitrarily (and
	* safely) define "very low" to be 2 bits of overlap or lower.
	*
	* Properties 1 and 2 lead to the following overlap rules for buckets i and j:<br>
	*
	* <pre>
	* {@code
	* If abs(i-j) < w then:
	* 		overlap(i,j) = w - abs(i-j);
	* else:
	* 		overlap(i,j) <= maxOverlap;
	* }
	* </pre>
	*
	* <li>The representation for a scalar must not change during the lifetime of
	* the object. Specifically, as new buckets are created and the min/max range is
	* extended, the representation for previously in-range scalars and previously
	* created buckets must not change.
	* </ol>
	*
	*
	* @author Numenta
	* @author Anubhav Chaturvedi
	*/
	template<typename T>
	class RandomDistributedScalarEncoder : public Encoder<T>
	{
	public:

		/***************** constructors and destructor *****************/
		RandomDistributedScalarEncoder();
		~RandomDistributedScalarEncoder();

		/***************** methods *****************/

		/**
		* Perform the initialization of the encoder.
		*
		* @param resolution		A floating point positive number denoting the resolution
		*						of the output representation. Numbers within
		*						[offset-resolution/2, offset+resolution/2] will fall into
		*						the same bucket and thus have an identical representation.
		*						Adjacent buckets will differ in one bit. resolution is a
		*						required parameter.
		*
		* @param w				Number of bits to set in output. w must be odd to avoid centering
		*						problems.  w must be large enough that spatial pooler
		*						columns will have a sufficiently large overlap to avoid
		*						false matches. A value of w=21 is typical.
		*
		* @param n				Number of bits in the representation (must be > w). n must be
		*						large enough such that there is enough room to select
		*						new representations as the range grows. With w=21 a value
		*						of n=400 is typical. The class enforces n > 6*w.
		*
		* @param name			An optional string which will become part of the description.
		*
		* @param offset			A floating point offset used to map scalar inputs to bucket
		*						indices. The middle bucket will correspond to numbers in the
		*						range [offset - resolution/2, offset + resolution/2). If set
		*						to None, the very first input that is encoded will be used
		*						to determine the offset.
		*
		* @param seed			The seed used for numpy's random number generator. If set to -1
		*						the generator will be initialized without a fixed seed.
		*/
		void init(double resolution, int w, int n, string name, double offset, long seed);

		/**
		* Initialize the bucket map assuming the given number of maxBuckets.
		*
		* @param maxBuckets
		* @param offset
		*/
		void initializeBucketMap(int maxBuckets, double offset);

		/**
		* Create the given bucket index. Recursively create as many in-between
		* bucket indices as necessary.
		*
		* @param index	the index at which bucket needs to be created
		*/
		void createBucket(int index);

		/**
		* Get a new representation for newIndex that overlaps with the
		* representation at index by exactly w-1 bits
		*
		* @param index
		* @param newIndex
		*/
		vector<nupic::UInt32> newRepresentation(int index, int newIndex);

		/**
		* Check if this new candidate representation satisfies all our
		* overlap rules. Since we know that neighboring representations differ by
		* at most one bit, we compute running overlaps.
		*
		* @param newRep			Encoded SDR to be considered
		* @param newIndex		The index being considered
		*
		* @return {@code true} if newRep satisfies all our overlap rules
		*/
		bool newRepresentationOK(vector<nupic::UInt32> newRep, int newIndex);

		/**
		* Get the overlap between two representations. rep1 and rep2 are
		* {@link List} of non-zero indices.
		*
		* @param rep1		The first representation for overlap calculation
		* @param rep2		The second representation for overlap calculation
		*
		* @return The number of 'on' bits that overlap
		*/
		int countOverlap(vector<nupic::UInt32> rep1, vector<nupic::UInt32> rep2);

		/**
		* Check if the given overlap between bucket indices i and j are acceptable.
		*
		* @param i			The index of the bucket to be compared
		* @param j			The index of the bucket to be compared
		* @param overlap	The overlap between buckets at index i and j
		*
		* @return {@code true} if overlap is acceptable, else {@code false}
		*/
		bool overlapOK(int i, int j, int overlap);

		/**
		* Check if the overlap between the buckets at indices i and j are
		* acceptable. The overlap is calculate from the bucketMap.
		*
		* @param i		The index of the bucket to be compared
		* @param j		The index of the bucket to be compared
		*
		* @return {@code true} if the given overlap is acceptable, else {@code false}
		*/
		bool overlapOK(int i, int j);

		/**
		* Get the overlap between bucket at indices i and j
		*
		* @param i		The index of the bucket
		* @param j		The index of the bucket
		*
		* @return the overlap between bucket at indices i and j
		*/
		int countOverlapIndices(int i, int j);

		/**
		* Given a bucket index, return the list of indices of the 'on' bits. If the
		* bucket index does not exist, it is created. If the index falls outside
		* our range we clip it.
		*
		* @param index	The bucket index
		*
		* @return The list of active bits in the representation
		*/
		vector<nupic::UInt32> mapBucketIndexToNonZeroBits(int index);

		/**
		* Encodes inputData and puts the encoded value into the output array,
		* which is a 1-D array of length returned by {@link #getW()}.
		*
		* Note: The output array is reused, so clear it before updating it.
		* @param inputData		Data to encode. This should be validated by the encoder.
		* @param output			1-D array of same length returned by {@link #getW()}
		*
		* @return
		* @Override
		*/
		virtual void encodeIntoArray(T inputData, vector<nupic::UInt>& output) override;

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
		vector<int> getBucketIndices(double x);

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
		vector<tuple<std::string, int> > getDescription() const override;

		/**
		* @return the seed for the random number generator
		*/
		long getSeed() const { return seed_; };

		/**
		* @return the offset
		*/
		double getOffset() const { return offset_; };

		/**
		* @return the maxBuckets for this RDSE
		*/
		int getMaxBuckets() const { return maxBuckets_; };

		/**
		* @return the minIndex for this RDSE
		*/
		int getMinIndex() const { return minIndex_; };

		/**
		* @return the maxIndex for this RDSE
		*/
		int getMaxIndex()  const { return maxIndex_; };

		/**
		* @param numRetry New number of retries for new representation
		*/
		int getNumRetry()  const { return numRetry_; };

		/**
		* @return maxOverlap for this RDSE
		*/
		int getMaxOverlap()  const { return maxOverlap_; }; 

		/**
		* @return BucketMap for this RDSE
		*/
		map<int, vector<nupic::UInt32>> getBucketMap() const { return bucketMap_; };
		 
		/**
		* @param minIndex
		*/
		void setMinIndex(int minIndex) { minIndex_ = minIndex; };

		/**
		* @param maxIndex
		*/
		void setMaxIndex(int maxIndex) { maxIndex_ = maxIndex; };

		/**
		* @param offset
		*/
		void setOffset(double offset) { offset_ = offset; };

		/**
		* @param numRetry New number of retries for new representation
		*/
		void setNumRetry(int numRetry) { numRetry_ = numRetry; };

		/**
		* @param maxBuckets the new maximum number of buckets allowed
		*/
		void setMaxBuckets(int maxBuckets) { maxBuckets_ = maxBuckets; };

		/**
		* @param seed
		*/
		void setSeed(long seed) { seed_ = seed; };

		/**
		* @param maxOverlap The maximum permissible overlap between representations
		*/
		void setMaxOverlap(int maxOverlap) { maxOverlap_ = maxOverlap; };

		// print RDSEncoder values 
		void toString();

		/**
		* This dictionary maps a bucket index into its bit representation
		*/
		map<int, vector<nupic::UInt32>> bucketMap_;

		/**
		* Should return the output width, in bits.
		* @Override
		*/
		virtual int getWidth() const override;

		virtual DecodeResult decode(vector<nupic::UInt> encoded, std::string parentFieldName) override;

		virtual vector< tuple<boost::any, boost::any, vector<nupic::UInt>> > topDownCompute(vector<nupic::UInt> encoded) override;

	protected:

		/**
		* The largest overlap we allow for non-adjacent encodings
		*/
		int maxOverlap_;

		/**
		* The maxBuckets for this RDSE
		*/
		int maxBuckets_;

		/**
		* Internal parameters for bucket mapping
		*/
		int minIndex_;
		int maxIndex_;
		int numRetry_;

		/**
		* A floating point offset used to map scalar inputs to bucket
		* indices. The middle bucket will correspond to numbers in the
		* range [offset - resolution/2, offset + resolution/2). If set
		* to None, the very first input that is encoded will be used
		* to determine the offset.
		*/
		double offset_;

		/**
		* The seed used for numpy's random number generator. If set to -1
		* the generator will be initialized without a fixed seed.
		*/
		long seed_;

		nupic::Random random_;

		friend class RandomDistributedScalarEncoderTest;

	}; // end class RandomDistributedScalarEncoder


	   /***************** Start of Implementation ******************/
	template<class T>
	RandomDistributedScalarEncoder<T>::RandomDistributedScalarEncoder() :
		maxOverlap_(2), maxBuckets_(1000), offset_(0), seed_(42) {
	}

	template<class T>
	RandomDistributedScalarEncoder<T>::~RandomDistributedScalarEncoder() {}

	template<class T>
	void RandomDistributedScalarEncoder<T>::init(double resolution, int w, int n, string name, double offset, long seed) {

		this->setResolution(resolution);
		this->setW(w);
		this->setN(n);
		this->setName(name);
		this->setOffset(offset);
		this->setSeed(seed); 

		// Validate inputs
		if (this->getW() <= 0 || this->getW() % 2 == 0) {
			NTA_THROW << "W must be an odd positive integer (to eliminate centering difficulty)";
		}

		this->setHalfWidth((this->getW() - 1) / 2);

		if (this->getResolution() <= 0) {
			NTA_THROW << "Resolution must be a positive number" ; 
		}

		if (this->n_ <= 6 * this->getW()) {
			NTA_THROW << "n must be strictly greater than 6*w. For good results we recommend n be strictly greater than 11*w." ;
		}

		//Initialize the random seed
		if (seed != -1) {
			random_ = Random(seed);
		}
		else {
			random_ = Random();
		}

		initializeBucketMap(maxBuckets_, offset_);

		if (this->getName() == "" || this->getName().empty()) {
			this->setName("[" + std::to_string(this->getResolution()) + "]");
		}

		// print RDSEncoder values 
		toString();
	}

	template<class T>
	void RandomDistributedScalarEncoder<T>::initializeBucketMap(int maxBuckets, double offset) {
		/*
		* The first bucket index will be _maxBuckets / 2 and bucket indices
		* will be allowed to grow lower or higher as long as they don't become
		* negative. _maxBuckets is required because the current CLA Classifier
		* assumes bucket indices must be non-negative. This normally does not
		* need to be changed but if altered, should be set to an even number.
		*/
		setMaxBuckets(maxBuckets);
		setMinIndex(maxBuckets / 2);
		setMaxIndex(maxBuckets / 2); 

		/*
		* The scalar offset used to map scalar values to bucket indices. The
		* middle bucket will correspond to numbers in the range
		* [offset-resolution/2, offset+resolution/2). The bucket index for a
		* number x will be: maxBuckets/2 + int( round( (x-offset)/resolution )
		* )
		*/
		setOffset(offset);

		/*
		* This Map maps a bucket index into its bit representation We
		* initialize the Map with a single bucket with index 0
		*/
		bucketMap_ = map<int, vector<nupic::UInt32>>();


		////////////////   random shuffle
		nupic::UInt32 * arr = new nupic::UInt32[this->getN()];
		for (int i = 0; i < this->getN(); i++) {
			arr[i] = static_cast<nupic::UInt32>(i);
		}
		nupic::UInt32* start = arr;
		nupic::UInt32* end = start + this->getN();
		random_.shuffle(start, end);

		vector<nupic::UInt32> temp1_(arr, arr + this->getN());  // OK
		//subList of temp1
		vector<nupic::UInt32> temp2_(temp1_.begin(), temp1_.begin() + this->getW());
 
		////////////////////////////////////////
		// generate the random permutation
		vector<int> temp1;
		for (int i = 0; i < this->getN(); i++) {
			temp1.push_back(i);
		}
		//see @SparseBinaryMatrix.hpp
		std::random_shuffle(temp1.begin(), temp1.end(), random_); // or...  random_.shuffle(temp1.begin(), temp1.end());  
		
		//subList of temp1
		vector<nupic::UInt32> temp2(temp1.begin(), temp1.begin() + this->getW()); 

		bucketMap_.insert(pair<int, vector<nupic::UInt32>>(getMinIndex(), temp2_)); 

		// How often we need to retry when generating valid encodings
		setNumRetry(0);

	}

	template<class T>
	void RandomDistributedScalarEncoder<T>::createBucket(int index) {
		if (index < getMinIndex()) {
			if (index == getMinIndex() - 1) {
				/*
				* Create a new representation that has exactly w-1 overlapping
				* bits as the min representation
				*/
				bucketMap_.insert(std::pair<int, vector<nupic::UInt32>>(index, newRepresentation(getMinIndex(), index)));
				setMinIndex(index);
			}
			else {
				// Recursively create all the indices above and then this index
				createBucket(index + 1);
				createBucket(index);
			}
		}
		else {
			if (index == getMaxIndex() + 1) {
				/*
				* Create a new representation that has exactly w-1 overlapping
				* bits as the max representation
				*/
				bucketMap_.insert(std::pair<int, vector<nupic::UInt32>>(index, newRepresentation(getMaxIndex(), index)));
				setMaxIndex(index);
			}
			else {
				// Recursively create all the indices below and then this index
				createBucket(index - 1);
				createBucket(index);
			}
		} 
	}

	template<class T>
	vector<nupic::UInt32> RandomDistributedScalarEncoder<T>::newRepresentation(int index, int newIndex) {

		vector<nupic::UInt32> newRepresentation = bucketMap_[index];

		/*
		* Choose the bit we will replace in this representation. We need to
		* shift this bit deterministically. If this is always chosen randomly
		* then there is a 1 in w chance of the same bit being replaced in
		* neighboring representations, which is fairly high
		*/
		int ri = newIndex % this->getW();

		//choose a bit such that the overlap rules are satisfied.
		nupic::UInt32 newBit = random_.getUInt32(this->getN());
		newRepresentation[ri] = newBit;

		//contains newBit
		while (std::find(bucketMap_[index].begin(), bucketMap_[index].end(), newBit) != bucketMap_[index].end()
			|| !newRepresentationOK(newRepresentation, newIndex))
		{
			setNumRetry(getNumRetry() + 1);
			newBit = random_.getUInt32(this->getN());
			newRepresentation[ri] = newBit;
		}

		return newRepresentation;
	}

	template<class T>
	bool RandomDistributedScalarEncoder<T>::newRepresentationOK(vector<nupic::UInt32> newRep, int newIndex) {

		if (newRep.size() != (unsigned long)this->getW()) {
			return false;
		}
		if (newIndex < this->getMinIndex() - 1 || newIndex > this->getMaxIndex() + 1) {
			NTA_THROW << "newIndex must be within one of existing indices"; 
		}

		// A binary representation of newRep. We will use this to test
		// containment
		vector<bool> newRepBinary(this->getN(), false); 

		for (int index : newRep) {
			newRepBinary[index] = true;
		}
		// Midpoint
		int midIdx = getMaxBuckets() / 2;

		// Start by checking the overlap at minIndex
		int runningOverlap = countOverlap(bucketMap_.find(getMinIndex())->second, newRep);
		if (!overlapOK(getMinIndex(), newIndex, runningOverlap)) {
			return false;
		}

		// Compute running overlaps all the way to the midpoint
		for (int i = getMinIndex() + 1; i < midIdx + 1; i++) {
			// This is the bit that is going to change
			int newBit = (i - 1) % this->getW();

			// Update our running overlap
			if (newRepBinary[bucketMap_.find(i - 1)->second[newBit]]) {
				runningOverlap--;
			}
			if (newRepBinary[bucketMap_.find(i)->second[newBit]]) {
				runningOverlap++;
			}
			// Verify our rules
			if (!overlapOK(i, newIndex, runningOverlap)) {
				return false;
			}
		}

		// At this point, runningOverlap contains the overlap for midIdx
		// Compute running overlaps all the way to maxIndex
		for (int i = midIdx + 1; i <= getMaxIndex(); i++) {
			int newBit = i % this->getW();

			// Update our running overlap
			if (newRepBinary[bucketMap_.find(i - 1)->second[newBit]]) {
				runningOverlap--;
			}
			if (newRepBinary[bucketMap_.find(i)->second[newBit]]) {
				runningOverlap++;
			}
			// Verify our rules
			if (!overlapOK(i, newIndex, runningOverlap)) {
				return false;
			}
		}
		return true;
	}

	template<class T>
	int RandomDistributedScalarEncoder<T>::countOverlap(vector<nupic::UInt32> rep1, vector<nupic::UInt32> rep2) {
		int overlap = 0;
		for (int index : rep1) {
			for (int index2 : rep2) {
				if (index == index2) {
					overlap++;
				}
			}
		}

		return overlap;
	}

	template<class T>
	bool RandomDistributedScalarEncoder<T>::overlapOK(int i, int j, int overlap) {

		if (abs(i - j) < this->getW() && overlap == (this->getW() - abs(i - j))) {
			return true;
		}
		if (abs(i - j) >= this->getW() && overlap <= this->getMaxOverlap()) {
			return true;
		}
		return false;
	}

	template<class T>
	bool RandomDistributedScalarEncoder<T>::overlapOK(int i, int j) {
		return overlapOK(i, j, countOverlapIndices(i, j)); 
	}

	template<class T>
	int RandomDistributedScalarEncoder<T>::countOverlapIndices(int i, int j) {

		auto itI = bucketMap_.find(i);
		auto itJ = bucketMap_.find(j);
		if (itI != bucketMap_.end() && itJ != bucketMap_.end()) {
			vector<nupic::UInt32> rep1 = itI->second;
			vector<nupic::UInt32> rep2 = itJ->second;
			
			return countOverlap(rep1, rep2);
		}
		else {
			NTA_THROW << "Either i or j don't exist"  ; 
		}
	}

	template<class T>
	void RandomDistributedScalarEncoder<T>::encodeIntoArray(T inputData, vector<nupic::UInt>& output) {

		vector<int> bucketIdx = getBucketIndices(boost::any_cast<double>(inputData));		 

		if (bucketIdx.size() == 0) {
			return;
		}
		if (bucketIdx[0] != numeric_limits<int>::min()) {
			vector<nupic::UInt32> indices = mapBucketIndexToNonZeroBits(bucketIdx[0]);
			for (int index : indices) {
				output[index] = 1;
			}
		}
	}

	template<class T>
	vector<int> RandomDistributedScalarEncoder<T>::getBucketIndices(double x) {
		if (std::isnan(x) || x == SENTINEL_VALUE_FOR_MISSING_DATA) {
			return  vector<int>();
		}

		if (std::isnan(offset_)) {
			setOffset(x);
		}

		/*
		* Difference in the round function behavior for Python and Java In
		* Python, the absolute value is rounded up and sign is applied in Java,
		* value is rounded to next biggest integer
		*
		* so for Python, round(-0.5) => -1.0 whereas in Java, Math.round(-0.5)
		* => 0.0
		*/
		double deltaIndex = (double)((double)(x - this->getOffset()) / this->getResolution());
		int sign = (int)(deltaIndex / abs(deltaIndex));
		int bucketIdx_ = (getMaxBuckets() / 2) + (sign * round(abs(deltaIndex)));

		if (bucketIdx_ < 0) {
			bucketIdx_ = 0;
		}
		else if (bucketIdx_ >= getMaxBuckets()) {
			bucketIdx_ = getMaxBuckets() - 1;
		} 

		return{ bucketIdx_ };			 
	}

	template<class T>
	vector<nupic::UInt32>  RandomDistributedScalarEncoder<T>::mapBucketIndexToNonZeroBits(int index) {

		if (index < 0) {
			index = 0;
		}
		if (index >= getMaxBuckets()) {
			index = getMaxBuckets() - 1;
		}

		std::map<int, vector<nupic::UInt32>>::iterator it = bucketMap_.find(index);
		if (it == bucketMap_.end()) {
			createBucket(index);
		} 

		return bucketMap_[index];		
	}

	template<class T>
	vector<tuple<std::string, int>> RandomDistributedScalarEncoder<T>::getDescription() const{
		string name = this->getName();
		vector<tuple<std::string, int>> desc;
		desc.push_back(std::make_tuple(name, 0));
		return desc;
	}
 
	template<class T>
	int RandomDistributedScalarEncoder<T>::getWidth() const{
		return this->getN();
	}

	template<class T>
	DecodeResult RandomDistributedScalarEncoder<T>::decode(vector<nupic::UInt> encoded, std::string parentFieldName) {
		return DecodeResult(); //TODO
	}

	template<class T>
	vector< tuple<boost::any, boost::any, vector<nupic::UInt>> > RandomDistributedScalarEncoder<T>::topDownCompute(vector<nupic::UInt> encoded) {
		return vector< tuple<boost::any, boost::any, vector<nupic::UInt>> >(); //TODO
	}

	template<class T>
	void RandomDistributedScalarEncoder<T>::toString() { //TODO rather implement << 
		std::cout << "\n RandomDistributedScalarEncoder:" << std::endl;
		std::cout << "  minIndex: " << getMinIndex() << std::endl;
		std::cout << "  maxIndex: " << getMaxIndex() << std::endl;
		std::cout << "  w: " << this->getW() << std::endl;
		std::cout << "  n: " << this->getN() << std::endl;
		std::cout << "  resolution: " << this->getResolution() << std::endl;
		std::cout << "  offset: " << getOffset() << std::endl;
		std::cout << "  numTries: " << getNumRetry() << std::endl;
		std::cout << "  name: " << this->getName() << std::endl;
		std::cout << "  buckets :" << std::endl;

		std::string str;
		for (map<int, vector<nupic::UInt32>>::iterator it = bucketMap_.begin(); it != bucketMap_.end(); ++it) {
			cout << " [" << it->first << "]: [";
			for (vector<nupic::UInt32>::iterator i = it->second.begin(); i != it->second.end(); ++i) {
				cout << ' ' << *i;
			}
			std::cout << "]" << '\n';
		}
	}

}; // end namespace 
#endif // NTA_RandomDistributedScalarEncoder_HPP
