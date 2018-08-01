/* ---------------------------------------------------------------------
 * Numenta Platform for Intelligent Computing (NuPIC)
 * Copyright (C) 2013, Numenta, Inc.  Unless you have an agreement
 * with Numenta, Inc., for a separate license for this software code, the
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
// derived from code taken from
// https://stackoverflow.com/questions/1120140/how-can-i-read-and-parse-csv-files-in-c
//    answered May 20 '15 at 0:59 by @sastanin

#ifndef _CSVREADER_HPP_
#define _CSVREADER_HPP_

#include <istream>
#include <string>
#include <vector>
#include <nupic/types/Types.hpp>
namespace nupic 
{


/// Read CSV file, Excel dialect. Accept "quoted fields ""with quotes"""
std::vector<std::vector<std::string>> readCSV(std::istream &in);

// Read a CSV file, return a two dimentional array in a tuple.
template <typename T>
std::tuple<Size, Size, std::shared_ptr<T>> readCSVArray(std::istream &in);

} // namespace
#endif // _CSVREADER_HPP_