/* ---------------------------------------------------------------------
* Numenta Platform for Intelligent Computing (NuPIC)
* Copyright (C) 2014-2016, Numenta, In  Unless you have an agreement
* with Numenta, In, for a separate license for this software code, the
* following terms and conditions apply:
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU Affero Public License version 3 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU Affero Public License for more details.
*
* You should have received a copy of the GNU Affero Public License
* along with this program.  If not, see http://www.gnu.org/licenses.
*
* http://numenta.org/licenses/
* ---------------------------------------------------------------------
*/

#ifndef NTA_dateencoder_HPP
#define NTA_dateencoder_HPP

#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <tuple>
#include <map>
#include <list> 
#include <cstring> 
#include <time.h>
#include <ctime> 

#include "Encoder.hpp"
#include "ScalarEncoder2.hpp"
#include "Utils.hpp" 

//#include <boost\any.hpp>

using namespace std;

namespace encoders {

	/**
	* DOCUMENTATION TAKEN DIRECTLY FROM THE PYTHON VERSION:
	*
	* A date encoder encodes a date according to encoding parameters specified in its constructor.
	*
	* The input to a date encoder is a datetime.datetime object. The output is
	* the concatenation of several sub-encodings, each of which encodes a different
	* aspect of the date. Which sub-encodings are present, and details of those
	* sub-encodings, are specified in the DateEncoder constructor.
	*
	* Each parameter describes one attribute to encode. By default, the attribute
	* is not encoded.
	*
	* season (season of the year; units = day):
	* (int) width of attribute; default radius = 91.5 days (1 season)
	* (tuple)  season[0] = width; season[1] = radius
	*
	* dayOfWeek (monday = 0; units = day)
	* (int) width of attribute; default radius = 1 day
	* (tuple) dayOfWeek[0] = width; dayOfWeek[1] = radius
	*
	* weekend (boolean: 0, 1)
	* (int) width of attribute
	*
	* holiday (boolean: 0, 1)
	* (int) width of attribute
	*
	* timeOfday (midnight = 0; units = hour)
	* (int) width of attribute: default radius = 4 hours
	* (tuple) timeOfDay[0] = width; timeOfDay[1] = radius
	*
	* customDays TODO: what is it?
	*
	* forced (default True) : if True, skip checks for parameters' settings; see {@code ScalarEncoders} for details
	*
	* @author utensil
	*
	*/
	template<typename T>
	class DateEncoder : public Encoder<T>
	{
	public:
		/***************** constructors and destructor *****************/

		/**
		* Constructs a new {@code DateEncoder}
		*
		* Package private to encourage construction using the Builder Pattern
		* but still allow inheritance.
		*/
		DateEncoder();
		~DateEncoder();

		/***************** methods *****************/

		/**
		* Init the {@code DateEncoder} with parameters
		*/
		void init();

		/**
		* Encodes inputData and puts the encoded value into the output array,
		* which is a 1-D array of length returned by {@link Connections#getW()}.
		*
		* Note: The output array is reused, so clear it before updating it.
		* @param inputData Data to encode. This should be validated by the encoder.
		* @param output 1-D array of same length returned by {@link Connections#getW()}
		* @Override
		*/
		virtual void encodeIntoArray(T inputData, vector<nupic::UInt>& output) override;

		/**
		* Returns an {@link TDoubleList} containing the sub-field scalar value(s) for
		* each sub-field of the inputData. To get the associated field names for each of
		* the scalar values, call getScalarNames().
		*
		* @param inputData	the input value, in this case a date object
		* @return	a list of one input double
		*/
		vector<double> getScalars(time_t inputData);

