#include <nupic/encoders/Encoder.hpp>
#include <nupic/encoders/SparsePassThroughEncoder.hpp> 
#include <nupic/encoders/Utils.hpp>
#include "gtest/gtest.h"

using namespace encoders;
using namespace std;
using nupic::UInt;
using U = int;

void testCloseInner(vector<U> bitmap1, int outputWidth1, vector<U> bitmap2, int outputWidth2, double expectedScore){
	SparsePassThroughEncoder<vector<U>> encoder1;
	encoder1.init(outputWidth1, Utils<U>::where(bitmap1, Utils<U>::WHERE_GREATER_OR_EQUAL_0).size(), "foo1");
	
	SparsePassThroughEncoder<vector<U>> encoder2;
	encoder2.init(outputWidth2, Utils<U>::where(bitmap2, Utils<U>::WHERE_GREATER_OR_EQUAL_0).size(), "foo2");

	vector<nupic::UInt> out1 = encoder1.encode(bitmap1);
	vector<nupic::UInt> out2 = encoder2.encode(bitmap2);

	vector<double> result = encoder1.closenessScores(Utils<nupic::UInt>::toDoubleArray(out1), Utils<nupic::UInt>::toDoubleArray(out2), true); 
	ASSERT_TRUE(result.size() == 1 );
	ASSERT_EQ(expectedScore, result[0]);
}


TEST(SparsePassThroughEncoderTest,testEncodeArray_24outputBits) { 
	cout << "\n ---testEncodeArray_24outputBits--- \n" << endl;

	SparsePassThroughEncoder<vector<U>> spte;
	spte.init(24,5,"foo");

	//Send bitmap as array of indices
	vector<U> bitmap({2, 7, 15, 18, 23});
	vector<nupic::UInt> output(24, 0);

	spte.encodeIntoArray(bitmap, output);
	ASSERT_EQ(bitmap.size(), Utils<nupic::UInt>::sum(output)); 

	DecodeResult decode = spte.decode(output, "");
	map<string, RangeList> fields = decode.getFields();
	ASSERT_EQ(spte.getName(), fields.find(spte.getName())->first);
}

TEST(SparsePassThroughEncoderTest,testEncodeArray_12outputBits) { 
	cout << "\n ---testEncodeArray_12outputBits--- \n" << endl;

	SparsePassThroughEncoder<vector<U>> spte;
	spte.init(12,2,"foo2");

	//Send bitmap as array of indices
	vector<U> bitmap({0, 11});
	vector<nupic::UInt> output(12, 0);

	spte.encodeIntoArray(bitmap, output);
	ASSERT_EQ(bitmap.size(), Utils<nupic::UInt>::sum(output));

	DecodeResult decode = spte.decode(output, "");
	map<string, RangeList> fields = decode.getFields();
	ASSERT_EQ(spte.getName(), fields.find(spte.getName())->first); 
}

TEST(SparsePassThroughEncoderTest,testClosenessScores) { 
	cout << "\n ---testClosenessScores--- \n" << endl; 

	//Identical => 1
	testCloseInner(vector<U>({2, 7, 15, 18, 23}), 24, vector<U>({2, 7, 15, 18, 23}), 24, 1.0);

	//No overlap => 0
	testCloseInner(vector<U>({2, 7, 15, 18, 23}), 24, vector<U>({3, 9, 14, 19, 24}), 25, 0.0);

	//Similar => 4 of 5 match
	testCloseInner(vector<U>({2, 7, 15, 18, 23}), 24, vector<U>({2, 7, 17, 18, 23}), 24, 0.8);

	//Little => 1 of 5 match
	testCloseInner(vector<U>({2, 7, 15, 18, 23}), 24, vector<U>({3, 7, 17, 19, 24}), 25, 0.2);

	//Extra active bit => off by 1 of 5		
	testCloseInner(vector<U>({2, 7, 15, 18, 23}), 24, vector<U>({2, 7, 11, 15, 18, 23}), 24, 0.8);

	//Missing active bit => off by 1 of 5
	testCloseInner(vector<U>({2, 7, 15, 18, 23}), 24, vector<U>({2, 7, 18, 23}), 24, 0.8);		
}
