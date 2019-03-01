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

#ifndef NTA_utils_HPP
#define NTA_utils_HPP

#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <tuple>
#include <map>
#include <list>
#include <array>         
#include <random>        
#include <cstring>
#include <algorithm> 
#include <limits> // minimum & maximum value
#include <time.h>
#include <ctime> 
#include <nupic/types/Types.hpp>

using namespace std;

/**
* Utilities to match some of the functionality found in Python's Numpy.
*/

template<class T>
class Utils {

public:

	enum Condition {
		WHERE_1,
		WHERE_GREATER_THAN_0,
		WHERE_GREATER_OR_EQUAL_0
	};

	/**
	* Copies the specified array, truncating or padding with zeros (if necessary)
	* so the copy has the specified length.  For all indices that are
	* valid in both the original array and the copy, the two arrays will
	* contain identical values.  For any indices that are valid in the
	* copy but not the original, the copy will contain <tt>0</tt>.
	* Such indices will exist if and only if the specified length
	* is greater than that of the original array.
	*
	* @param original the array to be copied
	* @param newLength the length of the copy to be returned
	* @return a copy of the original array, truncated or padded with zeros
	*     to obtain the specified length
	* @throws NegativeArraySizeException if <tt>newLength</tt> is negative
	* @throws NullPointerException if <tt>original</tt> is null
	* @since 1.6
	*/
	static vector<T> copyOf(vector<T> original, int newLength);


	/**
	* Copies the specified range of the specified array into a new array.
	* The initial index of the range (<tt>from</tt>) must lie between zero
	* and <tt>original.length</tt>, inclusive.  The value at
	* <tt>original[from]</tt> is placed into the initial element of the copy
	* (unless <tt>from == original.length</tt> or <tt>from == to</tt>).
	* Values from subsequent elements in the original array are placed into
	* subsequent elements in the copy.  The final index of the range
	* (<tt>to</tt>), which must be greater than or equal to <tt>from</tt>,
	* may be greater than <tt>original.length</tt>, in which case
	* <tt>(byte)0</tt> is placed in all elements of the copy whose index is
	* greater than or equal to <tt>original.length - from</tt>.  The length
	* of the returned array will be <tt>to - from</tt>.
	*
	* @param original the array from which a range is to be copied
	* @param from the initial index of the range to be copied, inclusive
	* @param to the final index of the range to be copied, exclusive.
	*     (This index may lie outside the array.)
	* @return a new array containing the specified range from the original array,
	*     truncated or padded with zeros to obtain the required length
	* @throws ArrayIndexOutOfBoundsException if {@code from < 0}
	*     or {@code from > original.length}
	* @throws IllegalArgumentException if <tt>from &gt; to</tt>
	* @throws NullPointerException if <tt>original</tt> is null
	* @since 1.6
	*/
	static vector<T> copyOfRange(vector<T> original, unsigned int from, unsigned int to);

	/**
	* Returns an array which starts from lowerBounds (inclusive) and
	* ends at the upperBounds (exclusive).
	*
	* @param lowerBounds
	* @param upperBounds
	* @return
	*/
	static vector<T> range(T lowerBounds, T upperBounds);

	/**
	* Returns an array which starts from lowerBounds (inclusive) and
	* ends at the upperBounds (exclusive).
	*
	* @param lowerBounds the starting value
	* @param upperBounds the maximum value (exclusive)
	* @param interval    the amount by which to increment the values
	* @return
	*/
	static vector<T> arange(T lowerBounds, T upperBounds, T interval);

	/**
	* Returns the index of the max value in the specified array
	* @param array the array to find the max value index in
	* @return the index of the max value
	*/
	static int argmax(vector<T> array); 

	/**
	* Sets the values in the specified values array at the indexes specified,
	* to the value "setTo".
	*
	* @param values  the values to alter if at the specified indexes.
	* @param indexes the indexes of the values array to alter
	* @param setTo   the value to set at the specified indexes.
	*/
	static void setIndexesTo(vector<T>& values, vector<int> indexes, T setTo);