		/**
		* See nupic.htm Encoder.java for more information
		*
		* Takes an encoded output and does its best to work backwards and generate
		* the input that would have generated it.
		*
		* In cases where the encoded output contains more ON bits than an input
		* would have generated, this routine will return one or more ranges of inputs
		* which, if their encoded outputs were ORed together, would produce the
		* target output. This behavior makes this method suitable for doing things
		* like generating a description of a learned coincidence in the SP, which
		* in many cases might be a union of one or more inputs.
		*
		* If instead, you want to figure the *most likely* single input scalar value
		* that would have generated a specific encoded output, use the topDownCompute()
		* method.
		*
		* If you want to pretty print the return value from this method, use the
		* decodedToStr() method.
		*
		*************
		* OUTPUT EXPLAINED:
		*
		* fieldsMap is a {@link Map} where the keys represent field names
		* (only 1 if this is a simple encoder, > 1 if this is a multi
		* or date encoder) and the values are the result of decoding each
		* field. If there are  no bits in encoded that would have been
		* generated by a field, it won't be present in the Map. The
		* key of each entry in the dict is formed by joining the passed in
		* parentFieldName with the child encoder name using a '.'.
		*
		* Each 'value' in fieldsMap consists of a {@link Tuple} of (ranges, desc),
		* where ranges is a list of one or more {@link MinMax} ranges of
		* input that would generate bits in the encoded output and 'desc'
		* is a comma-separated pretty print description of the ranges.
		* For encoders like the category encoder, the 'desc' will contain
		* the category names that correspond to the scalar values included
		* in the ranges.
		*
		* The fieldOrder is a list of the keys from fieldsMap, in the
		* same order as the fields appear in the encoded output.
		*
		* Example retvals for a scalar encoder:
		*
		*   {'amount':  ( [[1,3], [7,10]], '1-3, 7-10' )}
		*   {'amount':  ( [[2.5,2.5]],     '2.5'       )}
		*
		* Example retval for a category encoder:
		*
		*   {'country': ( [[1,1], [5,6]], 'US, GB, ES' )}
		*
		* Example retval for a multi encoder:
		*
		*   {'amount':  ( [[2.5,2.5]],     '2.5'       ),
		*   'country': ( [[1,1], [5,6]],  'US, GB, ES' )}
		* @param encoded      		The encoded output that you want decode
		* @param parentFieldName 	The name of the encoder which is our parent. This name
		*      					is prefixed to each of the field names within this encoder to form the
		*    						keys of the {@link Map} returned.
		*
		* @returns Tuple(fieldsMap, fieldOrder)
		*/
		virtual DecodeResult decode(vector<nupic::UInt> encoded, string parentFieldName) override;

		/**
		* See nupic.htm Encoder.java for more information
		*
		* Returns an array containing the sub-field bucket indices for
		* each sub-field of the inputData. To get the associated field names for each of
		* the buckets, call getScalarNames().
		* @param  	input 	The data from the source. This is typically a object with members.
		*
		* @return 	array of bucket indices
		*/
		vector<nupic::UInt> getBucketIndices(time_t input);

		/**
		* See nupic.htm Encoder.java for more information
		*
		* Returns a list of {@link EncoderResult}s describing the inputs for
		* each sub-field that correspond to the bucket indices passed in 'buckets'.
		* To get the associated field names for each of the values, call getScalarNames().
		* @param buckets 	The list of bucket indices, one for each sub-field encoder.
		*              	These bucket indices for example may have been retrieved
		*              	from the getBucketIndices() call.
		*
		* @return A list of {@link EncoderResult}s. Each EncoderResult has
		*/
		vector< tuple<boost::any, boost::any, vector<nupic::UInt>> >  getBucketInfo(vector<nupic::UInt> buckets);

		/**
		* See nupic.htm Encoder.java for more information
		*
		* Returns a list of EncoderResult named tuples describing the top-down
		* best guess inputs for each sub-field given the encoded output. These are the
		* values which are most likely to generate the given encoded output.
		* To get the associated field names for each of the values, call
		* getScalarNames().
		* @param encoded The encoded output. Typically received from the topDown outputs
		*              from the spatial pooler just above us.
		*
		* @returns A list of EncoderResult named tuples. Each EncoderResult has
		*        three attributes:
		*
		*        -# value:         This is the best-guess value for the sub-field
		*                          in a format that is consistent with the type
		*                          specified by getDecoderOutputFieldTypes().
		*                          Note that this value is not necessarily
		*                          numeric.
		*
		*        -# scalar:        The scalar representation of this best-guess
		*                          value. This number is consistent with what
		*                          is returned by getScalars(). This value is
		*                          always an int or float, and can be used for
		*                          numeric comparisons.
		*
		*        -# encoding       This is the encoded bit-array
		*                          that represents the best-guess value.
		*                          That is, if 'value' was passed to
		*                          encode(), an identical bit-array should be
		*                          returned.
		*/
		virtual vector< tuple<boost::any, boost::any, vector<nupic::UInt>> > topDownCompute(vector<nupic::UInt> encoded) override;

