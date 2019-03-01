#include <nupic/encoders/Encoder.hpp>
#include <nupic/encoders/ScalarEncoder.hpp>
#include <nupic/encoders/AdaptiveScalarEncoder.hpp>
#include <nupic/encoders/Utils.hpp>

#include "gtest/gtest.h"

using namespace encoders;
using namespace std;

AdaptiveScalarEncoder<double> ase;

void initASE() {
	int w = 3;
	int n = 14;
	double minVal = 1;
	double maxVal = 8;
	string name = "";
	double radius = 1.5;
	double resolution = 0.5;
	bool forced = true;

	ase.init(w, n, minVal, maxVal, name, radius, resolution, forced);
}

TEST(AdaptiveScalarEncoderTest,testNonPeriodicEncoderMinMaxSpec) { 
	cout << "\n\n --------test Non-periodic encoder, min and max specified--------- \n" << endl;
	initASE();

	vector<nupic::UInt> res = ase.encode(1.0);
	vector<nupic::UInt> inputArray1({ 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 });
	ASSERT_TRUE(equal(inputArray1.begin(), inputArray1.end(), res.begin()));

	cout << " Encoded data as: [";
	for (auto &i : res) {
		cout << ' ' << i;
	}		std::cout << "]" << '\n';

	///
	res = ase.encode(2.0);
	vector<nupic::UInt> inputArray2({ 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 });
	ASSERT_TRUE(equal(inputArray2.begin(), inputArray2.end(), res.begin()));

	cout << " Encoded data as: [";
	for (auto &i : res) {
		cout << ' ' << i;
	}		std::cout << "]" << '\n';

	///
	res = ase.encode(8.0);
	vector<nupic::UInt> inputArray3({ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1 });
	ASSERT_TRUE(equal(inputArray3.begin(), inputArray3.end(), res.begin()));		
	cout << " Encoded data as: [";
	for (auto &i : res) {
		cout << ' ' << i;
	}		std::cout << "]" << '\n';
}

//Test the input description generation and topDown decoding
TEST(AdaptiveScalarEncoderTest,testTopDownDecode) { 
	cout << "\n\n --------test TopDownDecode--------- \n" << endl;
	initASE();

	double minVal = ase.getMinVal();
	cout << " The min value is: " << minVal << endl;
	double resolution = ase.getResolution();
	cout << " Testing non-periodic encoder decoding, resolution is " << resolution << endl;
	double maxVal = ase.getMaxVal();
	cout << " The max value is: " << maxVal << endl;
	 
	while (minVal < 1.5) {  //original Test => while(minVal < maxVal)
		vector<nupic::UInt> output = ase.encode(minVal);	 
		DecodeResult decoded = ase.decode(output, "");	
		map<string, RangeList> fields = decoded.getFields();

		cout.precision(17);
		cout << "\n \n ------- min value is: " << minVal << endl;
		cout << " Decoding: [";
		for (auto &i : output) {
			cout << ' ' << i;
		}		cout << "]" << '\n';

		//cout.precision(16);
		for (const auto &pair : fields) {
			cout << " Field Key: " << pair.first << endl;
		}
		 
		double rangMin, rangMax;
		cout << " Field Range Value: [";
		for (map<string, RangeList>::iterator it=fields.begin(); it!=fields.end(); ++it){
			 rangMin = it->second.getRange(0).first;
			 rangMax = it->second.getRange(0).second;
			std::cout <<  rangMin << ", " << rangMax;
		}		cout << "]" << '\n';

		//Range max and min are not matching 
		ASSERT_EQ(rangMin, rangMax ); 
		ASSERT_TRUE(abs(rangMin - minVal) < ase.getResolution());


		vector< tuple<boost::any, boost::any, vector<nupic::UInt>> > topDown = ase.topDownCompute(output);
		double topDown_value	= boost::any_cast<double>(get<0>(topDown[0]));
		double topDown_scalar	= boost::any_cast<double>(get<1>(topDown[0]));
		vector<nupic::UInt> topDown_vec =  get<2>(topDown[0]);
		ASSERT_TRUE(topDown.size() == 1);
		cout << "\n TopDown => [EncoderResult(value= " << topDown_value << ", scalar= " <<  topDown_scalar  << ", encoding=[ hashcode(...)])"<< endl;


		vector<nupic::UInt> bucketIndices = ase.getBucketIndices(minVal);
		//The bucket indice size is not matching
		ASSERT_TRUE(bucketIndices.size() == 1);

		cout << "\n Bucket indices =>[";
		for (auto &i : bucketIndices) {
			cout << ' ' << i << ' ';
		}		cout << "]" << '\n';


		vector< tuple<boost::any, boost::any, vector<nupic::UInt>> >  bucketInfoList = ase.getBucketInfo(bucketIndices);
		double bucket_value		= boost::any_cast<double>(get<0>(bucketInfoList[0]));
		double bucket_scalar	= boost::any_cast<double>(get<1>(bucketInfoList[0]));
		vector<nupic::UInt> bucket_vec	=  get<2>(bucketInfoList[0]);

		cout.precision(17);
		cout << " \n Minval: " << minVal << "     Abs(BucketVal - Minval): " << abs((double)bucket_value - minVal) << endl;
		cout << " Resolution: " << ase.getResolution() << " Resolution/2: " << ase.getResolution() / 2 << endl;

		ASSERT_TRUE((abs(bucket_value - minVal)) <= (ase.getResolution() / 2));
		//ASSERT_TRUE(bucket_value== (double)ase.getBucketValues(Double.class).toArray()[bucketIndices[0]]);
		ASSERT_TRUE(bucket_scalar == bucket_value);
		cout << "\n Bucket info value: " << bucket_value << endl;
		cout << " Bucket info scalar: " << bucket_scalar << endl;
		cout << " Bucket info encoding: [hashcode(...)] "  << endl;

		ASSERT_TRUE(equal(bucket_vec.begin(), bucket_vec.end(), output.begin()));	
		cout << "\n Original encoding =>[";
		for (auto &i : bucket_vec) {
			cout << ' ' << i << ' ';
		}		cout << "]" << '\n'  ;

		minVal += resolution / 4; 
	}		 
}

