#include <nupic/encoders/Encoder.hpp>
#include <nupic/encoders/PassThroughEncoder.hpp> 
#include <nupic/encoders/Utils.hpp>
#include "gtest/gtest.h"

using namespace encoders;
using namespace std;

PassThroughEncoder<vector<nupic::UInt>> pte;

void testCloseInner(vector<nupic::UInt> bitmap1, vector<nupic::UInt> bitmap2, double expectedScore){
	 
	PassThroughEncoder<vector<nupic::UInt>> encoder;
	encoder.init(9, Utils<nupic::UInt>::where(bitmap1, Utils<nupic::UInt>::WHERE_1).size(), "foo");

	vector<nupic::UInt> out1 = encoder.encode(bitmap1);
	encoder.setW(Utils<nupic::UInt>::where(bitmap2, Utils<nupic::UInt>::WHERE_1).size());
	vector<nupic::UInt> out2 = encoder.encode(bitmap2);

	vector<double> result = encoder.closenessScores(Utils<nupic::UInt>::toDoubleArray(out1), Utils<nupic::UInt>::toDoubleArray(out2), true);   
	ASSERT_TRUE(result.size() == 1 );
	ASSERT_EQ(expectedScore, result[0]);
}


TEST(PassThroughEncoderTest,testEncodeBitArray) { 
	cout << "\n ---testEncodeBitArray--- \n" << endl;

	pte.init(9,2,"foo");
	vector<nupic::UInt> bitmap({0,0,0,1,0,1,0,0,0});
	vector<nupic::UInt> output = pte.encode(bitmap);
	ASSERT_EQ(Utils<nupic::UInt>::sum(bitmap), Utils<nupic::UInt>::sum(output));

	pte.init(9, Utils<nupic::UInt>::where(output, Utils<nupic::UInt>::WHERE_1).size(), "foo");	 
	output = pte.encode(bitmap);
	ASSERT_EQ(Utils<nupic::UInt>::sum(bitmap), Utils<nupic::UInt>::sum(output));
}

TEST(PassThroughEncoderTest,testEncodeArray) { 
	cout << "\n ---testEncodeArray--- \n" << endl;

	pte.init(9,1,"foo");
	vector<nupic::UInt> bitmap({0,0,0,1,0,0,0,0,0});
	vector<nupic::UInt> output(9, 0);

	pte.encodeIntoArray(bitmap,output);
	ASSERT_EQ(Utils<nupic::UInt>::sum(bitmap), Utils<nupic::UInt>::sum(output));

	DecodeResult decode = pte.decode(output, "");
	map<string, RangeList> fields = decode.getFields();
	ASSERT_EQ(pte.getName(), fields.find(pte.getName())->first);
}

TEST(PassThroughEncoderTest,testClosenessScores) { 
	cout << "\n ---testClosenessScores--- \n" << endl;

	//Identical => 1
	testCloseInner(vector<nupic::UInt>({0,0,0,1,1,1,0,0,0}), vector<nupic::UInt>({0,0,0,1,1,1,0,0,0}), 1.0);

	//No overlap => 0
	testCloseInner(vector<nupic::UInt>({0,0,0,1,1,1,0,0,0}), vector<nupic::UInt>({1,1,1,0,0,0,1,1,1}), 0.0);

	//Similar => 4 of 5 match
	testCloseInner(vector<nupic::UInt>({1,0,1,0,1,0,1,0,1}), vector<nupic::UInt>({1,0,0,1,1,0,1,0,1}), 0.8);

	//Little => 1 of 5 match
	testCloseInner(vector<nupic::UInt>({1,0,0,1,1,0,1,0,1}), vector<nupic::UInt>({0,1,1,1,0,1,0,1,0}), 0.2);

	//Extra active bit => off by 1 of 5		
	testCloseInner(vector<nupic::UInt>({1,0,1,0,1,0,1,0,1}), vector<nupic::UInt>({1,0,1,1,1,0,1,0,1}), 0.8);

	//Missing active bit => off by 1 of 5
	testCloseInner(vector<nupic::UInt>({1,0,1,0,1,0,1,0,1}), vector<nupic::UInt>({1,0,0,0,1,0,1,0,1}), 0.8);		
}