		bool isValidEncoderPropertyTuple(tuple<int, double> encoderPropertyTuple);

		bool isValidEncoderPropertyTuple(tuple<int, vector<string>> encoderPropertyTuple);

		void addChildEncoder(ScalarEncoder2<double> encoder);

		void addCustomDays(vector<string> daysList);

		tuple<int, double> getSeason();

		void setSeason(tuple<int, double> season);

		tuple<int, double> getDayOfWeek();

		void setDayOfWeek(tuple<int, double> dayOfWeek);

		tuple<int, double> getWeekend();

		void setWeekend(tuple<int, double> weekend);

		tuple<int, vector<string>> getCustomDays();

		void setCustomDays(tuple<int, vector<string>> customDays);

		tuple<int, double> getHoliday();

		void setHoliday(tuple<int, double> holiday);

		tuple<int, double> getTimeOfDay();

		void setTimeOfDay(tuple<int, double> timeOfDay);

		/**
		* Should return the output width, in bits.
		* @Override
		*/
		virtual int getWidth() const override;

		int getN() const;

		int getW() const;


	protected:

		int width_;

		// Ignore leap year differences -- assume 366 days in a year		 
		// Value is number of days since beginning of year (0 - 355)
		// Radius = 91.5 days = length of season 
		tuple<int, double> season_;

		// Value is day of week (floating point), Radius is 1 day  
		tuple<int, double> dayOfWeek_;

		// Binary value.
		tuple<int, double> weekend_;

		// Custom days encoder, first argument in tuple is width
		// second is either a single day of the week or a list of the days
		// you want encoded as ones. 
		tuple<int, vector<string>> customDays_;

		// A "continuous" binary value. = 1 on the holiday itself and smooth ramp
		//  0->1 on the day before the holiday and 1->0 on the day after the holiday.
		tuple<int, double> holiday_;

		// Value is time of day in hours
		// Radius = 4 hours, e.g. morning, afternoon, evening, early night,
		// late night, etc.  
		tuple<int, double> timeOfDay_;

		vector<int> customDaysList;

		// Currently the only holiday we know about is December 25
		// holidays is a list of holidays that occur on a fixed date every year
		vector<tuple<int, int>> holidaysList_;

		ScalarEncoder2<double> seasonEncoder;
		ScalarEncoder2<double> dayOfWeekEncoder;
		ScalarEncoder2<double> weekendEncoder;
		ScalarEncoder2<double> customDaysEncoder;
		ScalarEncoder2<double> holidayEncoder;
		ScalarEncoder2<double> timeOfDayEncoder;
		 
		vector<tuple<string, ScalarEncoder2<double>, nupic::UInt> >  scalarEncoders;
		 
	}; // end class DateEncoder


	   /***************** Start of Implementation ******************/
	template<class T>
	DateEncoder<T>::DateEncoder() : width_(), season_(0, 91.5), dayOfWeek_(0, 1.0), weekend_(0, 1.0),holiday_(0, 1.0), timeOfDay_(0, 4.0)		
	{
		holidaysList_.push_back(tuple<int, int>(12, 25));
	}


	template<class T>
	DateEncoder<T>::~DateEncoder() {}