	/**
	* Sets the values in range start to stop to the value specified. If
	* stop &lt; 0, then stop indicates the number of places counting from the
	* length of "values" back.
	*
	* @param values the array to alter
	* @param start  the start index (inclusive)
	* @param stop   the end index (exclusive)
	* @param setTo  the value to set the indexes to
	*/
	static void setRangeTo(vector<T>& values, int start, int stop, T setTo);

	/**
	* Returns a new array containing the items specified from
	* the source array by the indexes specified.
	*
	* @param source
	* @param indexes
	* @return
	*/
	static vector<T> sub(vector<T> source, vector<int> indexes);

	/**
	* Performs a modulus on every index of the first argument using
	* the second argument and places the result in the same index of
	* the first argument.
	*
	* @param a
	* @param b
	* @return
	*/
	static vector<T> modulo(vector<T> a, T b);

	/**
	* Performs a modulus operation in Python style.
	*
	* @param a
	* @param b
	* @return
	*/
	static T modulo(T a, T b);

	/**
	* Scans the specified values and applies the  Condition to each
	* value, returning the indexes of the values where the condition evaluates
	* to true.
	*
	* @param values the values to test
	* @param c      the condition used to test each value
	*
	* @return
	*/
	static vector<T> where(vector<T> values, int condition);

	/**
	* Returns a new int array containing the and'd bits of
	* both arg1 and arg2.
	*
	* @param arg1
	* @param arg2
	* @return
	*/
	static vector<T> and_(vector<T> arg1, vector<T> arg2);

	/**
	* Returns a new int array containing the and'd bits of
	* both arg1 and arg2.
	*
	* @param arg1
	* @param arg2
	* @return
	*/
	static vector<T> overlaping(vector<T> arg1, vector<T> arg2);

	/**
	* Returns the sum of all contents in the specified array.
	* @param array
	* @return
	*/
	static T sum(vector<T> array);

	/**
	* Returns a flag indicating whether the container list contains an
	* array which matches the specified match array.
	*
	* @param match     the array to match
	* @param container the list of arrays to test
	* @return true if so, false if not
	*/
	static bool contains(vector<T> match, vector<vector<T>> container) ;

	/**
	* Returns the passed in array with every value being altered
	* by the addition of the specified double amount at the same
	* index
	*
	* @param arr
	* @param amount
	* @return
	*/
	static vector<T> i_add(vector<T> arr, vector<T> amount);

	/**
	* Returns a new array containing the result of multiplying
	* each index of the specified array by the 2nd parameter.
	*
	* @param array
	* @param d
	* @return
	*/
	static vector<T> multiply(vector<T> array, T d);

	/**
	* <p>
	* Return a new double[] containing the difference of each element and its
	* succeding element.
	* </p><p>
	* The first order difference is given by ``out[n] = a[n+1] - a[n]``
	* along the given axis, higher order differences are calculated by using `diff`
	* recursively.
	*
	* @param d
	* @return
	*/
	static vector<double> diff(vector<double> d);

	/**
	* Returns the average of all the specified array contents.
	* @param arr
	* @return
	*/
	static double average(vector<double> arr);

	/**	
	* make date time
	* @param year
	* @param month
	* @param day
	* @param hour
	* @param min
	* @return
	*/
	static time_t makedate(int year, int month, int day , int hour, int min, int dl_savingtime );

	/**
	* Returns an array with the same shape and the contents
	* converted to integers.
	*
	* @param doubs an array of doubles.
	* @return
	*/
	static vector<unsigned int> toIntArray(vector<double> doubs);

	/**
	* Returns an array with the same shape and the contents
	* converted to doubles.
	*
	* @param ints an array of ints.
	* @return
	*/
	static vector<double> toDoubleArray(vector<T> ints);

	/**
	* Returns a new int array containing the or'd on bits of
	* both arg1 and arg2.
	*
	* @param arg1
	* @param arg2
	* @return
	*/
	static vector<T> or_(vector<T> arg1, vector<T> arg2); 

}; // end class Utils


