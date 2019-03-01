#include <time.h>
#include <ctime> 
#include "gtest/gtest.h"

#include <nupic/encoders/SDRCategoryEncoder.hpp>
#include <nupic/encoders/Utils.hpp>
#include "nupic/utils/Random.hpp"  
 
using namespace encoders;
using namespace std;


TEST(SDRCategoryEncoderTest, testSDRCategoryEncoder) {

	vector<string> categories( {"ES", "S1", "S2", "S3", "S4", "S5", "S6", "S7", "S8",
		   					    "S9", "S10", "S11", "S12", "S13", "S14", "S15", "S16",
								"S17", "S18", "S19", "GB", "US"} );
	
	int fieldWidth = 100;
	int bitsOn = 10;

	int sdrce_n		= fieldWidth;
	int sdrce_w		= bitsOn;  
	string name		= "foo";
	int seed		= 1;
	bool sdrce_forced	= true;

	SDRCategoryEncoder<string> sdrce;
	sdrce.init(sdrce_n, sdrce_w, categories, name, seed, sdrce_forced);

	//----------
	//internal check
	ASSERT_EQ( sdrce.getSDRs().size(), 23);
	for( auto &it : sdrce.getSDRs()){
		ASSERT_EQ( it.second.size(), fieldWidth);
	}

	//----------
	//ES
	vector<nupic::UInt> es = sdrce.encode("ES");
	ASSERT_EQ( Utils<nupic::UInt>::sum(es), bitsOn); 
	ASSERT_EQ(es.size(), fieldWidth);

	DecodeResult x = sdrce.decode(es, "");  
	ASSERT_EQ(x.getDescriptions()[0], "foo"); 
	ASSERT_EQ(x.getFields()["foo"].getDescription(), "ES");

	vector< tuple<boost::any, boost::any, vector<nupic::UInt>> > topDown = sdrce.topDownCompute(es);
	string bucket_value	         = boost::any_cast<string>(get<0>(topDown[0]));
	nupic::UInt bucket_scalar	 = boost::any_cast<nupic::UInt>(get<1>(topDown[0]));
	vector<nupic::UInt> encoding =  get<2>(topDown[0]); 
	ASSERT_EQ(bucket_value, "ES");
	ASSERT_EQ(bucket_scalar, 1);
	ASSERT_EQ( Utils<nupic::UInt>::sum(encoding), bitsOn);

	//----------
	//Test topDown compute
	for (string category : categories) {
		vector<nupic::UInt> output = sdrce.encode(category);
		topDown = sdrce.topDownCompute(output);
		ASSERT_EQ(boost::any_cast<string>(get<0>(topDown[0])), category);
		ASSERT_EQ(boost::any_cast<nupic::UInt>(get<1>(topDown[0])), (nupic::UInt)sdrce.getScalars(category)[0]);
		ASSERT_TRUE(equal(output.begin(), output.end(), get<2>(topDown[0]).begin()));	 
		vector<nupic::UInt> bucketIndices = sdrce.getBucketIndices(category);		 
		//cout << "bucket index =>" << bucketIndices[0] << endl;
	}   

	//----------
	//Unknown
	vector<nupic::UInt> unknown = sdrce.encode("ASDFLKJLK");
	ASSERT_EQ( Utils<nupic::UInt>::sum(unknown), bitsOn);
	ASSERT_EQ(unknown.size(), fieldWidth);
	x = sdrce.decode(unknown, "");
	ASSERT_EQ(x.getFields()["foo"].getDescription(), "<UNKNOWN>");

	topDown = sdrce.topDownCompute(unknown);
	ASSERT_EQ(boost::any_cast<string>(get<0>(topDown[0])), "<UNKNOWN>");
	ASSERT_EQ(boost::any_cast<nupic::UInt>(get<1>(topDown[0])), 0);

	//----------
	//US
	vector<nupic::UInt> us = sdrce.encode("US");
	ASSERT_EQ( Utils<nupic::UInt>::sum(us), bitsOn);
	ASSERT_EQ(us.size(), fieldWidth);
	ASSERT_EQ( Utils<nupic::UInt>::sum(us), bitsOn);
	x = sdrce.decode(us, "");
	ASSERT_EQ(x.getFields()["foo"].getDescription(), "US");

	topDown = sdrce.topDownCompute(us);
	ASSERT_EQ(boost::any_cast<string>(get<0>(topDown[0])), "US");
	ASSERT_EQ(boost::any_cast<nupic::UInt>(get<1>(topDown[0])), categories.size());
	ASSERT_EQ( Utils<nupic::UInt>::sum(get<2>(topDown[0])), bitsOn);

	//----------
	// empty field
	vector<nupic::UInt> empty = sdrce.encode(""); 
	ASSERT_EQ( Utils<nupic::UInt>::sum(empty), 0);
	ASSERT_EQ(empty.size(), fieldWidth);

	//----------
	//make sure it can still be decoded after a change
	nupic::Random random_;
	int bit = random_.getUInt32((sdrce.getWidth() - 1));     
	us[bit] = 1 - us[bit];
	x = sdrce.decode(us, "");
	ASSERT_EQ(x.getFields()["foo"].getDescription(), "US");

	//add two reps together
	vector<nupic::UInt> newrep = Utils<nupic::UInt>::or_(unknown, us);
	x = sdrce.decode(newrep, "");
	string name_ = x.getFields()["foo"].getDescription();

	if ("US <UNKNOWN>" == name_ && "<UNKNOWN> US" == name_) {
		string othercategory = name_.replace(name_.begin(), name_.end(),"US", "");
		othercategory = othercategory.replace(othercategory.begin(), othercategory.end(),"<UNKNOWN>", "");
		othercategory = othercategory.replace(othercategory.begin(), othercategory.end()," ", "");
		vector<nupic::UInt> otherencoded = sdrce.encode(othercategory);
		cout << "Decoding failure" << endl;
		exit(-1);
	}


	//----------
	//Test autogrow
	SDRCategoryEncoder<string> sdrce2;
	sdrce2.init(sdrce_n, sdrce_w, categories, "bar", seed, sdrce_forced);

	es = sdrce2.encode("ES");
	ASSERT_EQ( Utils<nupic::UInt>::sum(es), bitsOn); 
	ASSERT_EQ(es.size(), fieldWidth);
	x = sdrce2.decode(es, "");
	ASSERT_EQ(x.getDescriptions()[0], "bar");
	ASSERT_EQ(x.getFields()["bar"].getDescription(), "ES");

	us = sdrce2.encode("US");
	ASSERT_EQ(Utils<nupic::UInt>::sum(us), bitsOn);
	ASSERT_EQ(us.size(), (fieldWidth));
	x = sdrce2.decode(us, "");
	ASSERT_EQ(x.getDescriptions()[0], "bar");
	ASSERT_EQ(x.getFields()["bar"].getDescription(), "US");

	vector<nupic::UInt> es2 = sdrce2.encode("ES");
	ASSERT_TRUE(equal(es.begin(), es.end(), es2.begin()));

	vector<nupic::UInt> us2 = sdrce2.encode("US");
	ASSERT_TRUE(equal(us.begin(), us.end(), us2.begin()));
	
	//make sure it can still be decoded after a change 
	bit = random_.getUInt32((sdrce2.getWidth() - 1));
	us[bit] = 1 - us[bit];
	x = sdrce2.decode(us, "");
	ASSERT_EQ(x.getFields()["bar"].getDescription(), "US");

	// add two reps together
	newrep = Utils<nupic::UInt>::or_(us, es);
	x = sdrce2.decode(newrep, "");
	name_ = x.getFields()["bar"].getDescription();
	ASSERT_TRUE("US ES" == name_ || "ES US" == name_);

	// Catch duplicate categories
	SDRCategoryEncoder<string> sdrce3;
	bool caughtException = false;
	vector<string> newCategories = categories;
	newCategories.push_back("ES");
	try {
		sdrce3.init(sdrce_n, sdrce_w, newCategories, "foo", seed, sdrce_forced);
	}
	catch (const std::invalid_argument& e) {
		caughtException = true;
	}
	if (!caughtException) {
		throw std::invalid_argument("Did not catch duplicate category in constructor");
	}
}