	template<class T>
	void DateEncoder<T>::init() {

		width_ = 0;

		// Because most of the ScalarEncoder fields have less than 21 bits(recommended in
		// ScalarEncoder.checkReasonableSettings), so for now we set forced to be true to
		// override.
		// TODO figure out how to remove this
		this->setForced(true);

		// Note: The order of adding encoders matters, must be in the following
		// season, dayOfWeek, weekend, customDays, holiday, timeOfDay
		if (isValidEncoderPropertyTuple(season_)) {
			int w = (int)std::get<0>(season_);
			int minVal = 0;
			int maxVal = 366;
			double radius = (double)std::get<1>(season_);
			string name = "season";
			bool periodic = true;
			bool forced = this->isForced();

			// w, n, minVal,  maxVal, name, radius,  resolution, periodic,  forced
			seasonEncoder.init(w, 0, minVal, maxVal, name, radius, 0, periodic, forced);
			addChildEncoder(seasonEncoder); 
		} 

		if (isValidEncoderPropertyTuple(dayOfWeek_)) {
			int w = (int)std::get<0>(dayOfWeek_);
			int minVal = 0;
			int maxVal = 7;
			double radius = (double)std::get<1>(dayOfWeek_);
			string name = "day of week";
			bool periodic = true;
			bool forced = this->isForced();

			// w, n, minVal,  maxVal, name, radius,  resolution, periodic,  forced
			dayOfWeekEncoder.init(w, 0, minVal, maxVal, name, radius, 0, periodic, forced);
			addChildEncoder(dayOfWeekEncoder);
		}

		if (isValidEncoderPropertyTuple(weekend_)) {
			int w = (int)std::get<0>(weekend_);
			int minVal = 0;
			int maxVal = 1;
			double radius = (double)std::get<1>(weekend_);
			string name = "weekend";
			bool periodic = false;
			bool forced = this->isForced();

			// w, n, minVal,  maxVal, name, radius,  resolution, periodic,  forced
			weekendEncoder.init(w, 0, minVal, maxVal, name, radius, 0, periodic, forced);
			addChildEncoder(weekendEncoder);
		}

		if (isValidEncoderPropertyTuple(customDays_)) {
			vector<string> days = get<1>(customDays_);
			stringstream customDayEncoderName;

			if (days.size() == 1) {
				customDayEncoderName << days[0];
			}
			else {
				for (string day : days) {
					customDayEncoderName << day << " ";
				}
			}

			int w = (int)std::get<0>(customDays_);
			int minVal = 0;
			int maxVal = 1;
			double radius = 1;
			string name = customDayEncoderName.str();
			bool periodic = false;
			bool forced = this->isForced();

			// w, n, minVal,  maxVal, name, radius,  resolution, periodic,  forced
			customDaysEncoder.init(w, 0, minVal, maxVal, name, radius, 0, periodic, forced);
			addChildEncoder(customDaysEncoder);
			addCustomDays(days);
		}

		if (isValidEncoderPropertyTuple(holiday_)) {
			int w = (int)std::get<0>(holiday_);
			int minVal = 0;
			int maxVal = 1;
			double radius = (double)std::get<1>(holiday_);
			string name = "holiday";
			bool periodic = false;
			bool forced = this->isForced();

			// w, n, minVal,  maxVal, name, radius,  resolution, periodic,  forced
			holidayEncoder.init(w, 0, minVal, maxVal, name, radius, 0, periodic, forced);
			addChildEncoder(holidayEncoder);
		}

		if (isValidEncoderPropertyTuple(timeOfDay_)) {
			int w = (int)std::get<0>(timeOfDay_);
			int minVal = 0;
			int maxVal = 24;
			double radius = (double)std::get<1>(timeOfDay_);
			string name = "time of day";
			bool periodic = true;
			bool forced = this->isForced();

			// w, n, minVal,  maxVal, name, radius,  resolution, periodic,  forced
			timeOfDayEncoder.init(w, 0, minVal, maxVal, name, radius, 0, periodic, forced);
			addChildEncoder(timeOfDayEncoder);
		}
	}


	template<class T>
	bool DateEncoder<T>::isValidEncoderPropertyTuple(tuple<int, double> encoderPropertyTuple) {
		return (std::get<0>(encoderPropertyTuple) != 0);
	}


	template<class T>
	bool DateEncoder<T>::isValidEncoderPropertyTuple(tuple<int, vector<string>> encoderPropertyTuple) {
		return (get<0>(encoderPropertyTuple) != 0);
	}


