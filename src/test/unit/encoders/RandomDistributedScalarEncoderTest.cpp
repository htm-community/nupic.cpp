#include <nupic/encoders/RandomDistributedScalarEncoder.hpp>

#include "nupic/utils/Random.hpp" 
#include "gtest/gtest.h"

using namespace encoders;
using namespace std;

RandomDistributedScalarEncoder<double> rdse;

vector<UInt32> getRangeAsList(int lowerBound, int upperBound) {
	if (lowerBound > upperBound)
		return vector<UInt32>();

	vector<UInt32> arr((upperBound - lowerBound), 0);
	for (int i = lowerBound; i < upperBound; i++) {
		arr[i - lowerBound] = i;
	}

	return arr;
}

bool validateEncoder(RandomDistributedScalarEncoder<double> encoder, int subsampling) {
	for (int i = encoder.getMinIndex(); i <= encoder.getMaxIndex(); i++) {
		for (int j = i + 1; j <= encoder.getMaxIndex(); j += subsampling) {
			if (!encoder.overlapOK(i, j))
				return false;
		}
	}
	return true;
}


int computeOverlap(vector<nupic::UInt> result1, vector<nupic::UInt> result2) {
	if (result1.size() != result2.size())
		return 0;

	int overlap = 0;
	for (int i = 0; i < (int)result1.size(); i++)
		if (result1[i] == 1 && result2[i] == 1)
			overlap++;

	return overlap;
}


int getOnBits(vector<nupic::UInt> input) {
	int onBits = 0;
	for (int i : input)
	{
		if (i == 1)
			onBits += 1;
		else if (i != 0)
			return -1;
	}
	return onBits;
}

/**
* Test basic encoding functionality. Create encodings without crashing and
* check they contain the correct number of on and off bits. Check some
* encodings for expected overlap. Test that encodings for old values don't
* change once we generate new buckets.
*/
TEST(RandomDistributedScalarEncoderTest, testEncoding) {

	double resolution = 1.0;
	int w = 23;
	int n = 500;
	string name = "encoder";
	double offset = 0.0;
	long seed = rdse.getSeed();
	rdse.init(resolution, w, n, name, offset, seed);

	vector<nupic::UInt> e0 = rdse.encode(-0.1); 
	ASSERT_EQ(getOnBits(e0), 23) << "Number of on bits is incorrect"; 
	ASSERT_EQ(e0.size(), 500) << "Width of the vector is incorrect"; 
	ASSERT_EQ(rdse.getBucketIndices(0)[0], rdse.getMaxBuckets() / 2) << "Offset doesn't correspond to middle bucket"; 
	ASSERT_EQ(1, rdse.getBucketMap().size()) << "Number of buckets is not 1	";


	// Encode with a number that is resolution away from offset. Now we should
	// have two buckets and this encoding should be one bit away from e0
	vector<nupic::UInt> e1 = rdse.encode(1.0); 
	ASSERT_EQ(2, rdse.getBucketMap().size()) << "Number of buckets is not 2"; 
	ASSERT_EQ(getOnBits(e1), 23) << "Number of on bits is incorrect"; 
	ASSERT_EQ(e0.size(), 500) << "Width of the vector is incorrect"; 	
	ASSERT_EQ(computeOverlap(e0, e1), 22) << "Overlap is not equal to w-1";


	// Encode with a number that is resolution*w away from offset. Now we should
	// have many buckets and this encoding should have very little overlap with e0
	vector<nupic::UInt> e25 = rdse.encode(25.0); 
	ASSERT_TRUE(rdse.getBucketMap().size() > 23) << "Buckets created are not more than 23"; 
	ASSERT_EQ(getOnBits(e1), 23) << "Number of on bits is incorrect"; 
	ASSERT_EQ(e0.size(), 500) << "Width of the vector is incorrect"; 
	ASSERT_TRUE(computeOverlap(e0, e25) < 4) << "Overlap is too high";

	// Test encoding consistency. The encodings for previous numbers
	// shouldn't change even though we have added additional buckets
	ASSERT_TRUE(equal(e0.begin(), e0.end(), rdse.encode(-0.1).begin())) << "Encodings are not consistent - they have changed after new buckets have been created";
	ASSERT_TRUE(equal(e1.begin(), e1.end(), rdse.encode(1.0).begin()))  << "Encodings are not consistent - they have changed after new buckets have been created";

}