TEST(SDRCategoryEncoderTest, testAutoGrow) {

	//testing auto-grow
	int fieldWidth = 100;
	int bitsOn = 10;

	int sdrce_n		= fieldWidth;
	int sdrce_w		= bitsOn;  
	string name		= "foo";
	int seed		= 1;
	bool sdrce_forced	= true;
	vector<string> categories;

	SDRCategoryEncoder<string> sdrce;
	sdrce.init(sdrce_n, sdrce_w, categories, name, seed, sdrce_forced);

	vector<nupic::UInt> encoded(fieldWidth, 0);

	vector< tuple<boost::any, boost::any, vector<nupic::UInt>> >  topDown = sdrce.topDownCompute(encoded);
	string bucket_value		= boost::any_cast<string>(get<0>(topDown[0]));
	ASSERT_EQ( bucket_value, "<UNKNOWN>");

	//----------
	// catA
	sdrce.encodeIntoArray("catA", encoded);
	ASSERT_EQ( Utils<nupic::UInt>::sum(encoded), bitsOn);
	ASSERT_EQ((double)sdrce.getScalars("catA")[0], 1.0);
	vector<nupic::UInt> catA = encoded;

	//----------
	//catB
	sdrce.encodeIntoArray("catB", encoded);
	ASSERT_EQ( Utils<nupic::UInt>::sum(encoded), bitsOn);
	ASSERT_EQ((double)sdrce.getScalars("catB")[0], 2.0);
	vector<nupic::UInt> catB = encoded;

	ASSERT_EQ(boost::any_cast<string>(get<0>(sdrce.topDownCompute(catA)[0])), "catA");
	ASSERT_EQ(boost::any_cast<string>(get<0>(sdrce.topDownCompute(catB)[0])), "catB");

	//----------
	// empty field
	sdrce.encodeIntoArray("", encoded);
	ASSERT_EQ( Utils<nupic::UInt>::sum(encoded), 0);
	ASSERT_EQ(boost::any_cast<string>(get<0>(sdrce.topDownCompute(encoded)[0])), "<UNKNOWN>");

	//----------
	//Test Disabling Learning and autogrow
	sdrce.setLearning(false);								 
	sdrce.encodeIntoArray("catC", encoded);
	ASSERT_EQ( Utils<nupic::UInt>::sum(encoded), bitsOn);
	ASSERT_EQ((double)sdrce.getScalars("catC")[0], 0.0); 
	ASSERT_EQ(boost::any_cast<string>(get<0>(sdrce.topDownCompute(encoded)[0])), "<UNKNOWN>");

	sdrce.setLearning(true);		
	sdrce.encodeIntoArray("catC", encoded);
	ASSERT_EQ( Utils<nupic::UInt>::sum(encoded), bitsOn);
	ASSERT_EQ((double)sdrce.getScalars("catC")[0], 3.0); 
	ASSERT_EQ(boost::any_cast<string>(get<0>(sdrce.topDownCompute(encoded)[0])), "catC");
}
