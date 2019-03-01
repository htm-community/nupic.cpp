#include <nupic/encoders/Encoder.hpp>
#include <nupic/encoders/Utils.hpp>
#include <nupic/encoders/CoordinateEncoder.hpp>
#include <nupic/encoders/GeospatialCoordinateEncoder.hpp>

#include <nupic/encoders/md5.hpp>
#include "gtest/gtest.h"

using namespace encoders;
using namespace std;
using EncT = vector<int>;

GeospatialCoordinateEncoder<EncT> ge;

int ge_n		 = 313;
int ge_w		 = 3;
int ge_scale	 = 30;
int ge_timeStep	 = 60;
string ge_name	 = "coordinate";

void setUpGE() { 	
	ge.init(ge_n, ge_w, ge_name, ge_scale, ge_timeStep);
}

vector<UInt> encode(GeospatialCoordinateEncoder<EncT> encoder, double coordinate[], double radius) {
	vector<UInt> output(encoder.getWidth(), 0);
	encoder.encodeIntoArrayB(coordinate[0], coordinate[1], radius, output);
	return output;
}

double overlapGCE(vector<UInt> sdr1, vector<UInt> sdr2) {
	assert(sdr1.size() == sdr2.size());
	int sum = Utils<UInt>::sum(Utils<UInt>::overlaping(sdr1, sdr2));
	
	return (double)sum / (double)Utils<UInt>::sum(sdr1);
}

TEST(TestGeospatialCoordinateEncoder, testEncodeIntoArray) {
	cout << "\n\n --------test Encode Into Array --------- \n" << endl;

	ge.init(999, 25, ge_name, ge_scale, ge_timeStep);

	double speed = 2.5;	//meters per second
	double coordinate1[] = { -122.229194, 37.486782 };
	double coordinate2[] = { -122.229294, 37.486882 };
	double coordinate3[] = { -122.229294, 37.486982 };
	 
	vector<UInt> encoding1 = encode(ge, coordinate1, speed); 
	vector<UInt> encoding2 = encode(ge, coordinate2, speed);
	vector<UInt> encoding3 = encode(ge, coordinate3, speed);

	double overlap1 = overlapGCE(encoding1, encoding2);
	double overlap2 = overlapGCE(encoding1, encoding3);

	ASSERT_TRUE(overlap1 > overlap2); 
}

TEST(TestGeospatialCoordinateEncoder,testCoordinateForPosition){	
	cout << "\n\n --------test Coordinate For Position--------- \n" << endl;	
	setUpGE();	

	vector<double> coords({ -122.229194, 37.486782 }); 
	vector<int> coordinate = ge.coordinateForPosition(coords[0], coords[1]);

	vector<int> temp({ -453549, 150239 });
	ASSERT_TRUE(equal(coordinate.begin(), coordinate.end(), temp.begin()));
}

TEST(TestGeospatialCoordinateEncoder,testCoordinateForPositionOrigin){	
	cout << "\n\n --------test Coordinate For Position Origin--------- \n" << endl;
	setUpGE(); 

	vector<double> coords({ 0, 0 }); 
	vector<int> coordinate = ge.coordinateForPosition(coords[0], coords[1]);

	vector<int> temp({ 0, 0 });
	ASSERT_TRUE(equal(coordinate.begin(), coordinate.end(), temp.begin()));
}

TEST(TestGeospatialCoordinateEncoder,testRadiusForSpeed){	
	cout << "\n\n --------test Radius For Speed--------- \n" << endl;
	setUpGE(); 

	double speed = 50;	//meters per second
	double radius = ge.radiusForSpeed(speed);

	ASSERT_EQ(radius, 75);
}

TEST(TestGeospatialCoordinateEncoder,testRadiusForSpeed0){	
	cout << "\n\n --------test Radius For Speed 0 --------- \n" << endl;

	ge.init(999, 27, ge_name, ge_scale, ge_timeStep);

	double speed = 0;	//meters per second
	double radius = ge.radiusForSpeed(speed);

	ASSERT_EQ(radius, 3);
}

TEST(TestGeospatialCoordinateEncoder,testRadiusForSpeedInt){	
	cout << "\n\n --------test Radius For Speed Int --------- \n" << endl;
	setUpGE();

	double speed = 25;	//meters per second
	double radius = ge.radiusForSpeed(speed);

	ASSERT_EQ(radius, 38);
}


TEST(TestGeospatialCoordinateEncoder,testLongLatMercatorTransform){	
	cout << "\n\n --------test convert longitude and latitude to Mercator Transform --------- \n" << endl;
	setUpGE();

	double coords[] = { -122.229194, 37.486782 };
	vector<double> mercatorCoords = ge.toMercator(coords[0], coords[1]);

	ASSERT_DOUBLE_EQ(mercatorCoords[0] , -13606491.634258213);
	ASSERT_DOUBLE_EQ(mercatorCoords[1], 4507176.870955294);

	vector<double> longlats = ge.inverseMercator(mercatorCoords[0], mercatorCoords[1]);
	ASSERT_DOUBLE_EQ(coords[0], longlats[0]);
	ASSERT_DOUBLE_EQ(coords[1], longlats[1]);
}