template<class T>
vector<T> Utils<T>::copyOf(vector<T> original, int newLength){
	std::vector<T> newSrc(newLength);
	int len = (unsigned int)newLength > original.size() ? original.size() : newLength;
	std::copy(original.begin(), original.begin()+len, newSrc.begin());
	return newSrc;
}

template<class T>
vector<T> Utils<T>::copyOfRange(vector<T> original, unsigned int from, unsigned int to){
	unsigned int olen = original.size();
	from = from > olen ? olen : from;
	to   = to > olen ? olen : to;
	unsigned int newLength = to - from;
	vector<T> newSrc;
	if (newLength <= 0) return newSrc;
	newSrc.resize(newLength);
	std::copy(original.begin()+from, original.begin() + from + newLength, newSrc.begin());
	return newSrc;
}

template<class T>
vector<T> Utils<T>::range(T lowerBounds, T upperBounds) {
	vector<T> ints;
	ints.reserve(upperBounds - lowerBounds);
	for (T i = lowerBounds; i < upperBounds; i++) {
		ints.push_back(i);
	}
	return ints;
} 

template<class T>
vector<T> Utils<T>::arange(T lowerBounds, T upperBounds, T interval) {
	vector<T> doubs;
	for (T i = lowerBounds; i < upperBounds; i += interval) {
		doubs.push_back(i);
	}
	return doubs;
}


template<class T>
int Utils<T>::argmax(vector<T> array) {
	auto biggest = std::max_element(std::begin(array), std::end(array));
	return std::distance(std::begin(array), biggest);
	/*
	int index = -1;
	T max = std::numeric_limits<T>::min();
	for (unsigned int i = 0; i < array.size(); i++) {
	if (array[i] > max) {
	max = array[i];
	index = i;
	}
	}
	return index;*/
}

template<class T>
void Utils<T>::setIndexesTo(vector<T>& values, vector<int> indexes, T setTo) {
	for (unsigned int i = 0; i < indexes.size(); i++) {
		values[indexes[i]] = setTo;
	} 
}

template<class T>
void Utils<T>::setRangeTo(vector<T>& values, int start, int stop, T setTo) {
	stop = stop < 0 ? values.size() + stop : stop;
	for ( int i = start; i < stop; i++) {
		values[i] = setTo;
	} 
}

template<class T>
vector<T> Utils<T>::sub(vector<T> source, vector<int> indexes) {
	vector<T> retVal(indexes.size());
	for (unsigned int i = 0; i < indexes.size(); i++) {
		retVal[i] = source[indexes[i]];
	}
	return retVal;
}

template<class T>
vector<T> Utils<T>::modulo(vector<T> a, T b) {
	for (unsigned int i = 0; i < a.size(); i++) {
		a[i] = modulo(a[i], b);
	}
	return a;
}

template<class T>
T Utils<T>::modulo(T a, T b) {
	if (b == 0)	throw std::invalid_argument("Division by Zero!");
	if (a > 0 && b > 0 && b > a) return a;
	bool isMinus = abs(b - (a - b)) < abs(b - (a + b));
	if (isMinus) {
		while (a >= b) {
			a -= b;
		}
	}
	else {
		if (a % b == 0) return 0;
		while (a + b < b) {
			a += b;
		}
	}
	return a;
}

template<class T>
vector<T> Utils<T>::where(vector<T> values, int condition) {
	vector<T> retVal;
	const int len = values.size();
	retVal.reserve(len);
	switch (condition)
	{
	case WHERE_1:
		for ( int i = 0; i < len; i++)  
			if (values[i] == 1)  retVal.push_back(i);
		break;

	case WHERE_GREATER_THAN_0:
		for ( int i = 0; i < len; i++)  
			if (values[i] > 0)  retVal.push_back(i);
		break;

	case WHERE_GREATER_OR_EQUAL_0:
		for ( int i = 0; i < len; i++)  
			if (values[i] >= 0)  retVal.push_back(i);
		break;
	}
	return retVal;
}

