#include <nupic/encoders/CoordinateEncoder.hpp>

#include "nupic/utils/Random.hpp"
#include <nupic/encoders/md5.hpp>
#include "gtest/gtest.h"

using namespace encoders;
using namespace std;

CoordinateEncoder<vector<int>> coe;
bool verbose;

void setUpCOE() {
	coe.setN(33);
	coe.setW(3);
	coe.setName("coordinate");

	//  Implementing classes would do setting of specific  
	//  vars here together with any sanity checking        	 
	if (coe.getW() <= 0 || coe.getW() % 2 == 0) {
		cout << "w must be odd, and must be a positive integer";
		exit(-1);
	}

	if (coe.getN() <= 6 * coe.getW()) {
		cout << "n must be an int strictly greater than 6*w. For good results we recommend n be strictly greater than 11*w";
		exit(-1);
	}

	if (coe.getName() == "" || coe.getName() == "None") {
		coe.setName("[" + to_string(coe.getN()) + ":" + to_string(coe.getW()) + "]");
	}
}

double overlapCOE(vector<nupic::UInt> sdr1, vector<nupic::UInt> sdr2) {
	assert(sdr1.size() == sdr2.size());
	nupic::UInt sum = Utils<nupic::UInt>::sum(Utils<nupic::UInt>::and_(sdr1, sdr2));

	return (double)sum / (double)Utils<nupic::UInt>::sum(sdr1);
}

vector<double> overlapsForRelativeAreas(int n, int w, vector<int> initPosition,
	int initRadius, vector<int> dPosition, int dRadius, int num, bool verbose) {

	coe.setN(n);
	coe.setW(w);
	coe.setName("coordinate");

	vector<double> overlaps(num, 0);
	vector<nupic::UInt> outputA = coe.encode(initPosition, initRadius);
	vector<int> newPosition;
	for (int i = 0; i < num; i++) {
		newPosition = dPosition.empty() ? initPosition :
			Utils<int>::i_add(
				newPosition = initPosition,
				Utils<int>::multiply(dPosition, (i + 1)));
		int newRadius = initRadius + (i + 1) * dRadius;
		vector<nupic::UInt> outputB = coe.encode(newPosition, newRadius);
		overlaps[i] = overlapCOE(outputA, outputB);
	}

	return overlaps;
}

void assertDecreasingOverlaps(vector<double> overlaps) {
	ASSERT_TRUE(0 ==  Utils<double>::sum(
		Utils<double>::where(
			Utils<double>::diff(overlaps), Utils<double>::WHERE_GREATER_THAN_0)));
}

vector<double> overlapsForUnrelatedAreas(int n, int w, int radius, int repetitions, bool verbose) {
	return overlapsForRelativeAreas(n, w, vector<int>({ 0, 0 }), radius, vector<int>({ 0, radius * 10 }), 0, repetitions, verbose);
}

//As radius increases, the overlap should decrease
TEST(TestCoordinateEncoder, testEncodeRelativePositionsAndRadii) {
	std::cout << "\n ---test  encode relative positions and radii !" << std::endl;

	// As radius increases and positions change, the overlap should decrease
	vector<double> overlaps = overlapsForRelativeAreas(999, 25, vector<int>({ 100, 200 }), 5, vector<int>({ 1, 1 }), 1, 5, false);

	assertDecreasingOverlaps(overlaps);		
}	 


//As you get farther from a coordinate, the overlap should decrease
TEST(TestCoordinateEncoder, testEncodeRelativePositions) {
	std::cout << "\n ---test  encode relative positions !" << std::endl;

	// As you get farther from a coordinate, the overlap should decrease
	vector<double> overlaps = overlapsForRelativeAreas(999, 25, vector<int>({ 100, 200 }), 
													   10, vector<int>({ 2, 2 }), 0, 5, false);
	assertDecreasingOverlaps(overlaps);  

	// values of nupic.python testEncodeRelativePositions
	overlaps = overlapsForRelativeAreas(999, 51, vector<int>({ 100, 200 }),
										10, vector<int>({ 2, 2 }), 0, 5, false);
	assertDecreasingOverlaps(overlaps);
}	

