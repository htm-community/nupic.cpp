#include <nupic/encoders/ScalarEncoder2.hpp>
#include "gtest/gtest.h"

using namespace encoders;
using namespace std;

ScalarEncoder2<double> se;

int se_w = 3;
int se_n = 14;
double se_minVal = 1.0;
double se_maxVal = 8.0;
string se_name = "scalar";
double se_radius = 0.0;
double se_resolution = 0.0;
bool se_periodic = true;
bool se_forced = true;

void initSE() {
	se.init(se_w, se_n, se_minVal, se_maxVal, se_name, se_radius, se_resolution, se_periodic, se_forced);
}

TEST(ScalarEncoder2Test, testBottomUpEncodingPeriodicEncoder) {

	initSE();

	vector<int> enc1({ 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 });
	vector<int> enc2({ 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 });
	vector<int> enc3({ 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 });
	vector<int> enc4({ 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 });
	vector<int> enc5({ 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 });
	vector<int> enc6({ 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0 });
	vector<int> enc7({ 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 });
	vector<int> enc8({ 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 });
	vector<int> enc9({ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1 });
	vector<int> enc10({ 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 });

	vector<nupic::UInt> res = se.encode(3.0);
	ASSERT_TRUE(equal(res.begin(), res.end(), enc1.begin()));

	res = se.encode(3.1);
	ASSERT_TRUE(equal(res.begin(), res.end(), enc2.begin()));

	res = se.encode(3.5);
	ASSERT_TRUE(equal(res.begin(), res.end(), enc3.begin()));

	res = se.encode(3.6);
	ASSERT_TRUE(equal(res.begin(), res.end(), enc4.begin()));

	res = se.encode(3.7);
	ASSERT_TRUE(equal(res.begin(), res.end(), enc5.begin()));

	res = se.encode(4);
	ASSERT_TRUE(equal(res.begin(), res.end(), enc6.begin()));

	res = se.encode(1);
	ASSERT_TRUE(equal(res.begin(), res.end(), enc7.begin()));

	res = se.encode(1.5);
	ASSERT_TRUE(equal(res.begin(), res.end(), enc8.begin()));

	res = se.encode(7);
	ASSERT_TRUE(equal(res.begin(), res.end(), enc9.begin()));

	res = se.encode(7.5);
	ASSERT_TRUE(equal(res.begin(), res.end(), enc10.begin()));

	ASSERT_EQ(0.5, se.getResolution());
	ASSERT_EQ(1.5, se.getRadius());
}