/**
 * Check that the overlaps for the encodings are within the expected range.
 * Here we ask the encoder to create a bunch of representations under somewhat
 * stressfull conditions, and then verify they are correct. We rely on the fact
 * that the _overlapOK and _countOverlapIndices methods are working correctly.
 */
TEST(RandomDistributedScalarEncoderTest, testOverlapStatistics) {

	double resolution = 1;
	int w = 11;
	int n = 150;
	string name = "";
	double offset = 0;
	long seed = rdse.getSeed();
	rdse.init(resolution, w, n, name, offset, seed);

	rdse.encode(0.0);
	rdse.encode(-300.0);
	rdse.encode(300.0);
	 
	ASSERT_TRUE(validateEncoder(rdse, 3)) << "Illegal overlap encountered in encoder";

}

TEST(RandomDistributedScalarEncoderTest, testCountOverlapIndeciesWithWrongIndices_i_j) {

	double resolution = 1;
	int w = 5;
	int n = 100;
	string name = "enc";
	double offset = 0;
	long seed = rdse.getSeed();
	rdse.init(resolution, w, n, name, offset, seed);

	int midIdx = rdse.getMaxBuckets() / 2;

	rdse.bucketMap_[midIdx - 2] = getRangeAsList(3, 8);
	rdse.bucketMap_[midIdx - 1] = getRangeAsList(4, 9);
	rdse.bucketMap_[midIdx]		= getRangeAsList(5, 10);
	rdse.bucketMap_[midIdx + 1] = getRangeAsList(6, 11);
	rdse.bucketMap_[midIdx + 2] = getRangeAsList(7, 12);
	rdse.bucketMap_[midIdx + 3] = getRangeAsList(8, 13);

	rdse.setMinIndex(midIdx - 2);
	rdse.setMaxIndex(midIdx + 3);
	 
	bool caught = false;
	try
	{
		rdse.countOverlapIndices(midIdx - 3, midIdx - 4);
	}
	catch (LoggingException& exc)
	{
		caught = true;
	}
	ASSERT_TRUE(caught)	<< "index don't exist";
}

TEST(RandomDistributedScalarEncoderTest, testSeed) {

	double resolution = 1;
	int w = 21;
	int n = 400;
	string name = "enc";
	double offset = 0;

	RandomDistributedScalarEncoder<double> encoder1;
	encoder1.setSeed(42);
	encoder1.init(resolution, w, n, name, offset, encoder1.getSeed());

	RandomDistributedScalarEncoder<double> encoder2;
	encoder2.setSeed(42);
	encoder2.init(resolution, w, n, name, offset, encoder2.getSeed());

	RandomDistributedScalarEncoder<double> encoder3;
	encoder3.setSeed(-1);
	encoder3.init(resolution, w, n, name, offset, encoder3.getSeed());

	vector<nupic::UInt> e1 = encoder1.encode(23.0);
	vector<nupic::UInt> e2 = encoder2.encode(23.0);
	vector<nupic::UInt> e3 = encoder3.encode(23.0);
	 
	ASSERT_TRUE(equal(e1.begin(), e1.end(), e2.begin())) << "Same seed gives rise to different encodings";
	ASSERT_TRUE(!equal(e1.begin(), e1.end(), e3.begin())) << "Different seeds gives rise to same encodings";
}

TEST(RandomDistributedScalarEncoderTest, testGetOnBitsMethod)
{ 
	vector<nupic::UInt> input1( { 1,0,0,0,1 } ); 
	vector<nupic::UInt> input2( { 1,0,2,0,1 });
	 
	ASSERT_EQ(2, getOnBits(input1)) << "getOnBits returned wrong value ";
	ASSERT_EQ(-1, getOnBits(input2)) << "getOnBits did not return -1 for invalid input";
}