//As radius increases, the overlap should decrease
TEST(TestCoordinateEncoder, testEncodeRelativeRadii) {
	std::cout << "\n ---test  encode relative radii !" << std::endl;

	// As radius increases, the overlap should decrease
	vector<double> overlaps = overlapsForRelativeAreas(999, 25, vector<int>({ 100, 200 }), 5,
														vector<int>(), 1, 5, false);
	assertDecreasingOverlaps(overlaps);		

	// values of nupic.python testEncodeRelativeRadii
	overlaps = overlapsForRelativeAreas(999, 25, vector<int>({ 100, 200 }), 5,
														vector<int>(), 2, 5, false);
	assertDecreasingOverlaps(overlaps);

	// As radius decreases, the overlap should decrease
	overlaps = overlapsForRelativeAreas(999, 25, vector<int>({ 100, 200 }), 20,
										vector<int>(), -2, 5, false);
	assertDecreasingOverlaps(overlaps);

	// values of nupic.python testEncodeRelativeRadii
	overlaps = overlapsForRelativeAreas(999, 51, vector<int>({ 100, 200 }), 20,
										vector<int>(), -2, 5, false);
	assertDecreasingOverlaps(overlaps);
}	 

TEST(TestCoordinateEncoder, testOrderForCoordinate) {
	std::cout << "\n ---test order for coordinate !" << std::endl;

	double h1 = coe.orderForCoordinate(vector<int>({ 2, 5, 10 }));
	double h2 = coe.orderForCoordinate(vector<int>({ 2, 5, 11 }));
	double h3 = coe.orderForCoordinate(vector<int>({ 2497477, -923478 }));

	ASSERT_TRUE(0 <= h1 && h1 < 1);
	ASSERT_TRUE(0 <= h2 && h2 < 1);
	ASSERT_TRUE(0 <= h3 && h3 < 1);

	ASSERT_TRUE(h1 != h2);
	ASSERT_TRUE(h2 != h3);

	cout << "h1: " << h1 << ", h2: " << h2 << ", h3: " << h3 << endl;
	//JAVA h1: 0.29532587998654214, h2: 0.12030270383644848, h3: 0.8069197554915457
} 

TEST(TestCoordinateEncoder, testBitForCoordinate) {
	std::cout << "\n ---test bit for coordinate !" << std::endl;

	int n = 1000;

	double b1 = coe.bitForCoordinate(vector<int>({ 2, 5, 10 }), n);
	double b2 = coe.bitForCoordinate(vector<int>({ 2, 5, 11 }), n);
	double b3 = coe.bitForCoordinate(vector<int>({ 2497477, -923478 }), n);

	ASSERT_TRUE(0 <= b1 && b1 < n);
	ASSERT_TRUE(0 <= b2 && b2 < n);
	ASSERT_TRUE(0 <= b3 && b3 < n);

	ASSERT_TRUE(b1 != b2);
	ASSERT_TRUE(b2 != b3);

	// Small n
	n = 2;
	vector<int> inputArray4({ 5, 10 });
	double b4 = coe.bitForCoordinate(inputArray4, n);

	ASSERT_TRUE(0 <= b4 && b4 < n);

	cout << "b1: " << b1 << ", b2: " << b2 << ", b3: " << b3 << ", b4: " << b4 << endl;
	// JAVA b1 514.0, b2: 92.0, b3: 981.0, b4: 0.0
} 

class Coordinate : public encoders::CoordinateOrder {
public:
	double orderForCoordinate(vector<int> coordinate) const override{
		return  Utils<int>::sum(coordinate) / 5.0;
	}
};