	template<class T>
	void DateEncoder<T>::addChildEncoder(ScalarEncoder2<double> encoder) {
		scalarEncoders.push_back(tuple<string, ScalarEncoder2<double>, int>(encoder.getName(), encoder, width_));

		for (auto d : encoder.getDescription()) {
			tuple<string, int> dT = d;
			this->description_.push_back(tuple<string, int>(get<0>(dT), (int)get<1>(dT) + getWidth()));
		}
		width_ += encoder.getWidth();
		 
	}


	template<class T>
	void DateEncoder<T>::addCustomDays(vector<string> daysList) {
		for (string dayStr : daysList)
		{
			//tolower(dayStr)
			transform(dayStr.begin(), dayStr.end(), dayStr.begin(), ::tolower);

			if (dayStr == "mon" || dayStr == "monday") {
				customDaysList.push_back(1);
			}
			else if (dayStr == "tue" || dayStr == "tuesday") {
				customDaysList.push_back(2);
			}
			else if (dayStr == "wed" || dayStr == "wed") {
				customDaysList.push_back(3);
			}
			else if (dayStr == "thu" || dayStr == "thursday") {
				customDaysList.push_back(4);
			}
			else if (dayStr == "fri" || dayStr == "friday") {
				customDaysList.push_back(5);
			}
			else if (dayStr == "sat" || dayStr == "saturday") {
				customDaysList.push_back(6);
			}
			else if (dayStr == "sun" || dayStr == "sunday") {
				customDaysList.push_back(0);
			}
			else {
				NTA_THROW << "Unable to understand %s as a day of week" << dayStr ; 
			}
		}
	}


	template<class T>
	void DateEncoder<T>::encodeIntoArray(T inputData, vector<nupic::UInt>& output) {
		if (boost::any_cast<time_t>(inputData) == 0) NTA_THROW << "DateEncoder requires a valid Date object but got null" ;

		// Get the scalar values for each sub-field
		vector<double> scalars = getScalars(boost::any_cast<time_t>(inputData));
		int fieldCounter = 0;

		for (tuple<string, ScalarEncoder2<double>, int> t : scalarEncoders) {
			ScalarEncoder2<double> encoder = get<1>(t);
			vector<nupic::UInt> tempArray;
			encoder.encodeIntoArray(scalars[fieldCounter], tempArray);
			output.insert(output.end(), tempArray.begin(), tempArray.end());

			++fieldCounter;
		}
	}


	template<class T>
	vector<double> DateEncoder<T>::getScalars(time_t inputData) {
		if (inputData == 0)	NTA_THROW << "DateEncoder requires a valid Date object but got null";

		vector<double> values;
		tm * input = localtime(&inputData);

		//Get the scalar values for each sub-field
		double timeOfDay = input->tm_hour + (input->tm_min / 60.0) + (input->tm_sec / 3600.0);

		if (!seasonEncoder.isnull()) {
			// The day of year was 0 based
			double dayOfYear = input->tm_yday;
			values.push_back(dayOfYear);
		}

		if (!dayOfWeekEncoder.isnull()) {
			double dayOfWeek = (double)(input->tm_wday - 1) + (timeOfDay / 24.0);

			values.push_back(dayOfWeek);
		}

		if (!weekendEncoder.isnull()) {
			// saturday, sunday or friday evening

			// see date.py for more information //in C++:  tm->tm_wday : day of week [0,6], Fri == 5, Sat == 6, Sunday = 0, 
			int weekend;
			if ((input->tm_wday == 5 || input->tm_wday == 6 || input->tm_wday == 0) && timeOfDay > 18) {
				weekend = 1;
			}
			else {
				weekend = 0;
			}

			values.push_back(weekend);
		}

		if (!customDaysEncoder.isnull()) {
			bool isCustomDays = (find(customDaysList.begin(), customDaysList.end(), (int)input->tm_wday) != customDaysList.end());
			int customDay = isCustomDays ? 1 : 0;

			values.push_back(customDay);
		}

		if (!holidayEncoder.isnull()) {
			// A "continuous" binary value. = 1 on the holiday itself and smooth ramp
			//  0->1 on the day before the holiday and 1->0 on the day after the holiday.
			double holidayness = 0;

			for (tuple<int, int> h : holidaysList_) {
				//hdate is midnight on the holiday
				time_t hdate = Utils<int>::makedate(((int)input->tm_year + 1900), (int)get<0>(h), (int)get<1>(h), 0, 0 ,-1);
				long  diff = difftime(inputData, hdate);  // in second							
				long days = diff / 86400;	// calculate day

				if (inputData > hdate) {
					if (days == 0) {
						//return 1 on the holiday itself
						holidayness = 1.;
						break;
					}
					else if (days == 1) {
						//ramp smoothly from 1 -> 0 on the next day
						holidayness = 1.0 - ((diff - 86400.0 * days) / 86400.0);
						break;
					}
				}
				else {
					//TODO This is not the same as when date.isAfter(hdate), why?
					if (days == 0) {
						//ramp smoothly from 0 -> 1 on the previous day
						holidayness = 1.0 - ((diff - 86400.0 * days) / 86400.0);
						//TODO Why no break?
					}
				}
			}
			values.push_back(holidayness);
		}

		if (!timeOfDayEncoder.isnull()) {
			values.push_back(timeOfDay);
		}

		return values;
	}