// Test that mapBucketIndexToNonZeroBits works and that max buckets and
// clipping are handled properly.
TEST(RandomDistributedScalarEncoderTest, testMapBucketIndexToNonZeroBits)
{
	RandomDistributedScalarEncoder<double> rdse2;

	double resolution = 1;
	int w = 11;
	int n = 150;
	string name = "";
	double offset = NAN;
	long seed = rdse2.getSeed();
	rdse2.init(resolution, w, n, name, offset, seed);

	rdse2.initializeBucketMap(10, NAN);

	rdse2.encode(0.0); 
	rdse2.encode(-7.0); 
	rdse2.encode(7.0); 
	 
	ASSERT_EQ(rdse2.getMaxBuckets(), rdse2.getBucketMap().size()) << " maxBuckets exceeded ";		 
																								 
	vector<nupic::UInt32> bucketMap = rdse2.getBucketMap()[0];
	ASSERT_TRUE(equal(bucketMap.begin(), bucketMap.end(), rdse2.mapBucketIndexToNonZeroBits(-1).begin())) << "mapBucketIndexToNonZeroBits did not handle negative index";

	bucketMap = rdse2.getBucketMap()[9];
	ASSERT_TRUE(equal(bucketMap.begin(), bucketMap.end(), rdse2.mapBucketIndexToNonZeroBits(1000).begin())) << "mapBucketIndexToNonZeroBits did not handle negative index";

	vector<nupic::UInt> e23 = rdse2.encode(23.0);
	vector<nupic::UInt> e6 = rdse2.encode(6.0);

	ASSERT_TRUE(equal(e23.begin(), e23.end(), e6.begin())) << "Values not clipped correctly during encoding"; 
	ASSERT_EQ(0, rdse2.getBucketIndices(-8.0)[0]) << "getBucketIndices returned negative bucket index";
	ASSERT_EQ(rdse2.getMaxBuckets() - 1, rdse2.getBucketIndices(23.0)[0]) << "getBucketIndices returned negative bucket index";

} 

// Test that the internal method {@link RandomDistributedScalarEncoder#overlapOK(int, int)}
// works as expected.
TEST(RandomDistributedScalarEncoderTest, testOverlapOK) {

	double resolution = 1;
	int w = 5;
	int n = (5 * 20);
	string name = "enc";
	double offset = NAN;
	long seed = rdse.getSeed();
	rdse.init(resolution, w, n, name, offset, seed);
	 
	int midIdx = rdse.getMaxBuckets() / 2;

	rdse.bucketMap_[midIdx - 3] = getRangeAsList(4, 9);
	rdse.bucketMap_[midIdx - 2] = getRangeAsList(3, 8);
	rdse.bucketMap_[midIdx - 1] = getRangeAsList(4, 9);
	rdse.bucketMap_[midIdx]     = getRangeAsList(5, 10);
	rdse.bucketMap_[midIdx + 1] = getRangeAsList(6, 11);
	rdse.bucketMap_[midIdx + 2] = getRangeAsList(7, 12);
	rdse.bucketMap_[midIdx + 3] = getRangeAsList(8, 13);
	rdse.setMinIndex( midIdx - 3 );
	rdse.setMaxIndex( midIdx + 3 );

	ASSERT_TRUE(rdse.overlapOK(midIdx, midIdx - 1)) << "overlapOK didn't work";
	ASSERT_TRUE(rdse.overlapOK(midIdx - 2, midIdx + 3)) << "overlapOK didn't work";
	ASSERT_FALSE(rdse.overlapOK(midIdx - 3, midIdx - 1)) << "overlapOK didn't work";

	ASSERT_TRUE(rdse.overlapOK(100, 50, 0)) << "overlapOK didn't work for far values";
	ASSERT_TRUE(rdse.overlapOK(100, 50, rdse.getMaxOverlap())) << "overlapOK didn't work for far values";
	ASSERT_FALSE(rdse.overlapOK(100, 50, (rdse.getMaxOverlap() + 1))) << "overlapOK didn't work for far values";

	ASSERT_TRUE(rdse.overlapOK(50, 50, 5)) << "overlapOK didn't work for far values";
	ASSERT_TRUE(rdse.overlapOK(48, 50, 3)) << "overlapOK didn't work for far values";
	ASSERT_TRUE(rdse.overlapOK(46, 50, 1)) << "overlapOK didn't work for far values";

	ASSERT_TRUE(rdse.overlapOK(45, 50, rdse.getMaxOverlap())) << "overlapOK didn't work for far values";
	ASSERT_FALSE(rdse.overlapOK(48, 50, 4)) << "overlapOK didn't work for far values";
	ASSERT_FALSE(rdse.overlapOK(48, 50, 2)) << "overlapOK didn't work for far values";
	ASSERT_FALSE(rdse.overlapOK(46, 50, 2)) << "overlapOK didn't work for far values";
	ASSERT_FALSE(rdse.overlapOK(50, 50, 6)) << "overlapOK didn't work for far values";
} 

