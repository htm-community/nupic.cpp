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

/** @file STL IO
 * This file contains functions to print/restore as readable text
 * for a subset of the STL data structures.
 *
 * This include allows streaming of the following STL structures:
 *    std::pair<K,T>
 *    std::vector<T>
 *    std::map<K,T>
 *    std::set<T>
 *    std::shared_ptr<T>
 *    
 * Rules:
 *   1) The <T> can be any container in the list, any fundamental data
 *      type, or any subclass of Serializable.
 *   2) Cannot handle types that are raw pointers because the number of 
 *      elements are not known.
 *   3) std::shared_ptr<T> must not point to an array. The number of
 *      elements is unknown.
 *   4) Data types can be 'nested' as long as all data types are of the
 *      types in 1, 2, or 3.
 *
 */

#ifndef NTA_STL_IO_HPP
#define NTA_STL_IO_HPP

#include <list>
#include <map>
#include <set>
#include <vector>
#include <string>
#include <iostream>
#include <cctype>    // for isspace()
#include <typeinfo>  // for typeId
#include <algorithm> // for min
#include <iterator>  // for stream_iterator
//#include <iomanip> // stream manipulators

#include <nupic/types/Types.hpp>
#include <nupic/utils/Log.hpp>


namespace nupic {

//--------------------------------------------------------------------------------
// STL STREAMING HELPERS
//--------------------------------------------------------------------------------
static void ignore_whitespace(std::istream &in_stream) {
  while (isspace(in_stream.peek())) in_stream.ignore(1);
}

// adds some length syntax to stream so that a string value may contain spaces.
static void stringOut(std::ostream &out_stream, std::string fld) {
  out_stream << fld.length() << " |" << fld;
}
static void stringIn(std::istream &in_stream, std::string& fld) {
  size_t n;
  in_stream >> n;
  in_stream.ignore(2);
  std::istream_iterator<char> it(in_stream);
  std::copy_n(it, n, fld.begin());
}

//--------------------------------------------------------------------------------
// std::shared_ptr
// NOTE: This is assumed to NOT be an array.
//--------------------------------------------------------------------------------
template <typename T>
inline std::ostream &operator<<(std::ostream &out_stream, const std::shared_ptr<T> &p) {
  out_stream << (T&)*(p.get());
  return out_stream;
}

template <typename T>
inline std::istream &operator>>(std::istream &in_stream,  std::shared_ptr<T> &p) {
  ignore_whitespace(in_stream);
  std::shared_ptr<T> sp(new T);
  in_stream >> *(sp.get());
  p = sp;
  return in_stream;
}

//--------------------------------------------------------------------------------
// std::pair
//--------------------------------------------------------------------------------
template <typename T1, typename T2>
inline std::ostream &operator<<(std::ostream &out_stream, const std::pair<T1, T2> &p) {
  if (typeid(T1) == typeid(std::string))
    stringOut(out_stream, p.first);
  else
    out_stream << p.first;
  out_stream << " :";
  if (typeid(T2) == typeid(std::string))
    stringOut(out_stream, p.second);
  else
    out_stream << p.second;
  out_stream << " ";
  return out_stream;
}

template <typename T1, typename T2>
inline std::istream &operator>>(std::istream &in_stream, std::pair<T1, T2> &p) {
  ignore_whitespace(in_stream);
  if (typeid(T1) == typeid(std::string))
    stringIn(in_stream, p.first);
  else
    in_stream >> p.first;
  in_stream.ignore(2);
  if (typeid(T2) == typeid(std::string))
    stringIn(in_stream, p.second);
  else
    in_stream >> p.second;
  in_stream.ignore(1);
  return in_stream;
}

//--------------------------------------------------------------------------------
// std::vector
//--------------------------------------------------------------------------------

template <typename T>
inline std::ostream &operator<<(std::ostream &out_stream, const std::vector<T> &v) {
  out_stream << "[";
  out_stream << v.size() << " ";
  for(const auto& el : v) out_stream << el << " ";
  out_stream << "]";
  return out_stream;
}

template <typename T>
inline std::istream &operator>>(std::istream &in_stream, std::vector<T> &v) {
  ignore_whitespace(in_stream);
  NTA_CHECK(in_stream.get() == '[') << " was expecting a '[' as beginning of a Vector.";
  size_t n;
  in_stream >> n;
  v.clear();
  std::istream_iterator<char> it(in_stream);
  std::copy_n(it, n, back_inserter(v));
  in_stream.ignore(1);
  NTA_CHECK(in_stream.get() == ']') << " was expecting a ']' as end of a Vector.";
  return in_stream;
}


//--------------------------------------------------------------------------------
// std::set
//--------------------------------------------------------------------------------
template <typename T1>
inline std::ostream &operator<<(std::ostream &out_stream, const std::set<T1> &m) {
  out_stream << "[(" << m.size() << " )";
  for(const auto& el : m) out_stream << el << " ";
  out_stream << "]";
  return out_stream;
}

template <typename T>
inline std::istream &operator>>(std::istream &in_stream, std::set<T> &m) {
  ignore_whitespace(in_stream);
  NTA_CHECK(in_stream.get() == '[') << " was expecting a '[' as beginning of a set.";
  in_stream.ignore(1);
  size_t n;
  in_stream >> n;
  in_stream.ignore(2);
  m.clear();
  std::istream_iterator<char> it(in_stream); 
  std::copy_n(it, n, inserter(m));
  in_stream.ignore(1);
  NTA_CHECK(in_stream.get() == ']') << " was expecting a ']' as end of a set.";
  return in_stream;
}

//--------------------------------------------------------------------------------
// std::map
//--------------------------------------------------------------------------------
template <typename T1, typename T2>
  inline std::ostream& operator<<(std::ostream& out_stream, const std::map<T1, T2>& m)
  {
    out_stream << "{" << m.size() << "\n";
    std::ostream_iterator<std::pair<T1,T2>> it(out_stream," ");
    std::copy(m.begin(), m.end(), it);
    out_stream << "}\n";
  return out_stream;
}

template <typename T1, typename T2>
inline std::istream& operator>>(std::istream& in_stream, std::map<T1, T2>& m)
{
  ignore_whitespace(in_stream);
  NTA_CHECK(in_stream.get() == '{') << " was expecting a '{' as beginning of a map.";
  size_t n;
  in_stream >> n;
  m.clear();
  std::istream_iterator<char> it(in_stream); 
  std::copy_n(it, n, inserter(m));  // insert each pair

  NTA_CHECK(in_stream.get() == '}') << "Expected a closing '}' after map object.";
  in_stream.ignore(1);

  return in_stream;
}


//--------------------------------------------------------------------------------
} // end namespace nupic
#endif // NTA_STL_IO_HPP
