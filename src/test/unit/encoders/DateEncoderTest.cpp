#include <time.h>
#include <ctime> 
#include "gtest/gtest.h"

#include <nupic/encoders/Encoder.hpp> 
#include <nupic/encoders/ScalarEncoder.hpp>
#include <nupic/encoders/DateEncoder.hpp>
#include <nupic/encoders/DecodeResult.hpp>
#include <nupic/encoders/Utils.hpp>

using namespace encoders;
using namespace std;

DateEncoder<time_t> de;
time_t dt;
vector<nupic::UInt> bits;
vector<nupic::UInt> expectedDE;

void setUpDE() {
	de.setSeason(tuple<int, double>(3, (double)get<1>(de.getSeason())));
	de.setDayOfWeek(tuple<int, double>(1, (double)get<1>(de.getDayOfWeek())));
	de.setWeekend(tuple<int, double>(3, (double)get<1>(de.getWeekend())));
	de.setTimeOfDay(tuple<int, double>(5, (double)get<1>(de.getTimeOfDay())));
	de.init();

	//       makedate( year, month, day, hour,  min , dl_savingtime)
	dt = Utils<int>::makedate(2010, 11, 4, 14, 55, -1);
	//printf("time setUpDE %s \n", ctime(&dt));

	time_t comparison = Utils<int>::makedate(2010, 11, 4, 13, 55, -1);

	bits = de.encode(dt);
	vector<nupic::UInt> comparisonBits = de.encode(comparison);

	std::cout << " bits:           [";
	for (auto &i : bits) {
		std::cout << ' ' << i;
	}		std::cout << "]" << '\n';

	std::cout << " comparisonBits: [";
	for (auto &i : comparisonBits) {
		std::cout << ' ' << i;
	}		std::cout << "]" << '\n';

	// season is aaabbbcccddd (1 bit/month) # TODO should be <<3?
	// should be 000000000111 (centered on month 11 - Nov)
	int seasonExpected_[] = { 0,0,0,0,0,0,0,0,0,1,1,1 };
	expectedDE.insert(expectedDE.end(), begin(seasonExpected_), end(seasonExpected_));

	// week is MTWTFSS
	// contrary to local time documentation, Monday = 0 (for python
	//  datetime.datetime.timetuple()
	int dayOfWeekExpected[] = { 0,0,0,1,0,0,0 };
	expectedDE.insert(expectedDE.end(), begin(dayOfWeekExpected), end(dayOfWeekExpected));

	// not a weekend, so it should be "False"
	int weekendExpected[] = { 1,1,1,0,0,0 };
	expectedDE.insert(expectedDE.end(), begin(weekendExpected), end(weekendExpected));

	// time of day has radius of 4 hours and w of 5 so each bit = 240/5 min = 48min
	// 14:55 is minute 14*60 + 55 = 895; 895/48 = bit 18.6
	// should be 30 bits total (30 * 48 minutes = 24 hours)
	int timeOfDayExpected[] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,0,0,0 };
	expectedDE.insert(expectedDE.end(), begin(timeOfDayExpected), end(timeOfDayExpected));
}

TEST(TestDateEncoder, testDateEncoder) {
	std::cout << "\n ------ test date encoder ------ \n" << std::endl;
	setUpDE();

	vector<tuple<string, int> > descs = de.getDescription();

	vector<tuple<string, int> > expectedDescs;
	expectedDescs.push_back(tuple<string, int>("season", 0));
	expectedDescs.push_back(tuple<string, int>("day of week", 12));
	expectedDescs.push_back(tuple<string, int>("weekend", 19));
	expectedDescs.push_back(tuple<string, int>("time of day", 25));

	ASSERT_EQ(expectedDescs.size(), descs.size());

	for (int i = 0; i < (int)expectedDescs.size(); ++i) {
		tuple<string, int> desc = descs[i];
		ASSERT_TRUE(expectedDescs[i] == desc);
	}

	ASSERT_TRUE(equal(expectedDescs.begin(), expectedDescs.end(), descs.begin()));

}

