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

#ifndef NTA_decoderesult_HPP
#define NTA_decoderesult_HPP
 
#include <iostream>
#include <string>
#include <vector>
#include <tuple>
#include <map> 

#include "RangeList.hpp"

using namespace std;

/**
* Tuple to contain the results of an {@link Encoder}'s decoded
* values.
* 
*/
class DecodeResult {

public:
	DecodeResult(){};

	/**
	* Constructs a new {@code Decode}
	* @param m		Map of field names to {@link RangeList} object
	* @param l		List of comma-separated descriptions for each list of ranges.
	*/
	DecodeResult(map<string, RangeList> m, vector<string> l) : fields(m), fieldDescriptions(l) {};

	/**
	* Returns the Map of field names to {@link RangeList} object
	* @return
	*/
	map<string, RangeList> getFields(){return fields;};

	/**
	* Returns the List of comma-separated descriptions for each list of ranges.
	* @return
	*/
	vector<string> getDescriptions(){return fieldDescriptions;};

	/**
	* Returns the {@link RangeList} associated with the specified field.
	* @param fieldName		the name of the field
	* @return
	*/
	RangeList getRanges(string fieldName){return fields.find(fieldName)->second;};

	/**
	* Returns a specific range ({@link MinMax}) for the specified field.
	* @param fieldName		the name of the field
	* @param index			the index of the range to return
	* @return
	*/
	pair<double, double> getRange(string fieldName, int index){return fields.find(fieldName)->second.getRange(index);};

protected:
	//Map of field names to {@link RangeList} object
	map<string, RangeList> fields;

	//List of comma-separated descriptions for each list of ranges
	vector<string> fieldDescriptions;

}; // end class RangeList
#endif // NTA_decoderesult_HPP
