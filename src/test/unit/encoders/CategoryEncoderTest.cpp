#include <time.h>
#include <ctime> 
#include "gtest/gtest.h"

#include <nupic/encoders/Encoder.hpp>
#include <nupic/encoders/ScalarEncoder.hpp>
#include <nupic/encoders/CategoryEncoder.hpp>
#include <nupic/encoders/DecodeResult.hpp>
#include <nupic/encoders/Utils.hpp>

using namespace encoders;
using namespace std;

CategoryEncoder<string> cae; 

int ce_w				= 3;  
double ce_radius		= 1.0;
bool ce_periodic		= false;
bool ce_forced			= true;

void setUpCAE() {
	cae.init(ce_w, ce_radius, ce_periodic, ce_forced);
}

TEST(CategoryEncoderTest, testCategoryEncoder) {
	cout << "\n ------ testCategoryEncoder ------ \n" << endl;

	vector<string> categories( { "ES", "GB", "US" } );
	cae.setCategoryList(categories);
	setUpCAE(); 	 

	vector<nupic::UInt> output = cae.encode("US");

	vector<nupic::UInt> tempArr({ 0,0,0,0,0,0,0,0,0,1,1,1 });
	ASSERT_TRUE(equal(tempArr.begin(), tempArr.end(), output.begin()));

	// Test reverse lookup
	DecodeResult decoded = cae.decode(output, "");
	map<string, RangeList> fieldsMap = decoded.getFields();
	ASSERT_EQ(fieldsMap.size(), 1);

	for (auto &it : fieldsMap){ 
		double rangMin = it.second.getRange(0).first;
		double rangMax = it.second.getRange(0).second;
		ASSERT_EQ(1, it.second.size());
		ASSERT_EQ(rangMin, rangMax);
		ASSERT_TRUE(rangMin == 3 && rangMax == 3); 
		//LOGGER.info("decodedToStr of " + minMax + "=>" + cae.decodedToStr(decoded));
	}		

	// Test topdown compute
	for(string v : categories) {
		output = cae.encode(v); 
		vector< tuple<boost::any, boost::any, vector<nupic::UInt>> > topDown = cae.topDownCompute(output); 
		string bucket_value	= boost::any_cast<string>(get<0>(topDown[0])); 
		nupic::UInt bucket_scalar	= boost::any_cast<nupic::UInt>(get<1>(topDown[0])); 

		ASSERT_EQ(v , bucket_value); 
		ASSERT_EQ((int)cae.getScalars(v)[0], bucket_scalar); 
		
		vector<nupic::UInt> bucketIndices = cae.getBucketIndices(v);
		//cout << "bucket index => " << bucketIndices[0] << endl;

		topDown = cae.getBucketInfo(bucketIndices);
		bucket_value	= boost::any_cast<string>(get<0>(topDown[0]));
		bucket_scalar	= boost::any_cast<nupic::UInt>(get<1>(topDown[0]));
		vector<nupic::UInt> bucket_vec	=  get<2>(topDown[0]); 

		ASSERT_EQ(v , bucket_value);
		ASSERT_EQ((int)cae.getScalars(v)[0], bucket_scalar);
		ASSERT_TRUE(equal(bucket_vec.begin(), bucket_vec.end(), output.begin()));	
		ASSERT_EQ(bucket_value, cae.getBucketValues()[bucketIndices[0]]);
	} 

	//--------------
	// unknown category
	output = cae.encode("NA");
	vector<nupic::UInt> tempArr2( { 1,1,1,0,0,0,0,0,0,0,0,0 } );
	ASSERT_TRUE(equal(output.begin(), output.end(), tempArr2.begin()));

	// Test reverse lookup
	decoded = cae.decode(output, "");
	fieldsMap = decoded.getFields();
	ASSERT_EQ(fieldsMap.size(), 1);

	for (auto &it : fieldsMap){ 
		double rangMin = it.second.getRange(0).first;
		double rangMax = it.second.getRange(0).second;
		ASSERT_EQ(1, it.second.size());
		ASSERT_EQ(rangMin, rangMax);
		ASSERT_TRUE(rangMin == 0 && rangMax == 0);
	}		
	vector< tuple<boost::any, boost::any, vector<nupic::UInt>> > topDown = cae.topDownCompute(output);
	string bucket_value	= boost::any_cast<string>(get<0>(topDown[0]));
	nupic::UInt bucket_scalar	= boost::any_cast<nupic::UInt>(get<1>(topDown[0]));

	ASSERT_EQ("<UNKNOWN>" , bucket_value);
	ASSERT_EQ(0 , bucket_scalar);


	//--------------
	// ES
	output = cae.encode("ES");
	vector<nupic::UInt> tempArr3( { 0,0,0,1,1,1,0,0,0,0,0,0 } );
	ASSERT_TRUE(equal(output.begin(), output.end(), tempArr3.begin()));


	// MISSING VALUE
	vector<nupic::UInt> outputForMissing = cae.encode("");
	vector<nupic::UInt> tempArr4( { 0,0,0,0,0,0,0,0,0,0,0,0 } );
	ASSERT_TRUE(equal(outputForMissing.begin(), outputForMissing.end(), tempArr4.begin()));


	// Test reverse lookup
	decoded = cae.decode(output, "");
	fieldsMap = decoded.getFields();
	ASSERT_EQ(fieldsMap.size(), 1);

	for (auto &it : fieldsMap){ 
		double rangMin = it.second.getRange(0).first;
		double rangMax = it.second.getRange(0).second;
		ASSERT_EQ(1, it.second.size());
		ASSERT_EQ(rangMin, rangMax);
		ASSERT_TRUE(rangMin == 1 && rangMax == 1);
	}		

	// Test topdown compute
	topDown = cae.topDownCompute(output);
	bucket_value	= boost::any_cast<string>(get<0>(topDown[0]));
	bucket_scalar	= boost::any_cast<nupic::UInt>(get<1>(topDown[0]));

	ASSERT_EQ("ES" , bucket_value);
	ASSERT_EQ((int)cae.getScalars("ES")[0] , bucket_scalar);

	 
	//--------------
	// Multiple categories
	fill(output.begin(), output.end(), 1);

	// Test reverse lookup
	decoded = cae.decode(output, "");
	fieldsMap = decoded.getFields();
	ASSERT_EQ(fieldsMap.size(), 1);

	for (auto &it : fieldsMap){ 
		double rangMin = it.second.getRange(0).first;
		double rangMax = it.second.getRange(0).second;
		ASSERT_EQ(1, it.second.size());
		ASSERT_TRUE(rangMin != rangMax);
		ASSERT_TRUE(rangMin == 0 && rangMax == 3);
	}		

}