TEST(TestDateEncoder, testHoliday) {
	std::cout << "\n\n --------test holiday--------- \n" << std::endl;

	DateEncoder<time_t> deHoliday;
	deHoliday.setForced(true);
	deHoliday.setHoliday(tuple<int, double>(5, (double)get<1>(deHoliday.getHoliday())));
	deHoliday.init();

	vector<int> holiday({ 0,0,0,0,0,1,1,1,1,1 });
	vector<int> notholiday({ 1,1,1,1,1,0,0,0,0,0 });
	vector<int> holiday2({ 0,0,0,1,1,1,1,1,0,0 });

	time_t d = Utils<int>::makedate(2010, 12, 25, 4, 55, -1);
	vector<nupic::UInt> encHoliday = deHoliday.encode(d);
	ASSERT_TRUE(equal(encHoliday.begin(), encHoliday.end(), holiday.begin()));

	std::cout << " encHoliday:   [";
	for (auto &i : encHoliday) {
		std::cout << ' ' << i;
	}		std::cout << "]" << '\n';

	d = Utils<int>::makedate(2008, 12, 27, 4, 55, -1);
	vector<nupic::UInt> encNotHoliday = deHoliday.encode(d);
	ASSERT_TRUE(equal(encNotHoliday.begin(), encNotHoliday.end(), notholiday.begin()));

	std::cout << " encNotHoliday:   [";
	for (auto &i : encNotHoliday) {
		std::cout << ' ' << i;
	}		std::cout << "]" << '\n';

	d = Utils<int>::makedate(1999, 12, 26, 8, 0, -1);
	vector<nupic::UInt> encHoliday2 = deHoliday.encode(d);
	ASSERT_TRUE(equal(encHoliday2.begin(), encHoliday2.end(), holiday2.begin()));

	std::cout << " encHoliday2:   [";
	for (auto &i : encHoliday2) {
		std::cout << ' ' << i;
	}		std::cout << "]" << '\n';
	
	/*d = Utils<int>::makedate(2011, 12, 24, 16, 0, 0);
	vector<nupic::UInt> encHoliday3 = deHoliday.encode(d);	
	ASSERT_TRUE(equal(encHoliday3.begin(), encHoliday3.end(), holiday2.begin()));*/	
}

//Decoding date
TEST(TestDateEncoder, testDecoding) {
	std::cout << "\n ------ test decoding ------ \n" << std::endl;
	//setUpDE(); 

	DecodeResult decoded = de.decode(bits, "");
	map<string, RangeList> fieldsMap = decoded.getFields();
	vector<string> fieldsOrder = decoded.getDescriptions();

	std::cout << " decoded Date => ";
	for (auto &it : fieldsMap) {							
		string fieldName = it.first;
		double rangMin = it.second.getRange(0).first;
		double rangMax = it.second.getRange(0).second;
		std::cout << fieldName << "=[" << rangMin << ", " << rangMax << "], ";
	}		std::cout << '\n';

	/*std::cout << " fieldsOrder: [";
	for (auto &i: fieldsOrder) {
	std::cout << ' ' << i;
	}		std::std::cout << "]" << '\n';*/

	map<string, double> expectedMap;
	expectedMap.insert(pair<string, double>("season", 305.0));
	expectedMap.insert(pair<string, double>("time of day", 14.4));
	expectedMap.insert(pair<string, double>("day of week", 3.0));
	expectedMap.insert(pair<string, double>("weekend", 0.0));

	ASSERT_EQ(4, fieldsMap.size());

	for (auto& key : expectedMap) {
		double expectedDE = key.second;
		RangeList actual = fieldsMap.find(key.first)->second;
		ASSERT_EQ(1, actual.size());
		pair<double, double> minmax = actual.getRange(0);

		// mykkro: is this correct?
		// originally:
		//ASSERT_EQ(expectedDE, minmax.first, de.getMinVal());
		//ASSERT_EQ(expectedDE, minmax.second, de.getMaxVal());
		//cout << key.first << " " << expectedDE << " " << de.getMinVal() << " " << de.getMaxVal() << " " << minmax.first << " " << minmax.second;
		// this will produce: 3 0 0 3 3
		
		ASSERT_DOUBLE_EQ(expectedDE, minmax.first);
		//ASSERT_DOUBLE_EQ(expectedDE, de.getMinVal());
		ASSERT_DOUBLE_EQ(expectedDE, minmax.second);
		//ASSERT_DOUBLE_EQ(expectedDE, de.getMaxVal());
	}
}