TEST(TestCoordinateEncoder, testTopWCoordinates) {
	std::cout << "\n ---test top W coordinates !" << std::endl;
	 
	vector<vector<int>> coordinates({ { 1 }, { 2 }, { 3 }, { 4 }, { 5 } });

    Coordinate co;
	vector<vector<int> > top = coe.topWCoordinates(&co, coordinates, 2);

	ASSERT_EQ(2, top.size());
	ASSERT_TRUE(4 == top[0][0]); 
	ASSERT_TRUE(5 == top[1][0]); 

}

TEST(TestCoordinateEncoder, testNeighbors1D) {
	std::cout << "\n ---test neighbors 1D !" << std::endl;

	int radius = 5;
	vector<vector<int>> neighbors = coe.neighbors(vector<int>({ 100 }), radius);

	ASSERT_EQ(11, neighbors.size());
	ASSERT_EQ(95, neighbors[0][0]);
	ASSERT_EQ(100, neighbors[5][0]);
	ASSERT_TRUE(105 == neighbors[10][0]);
} 

TEST(TestCoordinateEncoder, testNeighbors2D) {
	std::cout << "\n ---test neighbors 2D !" << std::endl;

	int radius = 5;
	vector<vector<int>> neighbors = coe.neighbors(vector<int>({ 100, 200 }), radius);

	ASSERT_EQ(121, neighbors.size());

	ASSERT_TRUE(Utils<int>::contains(vector<int>({ 95, 195 }), neighbors));		
	ASSERT_TRUE(Utils<int>::contains(vector<int>({ 95, 205 }), neighbors));
	ASSERT_TRUE(Utils<int>::contains(vector<int>({ 100, 200 }), neighbors));
	ASSERT_TRUE(Utils<int>::contains(vector<int>({ 105, 195 }), neighbors));
	ASSERT_TRUE(Utils<int>::contains(vector<int>({ 105, 205 }), neighbors));
} 

TEST(TestCoordinateEncoder, testNeighbors0Radius) {
	std::cout << "\n ---test neighbors 0 radius !" << std::endl;

	int radius = 0;
	vector<vector<int>> neighbors = coe.neighbors(vector<int>({ 100, 200, 300 }), radius);

	ASSERT_EQ(1, neighbors.size());
	ASSERT_TRUE(Utils<int>::contains(vector<int>({ 100, 200, 300 }), neighbors));
} 

TEST(TestCoordinateEncoder, testEncodeIntoArray) {
	std::cout << "\n ---test encode into array !" << std::endl;

	setUpCOE();

	vector<nupic::UInt> output1 = coe.encode(vector<int>({ 100, 200 }), 5);
	ASSERT_EQ(Utils<nupic::UInt>::sum(output1), coe.getW());
	vector<nupic::UInt> output2 = coe.encode(vector<int>({ 100, 200 }), 5);
	ASSERT_TRUE(equal(output1.begin(), output1.end(), output2.begin()));
} 

TEST(TestCoordinateEncoder, testEncodeSaturateArea) {
	std::cout << "\n ---test EncodeSaturateArea!" << std::endl;
	 
	coe.setN(1999);
	coe.setW(25);
	coe.setRadius(2);

	vector<nupic::UInt> outputA = coe.encode(vector<int>({ 0, 0 }), 2);
	vector<nupic::UInt> outputB = coe.encode(vector<int>({ 0, 1 }), 2);

	ASSERT_EQ(0.8, overlapCOE(outputA, outputB));
} 

