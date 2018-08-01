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
// derived from code taken from https://stackoverflow.com/questions/1120140/how-can-i-read-and-parse-csv-files-in-c
//    answered May 20 '15 at 0:59 by @sastanin

#include "CSVReader.hpp"
namespace nupic 
{

enum class CSVState {
    UnquotedField,
    QuotedField,
    QuotedQuote
};

// Will return a vector of strings for each row
std::vector<std::string> readCSVRow(const std::string &row) {
    CSVState state = CSVState::UnquotedField;
    std::vector<std::string> fields {""};
    size_t i = 0; // index of the current field
    for (char c : row) {
        switch (state) {
            case CSVState::UnquotedField:
                switch (c) {
                    case ',': // end of field
                              fields.push_back(""); i++;
                              break;
                    case '"': state = CSVState::QuotedField;
                              break;
                    default:  fields[i].push_back(c);
                              break; }
                break;
            case CSVState::QuotedField:
                switch (c) {
                    case '"': state = CSVState::QuotedQuote;
                              break;
                    default:  fields[i].push_back(c);
                              break; }
                break;
            case CSVState::QuotedQuote:
                switch (c) {
                    case ',': // , after closing quote
                              fields.push_back(""); i++;
                              state = CSVState::UnquotedField;
                              break;
                    case '"': // "" -> "
                              fields[i].push_back('"');
                              state = CSVState::QuotedField;
                              break;
                    default:  // end of quote
                              state = CSVState::UnquotedField;
                              break; }
                break;
        }
    }
    return fields;
}

/// Read CSV file, Excel dialect. Accept "quoted fields ""with quotes"""
std::vector<std::vector<std::string>> readCSV(std::istream &in) {
    std::vector<std::vector<std::string>> table;
    std::string row;
    while (!in.eof()) {
        std::getline(in, row);
        if (in.bad() || in.fail()) {
            break;
        }
        auto fields = readCSVRow(row);
        table.push_back(fields);
    }
    return table;
}

// Read a CSV file, return a two dimentional array in a tuple.
// The first two fields of tuple are number of columns, number of rows/column.
// The third field is a shared pointer to an array of type T.
// returns empty tuple on errors.
template <typename T>
std::vector<std::tuple<Size, std::shared_ptr<T>>> readCSVArray(std::istream &in) {
	std::tuple<Size, std::shared_ptr<T>> result;
	std::vector<std::vector<std::string>> parsedStrings = readCSV(in);
  Size nrows = parsedStrings.size();
  Size ncol = (nrows > 0) ? parsedStrings[0].size() : 0;
  std::shared_ptr<T> arrayPtr(new T[ncol*nrows], std::default_delete<T[]>());

  bool err = false;
  T *ptr = arrayPtr.get();
  std::vector<std::vector<std::string>>::iterator c_itr;
  std::vector<std::string>::iterator r_itr;
  Size c, r = 0;
  for (c_itr = parsedStrings.begin(); c_itr != parsedStrings.end(); c_itr++) {
    if ((*c_itr).size() != nrows) {
      err = true;
      break;
    }
    r = 0;
    for (r_itr = (*c_itr).begin(); r_itr != (*c_itr).begin(); r_itr++) {
        T val = ::lexical_cast<T>(c_itr[i]);
      *(ptr + c * nrows + r) = val;
        r++;
    }
    c++;
  }
  if (err)
    return std::make_tuple(0, 0, arrayPtr);

  return std::make_tuple(ncol, nrows, arrayPtr);
}

} // namespace