	template<class T>
	DecodeResult DateEncoder<T>::decode(vector<nupic::UInt> encoded, string parentFieldName) {

		string parentName = parentFieldName.empty() || parentFieldName == "" ? this->getName() : parentFieldName + "." + this->getName();

		vector<tuple<string, ScalarEncoder2<double>, nupic::UInt> > encoders = scalarEncoders;
		const int len = encoders.size();

		map<string, RangeList> fieldsMap;
		vector<string> fieldsOrder;

		for (int i = 0; i < len; i++) {
			tuple<string, ScalarEncoder2<double>, nupic::UInt> threeFieldsTuple = encoders[i];
			int nextOffset = 0;
			if (i < len - 1) {
				nextOffset = (int)get<2>(encoders[i + 1]);
			}
			else {
				nextOffset = getW();
			}
			vector<nupic::UInt> fieldOutput = Utils<nupic::UInt>::sub(encoded, Utils<int>::range(get<2>(threeFieldsTuple), nextOffset));
			ScalarEncoder2<double> scalar_ = get<1>(threeFieldsTuple);
			DecodeResult result = scalar_.decode(fieldOutput, parentName);

			map<string, RangeList> fields = result.getFields();
			vector<string> fieldsDesc = result.getDescriptions();

			fieldsMap.insert(fields.begin(), fields.end());							
			fieldsOrder.insert(fieldsOrder.end(), fieldsDesc.begin(), fieldsDesc.end());
		}

		return DecodeResult(fieldsMap, fieldsOrder);
	}


	template<class T>
	vector<nupic::UInt> DateEncoder<T>::getBucketIndices(time_t input) {

		//See nupic.htm Encoder.java for more information
		vector<double> scalars = getScalars(input);
		vector<tuple<string, ScalarEncoder2<double>, nupic::UInt> > encoders = scalarEncoders;
		vector<nupic::UInt> l;

		if (!encoders.empty() && encoders.size() > 0) {
			int i = 0;
			for (auto &tuple : encoders) {
				ScalarEncoder2<double> scalar_ = get<1>(tuple);
				vector<nupic::UInt> bucketIdx = scalar_.getBucketIndices(scalars[i]);
				l.insert(l.end(), bucketIdx.begin(), bucketIdx.end());
				++i;
			}
		}
		else {
			NTA_THROW << "Should be implemented in base classes that are not containers for other encoders" ; 
		}

		return l;
	}