//Make sure we can fill in holes
TEST(AdaptiveScalarEncoderTest,TestFillHoles) { 
	cout << "\n\n --------test Fill Holes--------- \n" << endl;
	initASE();
	 
	//double minVal = ase.getMinVal();
	vector<nupic::UInt> inputArray1({ 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1 });
	DecodeResult decoded1 = ase.decode(inputArray1, "");
	map<string, RangeList> fields = decoded1.getFields();

	cout << " Decoding: [";
	for (auto &i : inputArray1) {
		cout << ' ' << i;
	}		cout << "]" << '\n';

	cout.precision(17);
	for (const auto &pair : fields) {
		cout << " Field Key: " << pair.first << endl;
	}

	double rangMin0, rangMax0, rangMin1, rangMax1;
	cout << " Field Range Value: [";
	for (map<string, RangeList>::iterator it=fields.begin(); it!=fields.end(); ++it){
		 rangMin0 = it->second.getRange(0).first;
		 rangMax0 = it->second.getRange(0).second;
		 rangMin1 = it->second.getRange(1).first;
		 rangMax1 = it->second.getRange(1).second;
		std::cout <<  rangMin0 << ", " << rangMax0 << ", " << rangMin1 << ", " << rangMax1;
	}		cout << "]" << '\n';
 
	//Range max and min are not matching 
	ASSERT_EQ(rangMin0, rangMax0 ); 
	ASSERT_TRUE(rangMin1 == 8.0);	
	ASSERT_TRUE(rangMax1 == 8.0);
	 
	vector<nupic::UInt> inputArray2({ 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1 });
	DecodeResult decoded2 = ase.decode(inputArray2, ""); 
	map<string, RangeList> newFields = decoded2.getFields();

	cout << "\n\n Decoding: [";
	for (auto &i : inputArray2) {
		cout << ' ' << i;
	}		cout << "]" << '\n';
	 
	for (const auto &pair : newFields) {
		cout << " Field Key: " << pair.first << endl;
	}

	double newRangMin0, newRangMax0, newRangMin1, newRangMax1;
	cout << " Field Range Value: [";
	for (map<string, RangeList>::iterator it=newFields.begin(); it!=newFields.end(); ++it){
		 newRangMin0 = it->second.getRange(0).first;
		 newRangMax0 = it->second.getRange(0).second;
		 newRangMin1 = it->second.getRange(1).first;
		 newRangMax1 = it->second.getRange(1).second;
		std::cout <<  newRangMin0 << ", " << newRangMax0 << ", " << newRangMin1 << ", " << newRangMax1;
	}		cout << "]" << '\n';
	 
	ASSERT_EQ(newRangMin0, newRangMax0 ); 
	ASSERT_TRUE(newRangMin1 == 8.0);	
	ASSERT_TRUE(newRangMax1 == 8.0);	
}

TEST(AdaptiveScalarEncoderTest, testMissingValues) {
	cout << "\n\n --------test Missing Values --------- \n" << endl;

	AdaptiveScalarEncoder<double> mv;
	int w = 3;
	int n = 14;
	double minVal = 1;
	double maxVal = 8;
	string name = "mv";
	double radius = 1.5;
	double resolution = 0.5;
	bool forced = true;

	mv.init(w, n, minVal, maxVal, name, radius, resolution, forced);

	vector<nupic::UInt> empty = mv.encode(SENTINEL_VALUE_FOR_MISSING_DATA);
	vector<int> expected(14);
	ASSERT_TRUE(equal(expected.begin(), expected.end(), empty.begin()));
	cout << " Encoded missing data as: [";
	for (auto &i : empty) {
		cout << ' ' << i;
	}		cout << "]" << '\n';

}