// Test that missing values and NaN return all zero's.
TEST(RandomDistributedScalarEncoderTest, testMissingValues) {

	double resolution = 1;
	int w = 21;
	int n = 400;
	string name = "enc";
	double offset = NAN;
	long seed = rdse.getSeed();
	rdse.init(resolution, w, n, name, offset, seed);

	vector<nupic::UInt> e1 = rdse.encode(SENTINEL_VALUE_FOR_MISSING_DATA);
	ASSERT_EQ(0, getOnBits(e1));
}

TEST(RandomDistributedScalarEncoderTest, testCountOverlap) {

	double resolution = 1;
	int w = 21;
	int n = 500;
	string name = "enc";
	double offset = NAN;
	long seed = rdse.getSeed();
	rdse.init(resolution, w, n, name, offset, seed);

	vector<nupic::UInt> r1({ 1, 2, 3, 4, 5, 6 });
	vector<nupic::UInt> r2({ 1, 2, 3, 4, 5, 6 });
	ASSERT_EQ(6, rdse.countOverlap(r1, r2));

	r1 = vector<nupic::UInt>({ 1, 2, 3, 4, 5, 6 });
	r2 = vector<nupic::UInt>({ 1, 2, 3, 4, 5, 7 });
	ASSERT_EQ(5, rdse.countOverlap(r1, r2));

	r1 = vector<nupic::UInt>({ 1, 2, 3, 4, 5, 6 });
	r2 = vector<nupic::UInt>({ 6, 5, 4, 3, 2, 1 });
	ASSERT_EQ(6, rdse.countOverlap(r1, r2));

	r1 = vector<nupic::UInt>({ 1, 2, 8, 4, 5, 6 });
	r2 = vector<nupic::UInt>({ 1, 2, 3, 4, 9, 6 });
	ASSERT_EQ(4, rdse.countOverlap(r1, r2));

	r1 = vector<nupic::UInt>({ 1, 2, 3, 4, 5, 6 });
	r2 = vector<nupic::UInt>({ 1, 2, 3 });
	ASSERT_EQ(3, rdse.countOverlap(r1, r2));

	r1 = vector<nupic::UInt>({ 7, 8, 9, 10, 11, 12 });
	r2 = vector<nupic::UInt>({ 1, 2, 3, 4, 5, 6 });
	ASSERT_EQ(0, rdse.countOverlap(r1, r2));
}

// Test that offset is working properly
TEST(RandomDistributedScalarEncoderTest, testOffset) {

	double resolution = 1;
	int w = 21;
	int n = 400;
	string name = "enc";
	double offset = NAN; 
	long seed = rdse.getSeed();
	rdse.init(resolution, w, n, name, offset, seed);
	 
	rdse.encode(23.0);
	ASSERT_EQ(23, rdse.getOffset()) << "Offset not initialized to specified constructor parameter";

	rdse.setOffset(25.0);

	rdse.encode(23.0);
	ASSERT_EQ(25, rdse.getOffset()) << "Offset not initialized to specified constructor parameter";
}