	template<class T>
	vector< tuple<boost::any, boost::any, vector<nupic::UInt>> > DateEncoder<T>::getBucketInfo(vector<nupic::UInt> buckets) {
		//See nupic.htm Encoder.java for more information

		//Concatenate the results from bucketInfo on each child encoder
		vector< tuple<boost::any, boost::any, vector<nupic::UInt>> > retVals;
		int bucketOffset = 0;

		for (tuple<string, ScalarEncoder2<double>, nupic::UInt> encoderTuple : scalarEncoders) {
			int nextBucketOffset = -1;
			ScalarEncoder2<double> scalar_ = get<1>(encoderTuple);

			// small change of python base.py(def_ getBucketInfo(...))
			if (scalar_.isnull()) {
				nextBucketOffset = bucketOffset + scalarEncoders.size();
			}
			else {
				nextBucketOffset = bucketOffset + 1;
			}

			vector<nupic::UInt> bucketIndices = Utils<nupic::UInt>::sub(buckets, Utils<int>::range(bucketOffset, nextBucketOffset));
			vector< tuple<boost::any, boost::any, vector<nupic::UInt>> > values = scalar_.getBucketInfo(bucketIndices);

			retVals.insert(retVals.end(), values.begin(), values.end());
			bucketOffset = nextBucketOffset;
		}

		return retVals;
	}

	  
	template<class T>
	vector< tuple<boost::any, boost::any, vector<nupic::UInt>> > DateEncoder<T>::topDownCompute(vector<nupic::UInt> encoded) {

		//See nupic.htm Encoder.java for more information
		vector< tuple<boost::any, boost::any, vector<nupic::UInt>> > retVals;

		vector<tuple<string, ScalarEncoder2<double>, nupic::UInt> > encoders = scalarEncoders;
		int len = encoders.size();
		for (int i = 0; i < len; i++) {
			int offset = (int)get<2>(encoders[i]);

			int nextOffset;
			if (i < len - 1) {
				nextOffset = (int)get<2>(encoders[i + 1]);
			}
			else {
				nextOffset = getW();
			}

			vector<nupic::UInt> fieldOutput = Utils<nupic::UInt>::sub(encoded, Utils<int>::range(offset, nextOffset));
			ScalarEncoder2<double>  encoder = get<1>(encoders[i]);
			vector< tuple<boost::any, boost::any, vector<nupic::UInt>> > values = encoder.topDownCompute(fieldOutput);

			retVals.insert(retVals.end(), values.begin(), values.end()); 
		}

		return retVals;
	}


	//////
	template<class T>
	tuple<int, double> DateEncoder<T>::getSeason() {
		return season_;
	}

	template<class T>
	void DateEncoder<T>::setSeason(tuple<int, double> season) {
		season_ = season;
	}

	template<class T>
	tuple<int, double> DateEncoder<T>::getDayOfWeek() {
		return dayOfWeek_;
	}

	template<class T>
	void DateEncoder<T>::setDayOfWeek(tuple<int, double> dayOfWeek) {
		dayOfWeek_ = dayOfWeek;
	}

	template<class T>
	tuple<int, double> DateEncoder<T>::getWeekend() {
		return weekend_;
	}

	template<class T>
	void DateEncoder<T>::setWeekend(tuple<int, double> weekend) {
		weekend_ = weekend;
	}

	template<class T>
	tuple<int, vector<string>> DateEncoder<T>::getCustomDays() {
		return customDays_;
	}

	template<class T>
	void DateEncoder<T>::setCustomDays(tuple<int, vector<string>> customDays) {
		customDays_ = customDays;
	}

	template<class T>
	tuple<int, double> DateEncoder<T>::getHoliday() {
		return holiday_;
	}

	template<class T>
	void DateEncoder<T>::setHoliday(tuple<int, double> holiday) {
		holiday_ = holiday;
	}

	template<class T>
	tuple<int, double> DateEncoder<T>::getTimeOfDay() {
		return timeOfDay_;
	}

	template<class T>
	void DateEncoder<T>::setTimeOfDay(tuple<int, double> timeOfDay) {
		timeOfDay_ = timeOfDay;
	}

	template<class T>
	int DateEncoder<T>::getWidth() const{
		return getN();
	}

	template<class T>
	int DateEncoder<T>::getN() const{
		return width_;
	}

	template<class T>
	int DateEncoder<T>::getW() const{ //FIXME is this correct?-all 3 are same!
		return width_;
	}

}; // end namespace DateEncoder
#endif // NTA_dateencoder_HPP