template<class T>
vector<T> Utils<T>::and_(vector<T> arg1, vector<T> arg2){
	vector<T> retVal(max(arg1.size(), arg2.size()), (T) 0);

	for (unsigned int i = 0; i < arg1.size(); i++) {
		retVal[i] = arg1[i] > 0 && arg2[i] > 0 ? 1 : 0;
	}
	return retVal;
}

template<class T>
vector<T> Utils<T>::overlaping(vector<T> arg1, vector<T> arg2) {
	std::sort(arg1.begin(), arg1.end());
	std::sort(arg2.begin(), arg2.end());

	std::vector<T> v_intersection;
	std::set_intersection(arg1.begin(), arg1.end(),	arg2.begin(), arg2.end(), std::back_inserter(v_intersection));
	return v_intersection;
}

template<class T>
T Utils<T>::sum(vector<T> array) {
	T sum = 0;
	for (unsigned int i = 0; i < array.size(); i++) {
		sum += array[i];
	}
	return sum;
} 

template<class T>
bool Utils<T>::contains(vector<T> match, vector<vector<T>> container) { 
	const int len = container.size(); 

	for (int i = 0; i < len; i++) { 
		if ( equal(match.begin(), match.end(), container[i].begin())  ) {
			return true;
		}
	}
	return false;
}

template<class T>
vector<T>  Utils<T>::i_add(vector<T> arr, vector<T> amount){
	for (unsigned int i = 0; i < arr.size(); i++) {
		arr[i] += amount[i];
	}
	return arr;
}

template<class T>
vector<T> Utils<T>::multiply(vector<T> array, T d){
	vector<T> product(array.size(), 0);
	for (unsigned int i = 0; i < array.size(); i++) {
		product[i] = array[i] * d;
	}
	return product;
}

template<class T>
vector<double> Utils<T>::diff(vector<double> d) {
	vector<double> retVal(d.size() - 1, 0);
	for (unsigned int i = 0; i < retVal.size(); i++) {
		retVal[i] = d[i + 1] - d[i];
	}
	return retVal;
}

template<class T>
double  Utils<T>::average(vector<double> arr){
	double sum = 0;
	for (unsigned int i = 0; i < arr.size(); i++) {
		sum += arr[i];
	}
	return sum / (double)arr.size();
}

template<class T>
time_t Utils<T>::makedate(int year, int month, int day , int hour, int min , int dl_savingtime )
{
	struct tm tm = {0};

	tm.tm_sec = 0;					/* seconds after the minute - [0,59] */
	tm.tm_min = min;				/* minutes after the hour - [0,59] */
	tm.tm_hour = hour;				/* hours since midnight - [0,23] */
	tm.tm_mday = day;				/* day of the month - [1,31] */
	tm.tm_mon = month -1;			/* months since January - [0,11] */
	tm.tm_year = year - 1900;		/* years since 1900 */
	tm.tm_isdst = dl_savingtime;	/* daylight savings time flag */

	return mktime(&tm);
}

template<class T>
vector<unsigned int> Utils<T>::toIntArray(vector<double> doubs){
	vector<unsigned int> retVal(doubs.size());
	for (size_t i = 0; i < doubs.size(); i++) {
		retVal[i] = (int)doubs[i];
	}
	return retVal;
}

template<class T>
vector<double> Utils<T>::toDoubleArray(vector<T> ints) {
	vector<double> retVal(ints.size());
	for (size_t i = 0; i < ints.size(); i++) {
		retVal[i] = ints[i];
	}
	return retVal;
}

template<class T>
vector<T> Utils<T>::or_(vector<T> arg1, vector<T> arg2){
	vector<T> retVal(max(arg1.size(), arg2.size()));
	for (size_t i = 0; i < arg1.size(); i++) {
		retVal[i] = arg1[i] > 0 || arg2[i] > 0 ? 1 : 0;
	}
	return retVal;
}

#endif // NTA_utils_HPP