/**
* Test the input description generation, top-down compute, and bucket
* support on a periodic encoder
*/
TEST(ScalarEncoder2Test, testDecodeAndResolution) {

	initSE();
	double resolution = se.getResolution();
	for (double v = se.getMinVal(); v < se.getMaxVal(); v += (resolution / 4.0)) {

		vector<nupic::UInt> output = se.encode(v);
		DecodeResult decoded = se.decode(output, "");
		map<string, RangeList> fieldsMap = decoded.getFields();

		ASSERT_EQ(1, fieldsMap.size());

		for (auto &it : fieldsMap) {
			double rangMin = it.second.getRange(0).first;
			double rangMax = it.second.getRange(0).second;
			ASSERT_EQ(1, it.second.size());
			ASSERT_EQ(rangMin, rangMax);
			ASSERT_TRUE((rangMin - v) < se.getResolution());
		}

		vector< tuple<boost::any, boost::any, vector<nupic::UInt>> >  topDown = se.topDownCompute(output);
		double bucket_value = boost::any_cast<double>(get<0>(topDown[0]));
		double bucket_scalar = boost::any_cast<double>(get<1>(topDown[0]));
		vector<nupic::UInt> bucket_vec = get<2>(topDown[0]);

		ASSERT_TRUE(equal(bucket_vec.begin(), bucket_vec.end(), output.begin()));
		ASSERT_TRUE(abs(bucket_scalar - v) <= se.getResolution() / 2);

		//Test bucket support
		vector<nupic::UInt> bucketIndices = se.getBucketIndices(v);

		topDown = se.getBucketInfo(bucketIndices);
		bucket_value = boost::any_cast<double>(get<0>(topDown[0]));
		bucket_scalar = boost::any_cast<double>(get<1>(topDown[0]));
		bucket_vec = get<2>(topDown[0]);

		ASSERT_TRUE(abs(bucket_scalar - v) <= se.getResolution() / 2);
		ASSERT_EQ(bucket_value, bucket_scalar);
		ASSERT_TRUE(equal(bucket_vec.begin(), bucket_vec.end(), output.begin()));
	}

	// -----------------------------------------------------------------------
	// Test the input description generation on a large number, periodic encoder
	se_radius = 1.5;
	initSE();
	cout << "\nTesting periodic encoder decoding, resolution of " << se.getResolution() << endl;

	//Test with a "hole"
	vector<nupic::UInt> encoded({ 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0 });
	DecodeResult decoded = se.decode(encoded, "");
	map<string, RangeList> fieldsMap = decoded.getFields();

	ASSERT_EQ(1, fieldsMap.size());
	ASSERT_EQ(1, decoded.getRanges("scalar").size());
	ASSERT_EQ(decoded.getRanges("scalar").getRange(0).first, 7.5);
	ASSERT_EQ(decoded.getRanges("scalar").getRange(0).second, 7.5);


	//Test with something wider than w, and with a hole, and wrapped
	vector<nupic::UInt> encoded2({ 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0 });
	decoded = se.decode(encoded2, "");
	fieldsMap = decoded.getFields();
	ASSERT_EQ(1, fieldsMap.size());
	ASSERT_EQ(2, decoded.getRanges("scalar").size());
	ASSERT_EQ(decoded.getRanges("scalar").getRange(0).first, 7.5);
	ASSERT_EQ(decoded.getRanges("scalar").getRange(0).second, 8.0);

	//Test with something wider than w, no hole
	vector<nupic::UInt> encoded3({ 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 });
	decoded = se.decode(encoded3, "");
	fieldsMap = decoded.getFields();
	ASSERT_EQ(1, fieldsMap.size());
	ASSERT_EQ(1, decoded.getRanges("scalar").size());
	ASSERT_EQ(decoded.getRanges("scalar").getRange(0).first, 1.5);
	ASSERT_EQ(decoded.getRanges("scalar").getRange(0).second, 2.5);

	//Test with 2 ranges
	vector<nupic::UInt> encoded4({ 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0 });
	decoded = se.decode(encoded4, "");
	fieldsMap = decoded.getFields();
	ASSERT_EQ(1, fieldsMap.size());
	ASSERT_EQ(2, decoded.getRanges("scalar").size());
	ASSERT_EQ(decoded.getRanges("scalar").getRange(0).first, 1.5);
	ASSERT_EQ(decoded.getRanges("scalar").getRange(0).second, 1.5);
	ASSERT_EQ(decoded.getRanges("scalar").getRange(1).first, 5.5);
	ASSERT_EQ(decoded.getRanges("scalar").getRange(1).second, 6.0);

	//Test with 2 ranges, 1 of which is narrower than w
	vector<nupic::UInt> encoded5({ 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0 });
	decoded = se.decode(encoded5, "");
	fieldsMap = decoded.getFields();
	ASSERT_EQ(1, fieldsMap.size());
	ASSERT_EQ(2, decoded.getRanges("scalar").size());
	ASSERT_EQ(decoded.getRanges("scalar").getRange(0).first, 1.5);
	ASSERT_EQ(decoded.getRanges("scalar").getRange(0).second, 1.5);
	ASSERT_EQ(decoded.getRanges("scalar").getRange(1).first, 5.5);
	ASSERT_EQ(decoded.getRanges("scalar").getRange(1).second, 6.0);
}

