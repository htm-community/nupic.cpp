#include <time.h>
#include <ctime> 
#include "gtest/gtest.h"
/*
#include <nupic/encoders/MultiEncoder.hpp> 
#include <nupic/encoders/Utils.hpp> 

using namespace encoders;
using namespace std;

MultiEncoder<map<string, boost::any>> me;


void runScalarTests(MultiEncoder<map<string, boost::any>> me) {

	// should be 7 bits wide
	// use of forced=true is not recommended, but here for readability, see scalar.py	 
	vector<nupic::UInt> expected({ 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1 });				 

	map<string, boost::any> d;
	d["dow"] = 3.;
	d["myval"] = 10.;

	vector<nupic::UInt> output = me.encode(d);

	ASSERT_TRUE(equal(expected.begin(), expected.end(), output.begin()));

	//// Check decoding
	DecodeResult decoded = me.decode(output, "");

	map<string, RangeList> fields = decoded.getFields();
	ASSERT_EQ(fields.size(), 2);

	pair<double, double> minMax = fields["aux"].getRange(0);
	ASSERT_EQ(minMax.first, 10.0);
	ASSERT_EQ(minMax.second, 10.0);

	minMax = fields["day of week"].getRange(0);
	ASSERT_EQ(minMax.first, 3.0);
	ASSERT_EQ(minMax.second, 3.0);


}

void runMixedTests(MultiEncoder<map<string, boost::any>> me) {
	
	map<string, boost::any> d;
	d["dow"] = 4.;
	d["myval"] = 6.;
	d["myCat"] = string("pass");

	vector<nupic::UInt> output = me.encode(d);
	vector< tuple<boost::any, boost::any, vector<nupic::UInt>> > topDownOut = me.topDownCompute(output);

	ScalarEncoder2<boost::any>* dow = 0;
	ScalarEncoder2<boost::any>* myval = 0;
	CategoryEncoder<boost::any> * myCat = 0;
	tuple<boost::any, boost::any, vector<nupic::UInt>> dowActual, myvalActual, myCatActual;


	int idx = 0;
	for (tuple< string, Encoder<boost::any>*, nupic::UInt> t : me.getMultiEncoder()) {
		Encoder<boost::any>* enc = get<1>(t);
		string name = get<0>(t);
		if (name == "dow") {
			dow = dynamic_cast<ScalarEncoder2<boost::any> *>(enc);
			dowActual = topDownOut[idx];
			idx++;
		}
		if (name == "myval") {
			myval = dynamic_cast<ScalarEncoder2<boost::any> *>(enc);
			myvalActual = topDownOut[idx];
			idx++;
		}

		if (name == "myCat") {
			myCat = dynamic_cast<CategoryEncoder<boost::any> *>(enc);
			myCatActual = topDownOut[idx];
			idx++;
		}
	}


	tuple<boost::any, boost::any, vector<nupic::UInt>> dowExpected = dow->topDownCompute(dow->encode(4.))[0];
	tuple<boost::any, boost::any, vector<nupic::UInt>> myvalExpected = myval->topDownCompute(myval->encode(6.))[0];
	tuple<boost::any, boost::any, vector<nupic::UInt>> myCatExpected = myCat->topDownCompute(myCat->encode(string("pass")))[0];


	ASSERT_EQ(boost::any_cast<double>(get<0>(dowExpected)), boost::any_cast<double>(get<0>(dowActual)));
	ASSERT_EQ(boost::any_cast<double>(get<1>(dowExpected)), boost::any_cast<double>(get<0>(dowActual)));
	ASSERT_TRUE(equal(get<2>(dowActual).begin(), get<2>(dowActual).end(), get<2>(dowExpected).begin()));

	ASSERT_EQ(boost::any_cast<double>(get<0>(myvalExpected)), boost::any_cast<double>(get<0>(myvalActual)));
	ASSERT_EQ(boost::any_cast<double>(get<1>(myvalExpected)), boost::any_cast<double>(get<1>(myvalActual)));
	ASSERT_TRUE(equal(get<2>(myvalActual).begin(), get<2>(myvalActual).end(), get<2>(myvalExpected).begin()));

	ASSERT_EQ(boost::any_cast<string>(get<0>(myCatExpected)), boost::any_cast<string>(get<0>(myCatActual)));
	ASSERT_EQ(boost::any_cast<nupic::UInt>(get<1>(myCatExpected)), boost::any_cast<nupic::UInt>(get<1>(myCatActual)));
	ASSERT_TRUE(equal(get<2>(myCatActual).begin(), get<2>(myCatActual).end(), get<2>(myCatExpected).begin()));

}
 

TEST(MultiEncoderTest, testMultiEncoder) {

	ScalarEncoder2<boost::any> scalar_;
	scalar_.init(3, 0, 1, 8, "day of week", 0, 1, true, true);
	me.addEncoder(dynamic_cast<Encoder<boost::any> *>(&scalar_), "dow");

	ScalarEncoder2<boost::any> scalar_2;
	scalar_2.init(5, 0, 1, 10, "aux", 0, 1, false, true);
	me.addEncoder(dynamic_cast<Encoder<boost::any> *>(&scalar_2), "myval");

	cout << "--- run scalar Tests " << endl;
	runScalarTests(me);
	

	ASSERT_EQ(me.getWidth(), 21);

	vector<tuple<string, int>> desc;
	desc.push_back(tuple<string, int>("day of week", 0));
	desc.push_back(tuple<string, int>("aux", 7));

	for (int i = 0; i < (int)me.getDescription().size(); i++) {
		ASSERT_EQ(get<0>(me.getDescription()[i]), get<0>(desc[i]));
		ASSERT_EQ(get<1>(me.getDescription()[i]), get<1>(desc[i]));
	}
	

	vector<string> categories({ "run", "pass", "kick" });	 

	CategoryEncoder<boost::any> cat;
	cat.setCategoryList(categories);
	cat.init(3, 2, false, true);
	me.addEncoder(dynamic_cast<Encoder<boost::any> *>(&cat), "myCat");

	cout << "--- run mixed Tests " << endl;
	runMixedTests(me);
}


TEST(MultiEncoderTest, testDateEncoder_) {
	std::cout << "\n\n --------test DateEncoder holiday--------- \n" << std::endl;

	DateEncoder<boost::any> deHoliday;
	deHoliday.setForced(true);
	deHoliday.setHoliday(tuple<int, double>(5, (double)get<1>(deHoliday.getHoliday())));
	deHoliday.init();

	MultiEncoder<map<string, boost::any>> multi;
	multi.addEncoder(dynamic_cast<Encoder<boost::any> *>(&deHoliday), "holiday");


	time_t d_hol = Utils<int>::makedate(2010, 12, 25, 4, 55, -1);
	map<string, boost::any> d;
	d["holiday"] = d_hol;

	vector<nupic::UInt> encHoliday = multi.encode(d);
	int holidayArr[] = { 0,0,0,0,0,1,1,1,1,1,  1,1,1,1,1,0,0,0,0,0 };
	vector<int> holiday(begin(holidayArr), end(holidayArr));
	ASSERT_TRUE(equal(encHoliday.begin(), encHoliday.end(), holiday.begin()));


	d_hol = Utils<int>::makedate(2008, 12, 27, 4, 55, -1);
	d["holiday"] = d_hol;

	vector<nupic::UInt> encNotHoliday = multi.encode(d);
	int notholidayArr[] = { 1,1,1,1,1,0,0,0,0,0 };
	vector<int> notholiday(begin(notholidayArr), end(notholidayArr));
	ASSERT_TRUE(equal(encNotHoliday.begin(), encNotHoliday.end(), notholiday.begin()));
}
 

TEST(MultiEncoderTest, TestAdaptiveScalarEncoder) {
	cout << "\n\n --------test AdaptiveScalarEncoder Fill Holes--------- \n" << endl;
	
	AdaptiveScalarEncoder<boost::any> ase;
	ase.init(3, 14, 1, 8, "adaptiv", 1.5, 0.5, true);
	MultiEncoder<map<string, boost::any>> multi;
	multi.addEncoder(dynamic_cast<Encoder<boost::any> *>(&ase), "adaptiv");

	map<string, boost::any> d;

	d["adaptiv"] = 1.0; 
	vector<nupic::UInt> res = multi.encode(d); 
	vector<nupic::UInt> inputArray1({ 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 });
	ASSERT_TRUE(equal(inputArray1.begin(), inputArray1.end(), res.begin())); 

	d["adaptiv"] = 2.0; 
	res = multi.encode(d); 
	vector<nupic::UInt> inputArray2({ 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 });
	ASSERT_TRUE(equal(inputArray2.begin(), inputArray2.end(), res.begin()));

	d["adaptiv"] = 8.0;
	res = multi.encode(d);
	vector<nupic::UInt> inputArray3({ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1 });
	ASSERT_TRUE(equal(inputArray3.begin(), inputArray3.end(), res.begin()));
}

int computeOverlapME(vector<nupic::UInt> result1, vector<nupic::UInt> result2) {
	if (result1.size() != result2.size())
		return 0;

	int overlap = 0;
	for (int i = 0; i < (int)result1.size(); i++)
		if (result1[i] == 1 && result2[i] == 1)
			overlap++;

	return overlap;
}

int getOnBitsME(vector<nupic::UInt> input) {
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

TEST(MultiEncoderTest, testRandomDistributedScalarEncoder) {
	cout << "\n\n --------test RandomDistributedScalarEncoder encode--------- \n" << endl;

	RandomDistributedScalarEncoder<boost::any> rdse;
	rdse.init(1.0, 23, 500, "encoder", 0.0, rdse.getSeed());

	MultiEncoder<map<string, boost::any>> multi;
	multi.addEncoder(dynamic_cast<Encoder<boost::any> *>(&rdse), "rdse");
	
	map<string, boost::any> d;

	d["rdse"] = -0.1;
	vector<nupic::UInt> e0 = multi.encode(d);
	ASSERT_EQ(getOnBitsME(e0), 23) << "Number of on bits is incorrect";
	ASSERT_EQ(e0.size(), 500) << "Width of the vector is incorrect";
	ASSERT_EQ(rdse.getBucketIndices(0)[0], rdse.getMaxBuckets() / 2) << "Offset doesn't correspond to middle bucket";
	ASSERT_EQ(1, rdse.getBucketMap().size()) << "Number of buckets is not 1	";
	  
	d["rdse"] = 1.0;
	vector<nupic::UInt> e1 = multi.encode(d);
	ASSERT_EQ(2, rdse.getBucketMap().size()) << "Number of buckets is not 2";
	ASSERT_EQ(getOnBitsME(e1), 23) << "Number of on bits is incorrect";
	ASSERT_EQ(e0.size(), 500) << "Width of the vector is incorrect";
	ASSERT_EQ(computeOverlapME(e0, e1), 22) << "Overlap is not equal to w-1";
	 
	d["rdse"] = 25.0;
	vector<nupic::UInt> e25 = multi.encode(d);
	ASSERT_TRUE(rdse.getBucketMap().size() > 23) << "Buckets created are not more than 23";
	ASSERT_EQ(getOnBitsME(e1), 23) << "Number of on bits is incorrect";
	ASSERT_EQ(e0.size(), 500) << "Width of the vector is incorrect";
	ASSERT_TRUE(computeOverlapME(e0, e25) < 4) << "Overlap is too high";
 
	ASSERT_TRUE(equal(e0.begin(), e0.end(), rdse.encode(-0.1).begin())) << "Encodings are not consistent - they have changed after new buckets have been created";
	ASSERT_TRUE(equal(e1.begin(), e1.end(), rdse.encode(1.0).begin())) << "Encodings are not consistent - they have changed after new buckets have been created";
}


double overlap(vector<nupic::UInt> sdr1, vector<nupic::UInt> sdr2) {
	assert(sdr1.size() == sdr2.size());
	nupic::UInt sum = Utils<nupic::UInt>::sum(Utils<nupic::UInt>::and_(sdr1, sdr2));

	return (double)sum / (double)Utils<nupic::UInt>::sum(sdr1);
}

TEST(MultiEncoderTest, testCoordinateEncoder) {
	std::cout << "\n ---test CoordinateEncoder encodeIntoArray !" << std::endl;

	CoordinateEncoder<boost::any> ce;
	ce.setN(33);
	ce.setW(3);
	ce.setName("coordinate");
	MultiEncoder<map<string, boost::any>> multi;
	multi.addEncoder(dynamic_cast<Encoder<boost::any> *>(&ce), "ce");

	map<string, boost::any> d;

	d["ce"] = tuple<vector<int>, double>(vector<int>({ 100, 200 }), 5.0);
	vector<nupic::UInt> output1 = multi.encode(d);
	ASSERT_EQ(Utils<nupic::UInt>::sum(output1), ce.getW());
	vector<nupic::UInt> output2 = multi.encode(d);
	ASSERT_TRUE(equal(output1.begin(), output1.end(), output2.begin()));

	ce.setN(1999);
	ce.setW(25);
	ce.setRadius(2);
	multi.addEncoder(dynamic_cast<Encoder<boost::any> *>(&ce), "ce");

	d["ce"] = tuple<vector<int>, double>(vector<int>({ 0, 0 }), 2);
	vector<nupic::UInt> outputA = multi.encode(d);
	d["ce"] = tuple<vector<int>, double>(vector<int>({ 0, 1 }), 2);
	vector<nupic::UInt> outputB = multi.encode(d);

	ASSERT_EQ(0.8, overlap(outputA, outputB));
}
 
TEST(MultiEncoderTest, testPassThroughEncoder_) {
	cout << "\n ---test PassThroughEncoder EncodeArray--- \n" << endl;

	PassThroughEncoder<boost::any> pte;
	pte.init(9, 1, "foo");

	MultiEncoder<map<string, boost::any>> multi;
	multi.addEncoder(dynamic_cast<Encoder<boost::any> *>(&pte), "pte");

	vector<nupic::UInt> bitmap({ 0,0,0,1,0,0,0,0,0 });
	map<string, boost::any> d;
	d["pte"] = bitmap;

	vector<nupic::UInt> pteOutput = multi.encode(d);
	ASSERT_EQ(Utils<nupic::UInt>::sum(bitmap), Utils<nupic::UInt>::sum(pteOutput));
}

TEST(MultiEncoderTest, testSparsePassThroughEncoder_) {
	cout << "\n ---test SparsePassThroughEncoder EncodeArray--- \n" << endl;

	SparsePassThroughEncoder<boost::any> spte;
	spte.init(24, 5, "foo");

	MultiEncoder<map<string, boost::any>> multi;
	multi.addEncoder(dynamic_cast<Encoder<boost::any> *>(&spte), "spte");

	vector<nupic::UInt> bitmap({ 2, 7, 15, 18, 23 });
	map<string, boost::any> d;
	d["spte"] = bitmap;

	vector<nupic::UInt> spteOutput = multi.encode(d);
	ASSERT_EQ(bitmap.size(), Utils<nupic::UInt>::sum(spteOutput));
}

TEST(MultiEncoderTest, testSDRCategoryEncoder) {
	cout << "\n ---test SDRCategoryEncoder EncodeArray--- \n" << endl;
 
	int fieldWidth = 100;
	int bitsOn = 10;
	vector<string> categories({ "ES", "S1", "S2" });

	SDRCategoryEncoder<boost::any> sdrce;
	sdrce.init(100, 10, categories, "foo", 1, true);
	MultiEncoder<map<string, boost::any>> multi;
	multi.addEncoder(dynamic_cast<Encoder<boost::any> *>(&sdrce), "sdrce");

	map<string, boost::any> d;
	d["sdrce"] = string("ES");

	vector<nupic::UInt> es = multi.encode(d);
	ASSERT_EQ(Utils<nupic::UInt>::sum(es), bitsOn);
	ASSERT_EQ(es.size(), fieldWidth);
}

*/ //FIXME reenable MultiEnc - conflicts boost::any_cast in Coord & PassThru encoders