//Test weekend encoder
TEST(TestDateEncoder, testWeekend) {
	std::cout << "\n ------ test weekend ------ \n" << std::endl;

	vector<string> day1({ "sat", "sun", "fri" });
	DateEncoder<time_t> e;
	e.setCustomDays(tuple<int, vector<string> >(21, day1));
	e.setForced(true);
	e.init();

	vector<string> day2({ "Monday" });
	DateEncoder<time_t> mon;
	mon.setCustomDays(tuple<int, vector<string> >(21, day2));
	mon.setForced(true);
	mon.init();

	DateEncoder<time_t> e2;
	e2.setWeekend(tuple<int, double >(21, 1));
	e2.setForced(true);
	e2.init();

	time_t dW = Utils<int>::makedate(1988, 5, 29, 20, 0, -1);   

	vector<nupic::UInt> encE = e.encode(dW);
	std::cout << " DateEncoderTest.testWeekend(): e.encode(d)  = [";
	for (auto &i : encE) {
		std::cout << ' ' << i;
	}	std::cout << "]" << '\n';

	vector<nupic::UInt> encE2 = e2.encode(dW);
	std::cout << " DateEncoderTest.testWeekend(): e2.encode(d) = [";
	for (auto &i : encE2) {
		std::cout << ' ' << i;
	}	std::cout << "]" << '\n';

	ASSERT_TRUE(equal(encE.begin(), encE.end(), encE2.begin()));

	for (int i = 0; i < 300; i++) {
		// set next date
		struct tm* tm = localtime(&dW);
		tm->tm_mday += i + 1;
		time_t curDate = mktime(tm);

		vector<nupic::UInt> enc = e.encode(curDate);
		vector<nupic::UInt> enc2 = e2.encode(curDate);
		ASSERT_TRUE(equal(enc.begin(), enc.end(), enc2.begin()));

		DecodeResult decoded = mon.decode(mon.encode(curDate), "");
		map<string, RangeList> fieldsMap = decoded.getFields();
		vector<string> fieldsOrder = decoded.getDescriptions();

		ASSERT_EQ(1, fieldsMap.size());
		RangeList range = fieldsMap.find("Monday")->second;
		ASSERT_EQ(1, range.size());
		ASSERT_EQ(1, range.getRanges().size());

		pair<double, double> minmax = range.getRange(0);
		double minVal = minmax.first;

		//print min values
		//std::cout << " DateEncoderTest.testWeekend(): minmax.min() = " << minVal << std::endl;
		struct tm * input = localtime(&curDate);

		if (minVal == 1.0) {
			ASSERT_EQ(1, input->tm_wday);
		}
		else {
			ASSERT_TRUE(1 != input->tm_wday);
		}
	}
}

//Check bucket index support
TEST(TestDateEncoder, testBucketIndexSupport) {
	std::cout << "\n ------ test bucket index support ------ \n" << std::endl;

	vector<nupic::UInt> bucketIndices = de.getBucketIndices(dt);
	std::cout << " bucket indices: [";
	for (auto &i : bucketIndices) {
		std::cout << ' ' << i;
	}	std::cout << "]" << '\n';

	vector< tuple<boost::any, boost::any, vector<nupic::UInt>> > bucketInfo = de.getBucketInfo(bucketIndices);

	double expectedList[] = { 320.25, 3.5, 0.0, 14.8 };  //{320.25, 3.5, .167, 14.8}
	vector<nupic::UInt> encodings;

	for (int i = 0; i < (int)bucketInfo.size(); i++) {
		tuple<boost::any, boost::any, vector<nupic::UInt>> r = bucketInfo[i];
		double actual = boost::any_cast<double>(get<0>(r));
		double expectedDE = boost::any_cast<double>(expectedList[i]);
		ASSERT_DOUBLE_EQ(expectedDE, actual);

		vector<nupic::UInt> temp = get<2>(r);
		encodings.insert(encodings.end(), temp.begin(), temp.end());
	}

	ASSERT_TRUE(equal(expectedDE.begin(), expectedDE.end(), encodings.begin()));
}


//Check topDownCompute
TEST(TestDateEncoder, testTopDownCompute) {
	std::cout << "\n ------ test TopDownCompute ------ \n" << std::endl;

	vector< tuple<boost::any, boost::any, vector<nupic::UInt>> > topDown = de.topDownCompute(bits);

	double expectedList[] = { 320.25, 3.5, 0.0, 14.8 };  //{320.25, 3.5, .167, 14.8}

	for (int i = 0; i < (int)topDown.size(); i++) {
		tuple<boost::any, boost::any, vector<nupic::UInt>>  r = topDown[i];
		double actual = boost::any_cast<double>(get<0>(r));
		double expectedDE = expectedList[i];
		ASSERT_DOUBLE_EQ(expectedDE, actual);
	}
}