TEST(ScalarEncoder2Test, testNonPeriodicBottomUp) {
	se_w = 5;
	se_n = 14;
	se_minVal = 1.0;
	se_maxVal = 10.0;
	se_name = "day of week";
	se_radius = 1.0;
	se_resolution = 0.0;
	se_periodic = false;
	se_forced = true;
	initSE();

	cout << "Testing non-periodic encoder encoding resolution of " << se.getResolution() << endl;

	vector<int> enc1({ 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 });
	vector<int> enc2({ 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 });
	vector<int> enc3({ 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1 });

	vector<nupic::UInt> encoded1 = se.encode(1);
	vector<nupic::UInt> encoded2 = se.encode(2);
	vector<nupic::UInt> encoded3 = se.encode(10);
	ASSERT_TRUE(equal(enc1.begin(), enc1.end(), enc1.begin()));
	ASSERT_TRUE(equal(enc2.begin(), enc2.end(), enc2.begin()));
	ASSERT_TRUE(equal(enc3.begin(), enc3.end(), enc3.begin()));

	// Test that we get the same encoder when we construct it using resolution
	// instead of n
	se_w = 5;
	se_n = 0;
	se_minVal = 1.0;
	se_maxVal = 10.0;
	se_name = "day of week";
	se_radius = 5.0;
	se_resolution = 0.0;
	se_periodic = false;
	se_forced = true;
	initSE();

	double v = se.getMinVal();
	while (v < se.getMaxVal()) {
		vector<nupic::UInt> output = se.encode(v);
		DecodeResult decoded = se.decode(output, "");
		map<string, RangeList> fieldsMap = decoded.getFields();

		ASSERT_EQ(1, fieldsMap.size());

		for (auto &it : fieldsMap) {
			double rangMin = it.second.getRange(0).first;
			double rangMax = it.second.getRange(0).second;
			ASSERT_EQ(1, it.second.size());
			ASSERT_EQ(rangMin, rangMax);
			ASSERT_TRUE((rangMin - v) < se.getResolution());
		}

		vector< tuple<boost::any, boost::any, vector<nupic::UInt>> >  topDown = se.topDownCompute(output);
		double bucket_value = boost::any_cast<double>(get<0>(topDown[0]));
		double bucket_scalar = boost::any_cast<double>(get<1>(topDown[0]));
		vector<nupic::UInt> bucket_vec = get<2>(topDown[0]);

		ASSERT_TRUE(equal(bucket_vec.begin(), bucket_vec.end(), output.begin()));
		ASSERT_TRUE(abs(bucket_value - v) <= se.getResolution());

		//Test bucket support
		vector<nupic::UInt> bucketIndices = se.getBucketIndices(v);

		topDown = se.getBucketInfo(bucketIndices);
		bucket_value = boost::any_cast<double>(get<0>(topDown[0]));
		bucket_scalar = boost::any_cast<double>(get<1>(topDown[0]));
		bucket_vec = get<2>(topDown[0]);

		ASSERT_TRUE(abs(bucket_scalar - v) <= se.getResolution() / 2);
		ASSERT_EQ(bucket_value, bucket_scalar);
		ASSERT_TRUE(equal(bucket_vec.begin(), bucket_vec.end(), output.begin()));

		// Next value
		v += se.getResolution() / 4;
	}

	// Make sure we can fill in holes
	vector<nupic::UInt> enc4({ 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1 });
	DecodeResult decoded = se.decode(enc4, "");
	map<string, RangeList> fieldsMap = decoded.getFields();
	ASSERT_EQ(fieldsMap.size(), 1/*, 0*/);

	for (auto &it : fieldsMap) {
		//double rangMin = it.second.getRange(0).first;
		//double rangMax = it.second.getRange(0).second;
		string desc = it.second.getDescription();
		ASSERT_EQ(1, it.second.size());
	}
 
	vector<nupic::UInt> enc5({ 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1 });
	decoded = se.decode(enc5, "");
	fieldsMap = decoded.getFields();
	ASSERT_EQ(fieldsMap.size(), 1/*, 0*/);
	for (auto &it : fieldsMap) {
		//double rangMin = it.second.getRange(0).first;
		//double rangMax = it.second.getRange(0).second;
		string desc = it.second.getDescription();
		ASSERT_EQ(1, it.second.size());
	}

	// Test min and max
	se_w = 3;
	se_n = 14;
	se_minVal = 1.0;
	se_maxVal = 10.0;
	se_name = "scalar";
	se_radius = 0.0;
	se_resolution = 0.0;
	se_periodic = false;
	se_forced = true;
	initSE();
	 
	vector<nupic::UInt> output({ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 });

	vector< tuple<boost::any, boost::any, vector<nupic::UInt>> >  decode = se.topDownCompute(output);
	double bucket_scalar = boost::any_cast<double>(get<1>(decode[0]));
	ASSERT_EQ(10, bucket_scalar);
	 
	vector<nupic::UInt> output2({ 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 });
	decode = se.topDownCompute(output2);
	bucket_scalar = boost::any_cast<double>(get<1>(decode[0]));
	ASSERT_EQ(1, bucket_scalar);

	// -------------------------------------------------------------------------
	// Test the input description generation and top-down compute on a small number
	//   non-periodic encoder
	se_w = 3;
	se_n = 15;
	se_minVal = .001;
	se_maxVal = .002;
	se_name = "scalar";
	se_radius = 0.0;
	se_resolution = 0.0;
	se_periodic = false;
	se_forced = true;
	initSE();

	cout << " \nTesting non-periodic encoder decoding resolution of " << se.getResolution() << endl;

	v = se.getMinVal();
	while (v < se.getMaxVal()) {
		vector<nupic::UInt> output = se.encode(v);
		decoded = se.decode(output, "");
		map<string, RangeList> fieldsMap = decoded.getFields();

		ASSERT_EQ(1, fieldsMap.size());

		for (auto &it : fieldsMap) {
			double rangMin = it.second.getRange(0).first;
			double rangMax = it.second.getRange(0).second;
			ASSERT_EQ(1, it.second.size());
			ASSERT_EQ(rangMin, rangMax);
			ASSERT_TRUE((rangMin - v) < se.getResolution());
		}

		decode = se.topDownCompute(output);
		//double bucket_value = boost::any_cast<double>(get<0>(decode[0]));
		double bucket_scalar = boost::any_cast<double>(get<1>(decode[0]));
		vector<nupic::UInt> bucket_vec = get<2>(decode[0]);
		ASSERT_TRUE(abs(bucket_scalar - v) <= se.getResolution() / 2);

		v += (se.getResolution() / 4);
	}

	// -------------------------------------------------------------------------
	// Test the input description generation on a large number, non-periodic encoder
	se_w = 3;
	se_n = 15;
	se_minVal = 1.0;
	se_maxVal = 1000000000.0;
	se_name = "scalar";
	se_radius = 0.0;
	se_resolution = 0.0;
	se_periodic = false;
	se_forced = true;
	initSE();

	cout << " \nTesting non-periodic encoder decoding resolution of " << se.getResolution() << endl;

	v = se.getMinVal();
	while (v < se.getMaxVal()) {
		vector<nupic::UInt> output = se.encode(v);
		decoded = se.decode(output, "");
		map<string, RangeList> fieldsMap = decoded.getFields();

		ASSERT_EQ(1, fieldsMap.size());

		for (auto &it : fieldsMap) {
			double rangMin = it.second.getRange(0).first;
			double rangMax = it.second.getRange(0).second;
			ASSERT_EQ(1, it.second.size());
			ASSERT_EQ(rangMin, rangMax);
			ASSERT_TRUE((rangMin - v) < se.getResolution());
		}

		decode = se.topDownCompute(output);
		//double bucket_value = boost::any_cast<double>(get<0>(decode[0]));
		double bucket_scalar = boost::any_cast<double>(get<1>(decode[0]));
		vector<nupic::UInt> bucket_vec = get<2>(decode[0]);
		ASSERT_TRUE(abs(bucket_scalar - v) <= se.getResolution() / 2);

		v += (se.getResolution() / 4);
	}
}

TEST(ScalarEncoder2Test, testScalarEncoder) {
	initSE();

	vector<nupic::UInt> empty = se.encode(SENTINEL_VALUE_FOR_MISSING_DATA);
	vector<int> expected(14);
	ASSERT_TRUE(equal(expected.begin(), expected.end(), empty.begin()));
}