// Test that the internal method _countOverlapIndices works as expected.
TEST(RandomDistributedScalarEncoderTest, testCountOverlapIndices) {

	double resolution = 1;
	int w = 5;
	int n = 5 * 20;
	string name = "enc";
	double offset = NAN;
	long seed = rdse.getSeed();
	rdse.init(resolution, w, n, name, offset, seed);

	// Create a fake set of encodings.
	int midIdx = rdse.getMaxBuckets() / 2;	 

	rdse.bucketMap_[midIdx - 2] = getRangeAsList(3, 8);
	rdse.bucketMap_[midIdx - 1] = getRangeAsList(4, 9);
	rdse.bucketMap_[midIdx]		= getRangeAsList(5, 10);
	rdse.bucketMap_[midIdx + 1] = getRangeAsList(6, 11);
	rdse.bucketMap_[midIdx + 2] = getRangeAsList(7, 12);
	rdse.bucketMap_[midIdx + 3] = getRangeAsList(8, 13);
	rdse.setMinIndex(midIdx - 2);
	rdse.setMaxIndex(midIdx + 3);

	// Test some overlaps
	ASSERT_EQ(5, rdse.countOverlapIndices(midIdx - 2, midIdx - 2)) << "countOverlapIndices didn't work";	
	ASSERT_EQ(4, rdse.countOverlapIndices(midIdx - 1, midIdx - 2)) << "countOverlapIndices didn't work";
	ASSERT_EQ(2, rdse.countOverlapIndices(midIdx + 1, midIdx - 2)) << "countOverlapIndices didn't work";
	ASSERT_EQ(0, rdse.countOverlapIndices(midIdx - 2, midIdx + 3)) << "countOverlapIndices didn't work";
}
	

TEST(RandomDistributedScalarEncoderTest, testCountOverlapIndeciesWithWrongIndices_i) {

	double resolution = 1;
	int w = 21;
	int n = 400;
	string name = "enc";
	double offset = NAN;
	long seed = rdse.getSeed();
	rdse.init(resolution, w, n, name, offset, seed);

	// Create a fake set of encodings.
	int midIdx = rdse.getMaxBuckets() / 2;

	rdse.bucketMap_[midIdx - 2] = getRangeAsList(3, 8);
	rdse.bucketMap_[midIdx - 1] = getRangeAsList(4, 9);
	rdse.bucketMap_[midIdx]		= getRangeAsList(5, 10);
	rdse.bucketMap_[midIdx + 1] = getRangeAsList(6, 11);
	rdse.bucketMap_[midIdx + 2] = getRangeAsList(7, 12);
	rdse.bucketMap_[midIdx + 3] = getRangeAsList(8, 13);
	rdse.setMinIndex(midIdx - 2);
	rdse.setMaxIndex(midIdx + 3);

	// Test some overlaps
	bool caught = false;
	try
	{
		rdse.countOverlapIndices(midIdx - 3, midIdx - 2);
	}
	catch (LoggingException& exc)
	{
		caught = true;
	}
	ASSERT_TRUE(caught)	<< "index don't exist";
}

/**
 * Test that numbers within the same resolution return the same encoding.
 * Numbers outside the resolution should return different encodings.
 */
TEST(RandomDistributedScalarEncoderTest, testResolution) {

	double resolution = 1;
	int w = 5;
	int n = 5 * 20;
	string name = "enc";
	double offset = NAN;
	long seed = rdse.getSeed();
	rdse.init(resolution, w, n, name, offset, seed);

	vector<nupic::UInt> e23		= rdse.encode(23.0);
	vector<nupic::UInt> e23_1	= rdse.encode(23.1);
	vector<nupic::UInt> e22_9	= rdse.encode(22.9);
	vector<nupic::UInt> e24		= rdse.encode(24.0);
	 
	ASSERT_EQ(rdse.getW(), getOnBits(e23));
	ASSERT_TRUE(equal(e23_1.begin(), e23_1.end(), e23.begin())) << "Numbers within resolution don't have the same encoding";
	ASSERT_TRUE(equal(e22_9.begin(), e22_9.end(), e23.begin())) << "Numbers within resolution don't have the same encoding";
	
	ASSERT_TRUE(!equal(e23.begin(), e23.end(), e24.begin())) << "Numbers outside resolution have the same encoding";
	
	vector<nupic::UInt> e22_5 = rdse.encode(22.5);
	ASSERT_TRUE(!equal(e23.begin(), e23.end(), e22_5.begin())) << "Numbers outside resolution have the same encoding";
}