TEST(CategoryEncoderTest, testWidth) {

	cout << "\n ------ testWidth ------ \n" << endl;

	CategoryEncoder<string> ce2;  

	// Test with width = 1
	vector<string> categories( { "cat1", "cat2", "cat3", "cat4", "cat5" } );
	ce2.setCategoryList(categories);
	ce2.init(ce_w, ce_radius, ce_periodic, ce_forced); 

	for(string cat : categories) { 
		vector<nupic::UInt> output = ce2.encode(cat);
		vector< tuple<boost::any, boost::any, vector<nupic::UInt>> > topDown = ce2.topDownCompute(output);
		string bucket_value	= boost::any_cast<string>(get<0>(topDown[0]));
		nupic::UInt bucket_scalar	= boost::any_cast<nupic::UInt>(get<1>(topDown[0]));

		ASSERT_EQ(cat , bucket_value);
		ASSERT_EQ((int)ce2.getScalars(cat)[0], bucket_scalar);
	}

	//==================
	// Test with width = 9, removing some bits in the encoded output
	categories.resize(9);
	for(int i = 0;i < 9;i++){
		stringstream ss;
		ss << "cat" << (i + 1);
		categories[i] = ss.str() ;
	} 

	CategoryEncoder<string> ce3;  
	//forced: is not recommended, but is used here for readability.
	ce3.setCategoryList(categories);
	ce_w = 9;
	ce_radius = 1;
	ce_forced = true;
	ce3.init(ce_w, ce_radius, ce_periodic, ce_forced);

	for(string cat : categories) {
		vector<nupic::UInt>output = ce3.encode(cat);
		vector< tuple<boost::any, boost::any, vector<nupic::UInt>> > topDown = ce3.topDownCompute(output);
		string bucket_value	= boost::any_cast<string>(get<0>(topDown[0]));
		nupic::UInt bucket_scalar	= boost::any_cast<nupic::UInt>(get<1>(topDown[0]));

		ASSERT_EQ(cat , bucket_value);
		ASSERT_EQ((int)ce3.getScalars(cat)[0], bucket_scalar);


		// Get rid of 1 bit on the left
		vector<nupic::UInt>  outputNZs = Utils<nupic::UInt>::where(output, Utils<nupic::UInt>::WHERE_1);
		output[outputNZs[0]] = 0;
		topDown = ce3.topDownCompute(output);
		bucket_value	= boost::any_cast<string>(get<0>(topDown[0]));
		bucket_scalar	= boost::any_cast<nupic::UInt>(get<1>(topDown[0]));

		ASSERT_EQ(cat , bucket_value);
		ASSERT_EQ((int)ce3.getScalars(cat)[0], bucket_scalar);


		// Get rid of 1 bit on the right
		output[outputNZs[0]] = 1;
		output[outputNZs[outputNZs.size() - 1]] = 0;

		topDown = ce3.topDownCompute(output);
		bucket_value	= boost::any_cast<string>(get<0>(topDown[0]));
		bucket_scalar	= boost::any_cast<nupic::UInt>(get<1>(topDown[0]));
 
		ASSERT_EQ(cat , bucket_value);
		ASSERT_EQ((int)ce3.getScalars(cat)[0], bucket_scalar);		 


		// Get rid of 4 bits on the left
		fill(output.begin(), output.end(), 0);

		vector<int> indexes = Utils<int>::range(outputNZs[outputNZs.size() - 5], outputNZs[outputNZs.size() - 1] + 1);
		for(int i = 0; i < (int)indexes.size(); i++) output[indexes[i]] = 1;
		
		topDown = ce3.topDownCompute(output);
		bucket_value	= boost::any_cast<string>(get<0>(topDown[0]));
		bucket_scalar	= boost::any_cast<nupic::UInt>(get<1>(topDown[0]));

		ASSERT_EQ(cat , bucket_value);
		ASSERT_EQ((int)ce3.getScalars(cat)[0], bucket_scalar);


		// Get rid of 4 bits on the right
		fill(output.begin(), output.end(), 0);
		indexes = Utils<int>::range(outputNZs[0], outputNZs[5]);
		for(int i = 0; i < (int)indexes.size(); i++) output[indexes[i]] = 1;
		
		topDown = ce3.topDownCompute(output);
		bucket_value	= boost::any_cast<string>(get<0>(topDown[0]));
		bucket_scalar	= boost::any_cast<nupic::UInt>(get<1>(topDown[0]));

		ASSERT_EQ(cat , bucket_value);
		ASSERT_EQ((int)ce3.getScalars(cat)[0], bucket_scalar);		 

	} 

	vector<nupic::UInt> output1 = ce3.encode("cat1");
	vector<nupic::UInt> output2 = ce3.encode("cat9");
	vector<nupic::UInt> output = Utils<nupic::UInt>::or_(output1, output2);				
	vector< tuple<boost::any, boost::any, vector<nupic::UInt>> > topDown = ce3.topDownCompute(output);
	nupic::UInt	bucket_scalar	= boost::any_cast<nupic::UInt>(get<1>(topDown[0]));

	ASSERT_TRUE((int)ce3.getScalars("cat1")[0] == (int)bucket_scalar ||
				(int)ce3.getScalars("cat9")[0] == (int)bucket_scalar);
	 
}