TEST(TestCoordinateEncoder, testEncodeUnrelatedAreas) {
	std::cout << "\n ---test  encode unrelated areas !" << std::endl;

	double avgThreshold = 0.3;

	// values of nupic.python testEncodeUnrelatedAreas
	double maxThreshold = 0.12;
	vector<double> overlaps = overlapsForUnrelatedAreas(1499, 37, 5, 100, false);

	auto biggest1 = std::max_element(begin(overlaps), end(overlaps));
	ASSERT_TRUE(*biggest1 < maxThreshold);								 
	ASSERT_TRUE(Utils<double>::average(overlaps) < avgThreshold);

	maxThreshold = 0.12;
	overlaps = overlapsForUnrelatedAreas(1499, 37, 10, 100, false);

	auto biggest2 = std::max_element(begin(overlaps), end(overlaps));
	ASSERT_TRUE(*biggest2 < maxThreshold);								 
	ASSERT_TRUE(Utils<double>::average(overlaps) < avgThreshold);

	maxThreshold = 0.17;
	overlaps = overlapsForUnrelatedAreas(999, 25, 10, 100, false);

	auto biggest3 = std::max_element(begin(overlaps), end(overlaps));
	ASSERT_TRUE(*biggest3 < maxThreshold);								 
	ASSERT_TRUE(Utils<double>::average(overlaps) < avgThreshold);

	maxThreshold = 0.25;
	overlaps = overlapsForUnrelatedAreas(499, 13, 10, 100, false);

	auto biggest4 = std::max_element(begin(overlaps), end(overlaps));
	ASSERT_TRUE(*biggest4 < maxThreshold);								
	ASSERT_TRUE(Utils<double>::average(overlaps) < avgThreshold);
}	
	 

TEST(TestCoordinateEncoder, testEncodeAdjacentPositions) {
	std::cout << "\n ---test encode adjacent positions !" << std::endl;

	int repetitions = 100;
	int n = 999;
	int w = 25;
	int radius = 10;
	double minThreshold = 0.75;
	double avgThreshold = 0.90;
	vector<double> allOverlaps(repetitions, 0);	

	for (int i = 0; i < repetitions; i++) {		
		vector<double> overlaps = overlapsForRelativeAreas(n, w, vector<int> ({ i * 10, i * 10 }), radius, vector<int> ({ 0, 1 }), 0, 1, false);
		allOverlaps[i] = overlaps[0];
	}

	auto biggest = std::min_element(begin(allOverlaps), end(allOverlaps));
	ASSERT_TRUE(*biggest > minThreshold);							
	ASSERT_TRUE(Utils<double>::average(allOverlaps) > avgThreshold);

	if (verbose) {
		cout << "===== Adjacent positions overlap " <<
			"(n = {0}, w = {1}, radius = {2} ===" << n << w << radius << endl;
		auto biggest1 = std::max_element(begin(allOverlaps), end(allOverlaps));
		cout << "Max: {0}" << *biggest1 << endl;
		auto biggest2 = std::min_element(begin(allOverlaps), end(allOverlaps));
		cout << "Min: {0}" << *biggest2 << endl;
		cout << "Average: {0}" << Utils<double>::average(allOverlaps) << endl;
	}
}


TEST(TestCoordinateEncoder, testLargeW) {
  std::cout << "\n ---test large w>= 27  which caused crash!" << std::endl;

  setUpCOE();
  coe.setW(131);
  coe.setN(1000);
  vector<nupic::UInt> output1 = coe.encode(vector<int>({ 100, 200 }), 5);
  ASSERT_TRUE(Utils<nupic::UInt>::sum(output1) > 0); 
}

TEST(TestCoordinateEncoder, testRetinaMode) {
  auto enc = CoordinateEncoder<vector<int>>(1, 6/*n*/, 1.0, true/*retina*/, vector<UInt>{2,3});
  vector<int> in1 = {0,0}; //min coord
  vector<UInt> exp1 = {1,0,0,0,0,0};
  auto out1 = enc.encode(in1, 1.0/*radius unused*/);
  EXPECT_EQ(out1, exp1);

  vector<int> in2 = {1,2}; //max coord
  vector<UInt> exp2 = {0,0,0,0,0,1};
  auto out2 = enc.encode(in2, 1.0/*radius unused*/);
  EXPECT_EQ(out2, exp2);


  vector<int> in3 = {1,1};
  vector<UInt> exp3 = {0,0,0,1,0,0};
  auto out3 = enc.encode(in3, 1.0/*radius unused*/);
  EXPECT_EQ(out3, exp3);

}
