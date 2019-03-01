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

#ifndef NTA_rangelist_HPP
#define NTA_rangelist_HPP

#include <iostream>
#include <string>
#include <vector>
#include <tuple>
#include <map>



/**
* Convenience subclass of {@link Tuple} to contain the list of
* ranges expressed for a particular decoded output of an
* {@link Encoder} by using tightly constrained types without 
* the verbosity at the instantiation site.
* 
*/
class RangeList {

public:
	RangeList(){};

	/**
	* Constructs and new {@code Ranges} object.
	* 
	* @param l				the {@link List} of {@link MinMax} objects which are the 
	* 						minimum and maximum postions of 1's
	* @param str			the descriptions for all of the {@link MinMax}es
	*/
	RangeList(std::vector<std::pair<double, double>> l, std::string str  ) : l(l), desc(str) {};

	/**
	* Returns a List of the {@link MinMax}es.
	* @return
	*/
	std::vector<std::pair<double, double>> getRanges(){return l;};

	/**
	* Returns a comma-separated String containing the descriptions
	* for all of the {@link MinMax}es
	* @return
	*/
	std::string getDescription() {return desc;};

	/**
	* Adds a {@link MinMax} to this list of ranges
	* @param min	minimum value
	* @param max	maximum value
	*/
	void add(double min, double max){l.push_back(std::make_pair(min, max));};

	/**
	* Returns the specified {@link MinMax}
	*
	* @param index		the index of the MinMax to return
	* @return			the specified {@link MinMax}
	*/
	std::pair<double, double> getRange(int index){return l[index];};

	/**
	* Sets the entire comma-separated description string
	* @param s
	*/
	void setDescription(std::string s){desc = s;};

	/**
	* Returns the count of ranges contained in this Ranges object
	* @return
	*/
	int size(){return l.size();};

	/**
	* {@inheritDoc}
	*/
	std::string toString()
	{
		std::string str = "[ ";
        for (const auto& p : l)
        {
            str.append(std::to_string(p.first) + "  " + std::to_string(p.second) + " ");
        }
        str.append("]");
        return str;
	};
 
protected:

	// list of range minimum and maximum objects
	std::vector<std::pair<double, double>> l;

	// the descriptions of ranges
	std::string desc;

}; // end class RangeList


#endif // NTA_rangelist_HPP
