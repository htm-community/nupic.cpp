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
 *    std::shared_ptr<T>    // no arrays
 *    
 * Rules:
 *   1) The <T> can be any container in the list, any fundamental data
 *      type, or any class that implements operator<< and operator>>.
 *      Note that std::strings that may contain whitespace require special
 *      handling with the stringIn() and stringOut() functions.
 *   2) Cannot handle types that are raw pointers because the number of 
 *      elements are not known.
 *   3) std::shared_ptr<T> must not point to an array. The number of
 *      elements is unknown.
 *   4) Data types can be 'nested' as long as all data types are of the
 *      types described in rules 1, 2, or 3.
 *
 *
 *   About Namespaces:
 *     In general you should have the >> and << operators in the same namespace 
 *     as the class upon which it operates.  Namespace nupic should work in most places.
 *     If you are using the >> or << operators with objects outside of nupic:: 
 *     the normal syntax may not work, i.e. nupic::algorithms::Cells4. So use this syntax:
 *          nupic::operator<<(outStream, _prevInfPatterns);
 *     https://stackoverflow.com/questions/5195512/namespaces-and-operator-resolution
 *
 */

#ifndef NTA_STL_IO_HPP
#define NTA_STL_IO_HPP

#include <deque>
#include <list>
#include <map>
#include <set>
#include <vector>
#include <string>
#include <iostream>

#include <typeinfo>             // for typeId
#include <iterator>             // for stream_iterator
#include <nupic/utils/Log.hpp>  // for CHECK( ) and THROW( ) macros



namespace nupic {

//
//--------------------------------------------------------------------------------
// STL STREAMING HELPERS
//--------------------------------------------------------------------------------
static size_t getLength(std::istream &in_stream) {
  in_stream >> std::ws;
  char s[20];
  in_stream.getline(s, sizeof(s), '|');
  NTA_CHECK(in_stream.get() == '|') << " was expecting a '|' as end of a length.";
  return strtoull(s, nullptr, 0);
}

//--------------------------------------------------------------------------------
// std::string   (that can include spaces)
//    len|val
//--------------------------------------------------------------------------------
// Adds some length syntax to stream so that a string value may contain spaces.
// Note that we cannot define our own operator>> for std::string
inline void stringOut(std::ostream &out_stream, std::string str) {
  out_stream << str.length() << "|" << str;
}
inline void stringIn(std::istream &in_stream, std::string& str) {
  size_t n = getLength(in_stream);
  std::istream_iterator<char> it(in_stream);
  std::copy_n(it, n, str.begin());
}

//--------------------------------------------------------------------------------
// Any unary STL container
//    [len| val val val ...]
//--------------------------------------------------------------------------------
// T is the container type, T1 is the type of elements in the container.

template <typename T, typename T1>
static void genaric_container(std::ostream &out_stream, const T &v) {
  out_stream << "[" << v.size() << "| ";
  if (typeid(T1) == typeid(std::string))
    for(const auto& el : v) stringOut(out_stream, el);
  else
    for(const auto& el : v) out_stream << el << " ";
  out_stream << "]";
}


template <typename T, typename T1>
static void genaric_container(std::istream &in_stream, T &v) {
  in_stream >> std::ws;  // skip whitespace
  NTA_CHECK(in_stream.get() == '[') << " was expecting a '[' as beginning of a container.";
  size_t n = getLength(in_stream);
  in_stream.ignore(1);
  v.clear();
  if (typeid(T1) == typeid(std::string)) {
    std::string el;
    for (size_t i = 0; i < n; i++) {
      stringIn(in_stream, el); 
      v.push_back(el);
    }
  }
  else {
    std::istream_iterator<T1> it(in_stream);
    std::copy_n(it, n, back_inserter(v));
  }
  in_stream.ignore(1);
  NTA_CHECK(in_stream.get() == ']') << " was expecting a ']' as end of a container.";
}



//--------------------------------------------------------------------------------
// std::shared_ptr
// NOTE: This is assumed to NOT be an array.
//     val
//--------------------------------------------------------------------------------
template <typename T>
inline std::ostream &operator<<(std::ostream &out_stream, const std::shared_ptr<T> &p) {
  out_stream << (T&)*(p.get());
  return out_stream;
}

template <typename T>
inline std::istream &operator>>(std::istream &in_stream,  std::shared_ptr<T> &p) {
  std::shared_ptr<T> sp(new T);
  in_stream >> *(sp.get());
  p = sp;
  return in_stream;
}

//--------------------------------------------------------------------------------
// std::pair
//    value1 : value2
//    len|strvalue : len|value2
//--------------------------------------------------------------------------------
template <typename T1, typename T2>
inline std::ostream &operator<<(std::ostream &out_stream, const std::pair<T1, T2> &p) {
  if (typeid(T1) == typeid(std::string))
    stringOut(out_stream, p.first);
  else
    out_stream << p.first;
  out_stream << " : ";
  if (typeid(T2) == typeid(std::string))
    stringOut(out_stream, p.second);
  else
    out_stream << p.second;
  out_stream << " ";
  return out_stream;
}

template <typename T1, typename T2>
inline std::istream &operator>>(std::istream &in_stream, std::pair<T1, T2> &p) {
  if (typeid(T1) == typeid(std::string))
    stringIn(in_stream, p.first);
  else
    in_stream >> p.first;
  in_stream.ignore(3);
  if (typeid(T2) == typeid(std::string))
    stringIn(in_stream, p.second);
  else
    in_stream >> p.second;
  in_stream.ignore(1);
  return in_stream;
}

//--------------------------------------------------------------------------------
// std::vector
//     [len| val val val ]
//--------------------------------------------------------------------------------

template <typename T>
inline std::ostream &operator<<(std::ostream &out_stream, const std::vector<T> &v) {
  genaric_container<std::vector<T>, T>(out_stream, v);
  return out_stream;
}

template <typename T>
inline std::istream &operator>>(std::istream &in_stream, std::vector<T> &v) {
  genaric_container<std::vector<T>, T>(in_stream, v);
  return in_stream;
}


//--------------------------------------------------------------------------------
// std::set
//    [len| val val val ]
//--------------------------------------------------------------------------------
template <typename T>
inline std::ostream &operator<<(std::ostream &out_stream, const std::set<T> &m) {
  out_stream << "[" << m.size() << "| ";
  for(const auto& el : m) out_stream << el << " ";
  out_stream << "]";
  return out_stream;
}

template <typename T>
inline std::istream &operator>>(std::istream &in_stream, std::set<T> &m) {
  in_stream >> std::ws;
  NTA_CHECK(in_stream.get() == '[') << " was expecting a '[' as beginning of a set.";
  size_t n = getLength(in_stream);
  in_stream.ignore(1);
  m.clear();
  std::istream_iterator<char> it(in_stream); 
  std::copy_n(it, n, inserter(m));
  in_stream.ignore(1);
  NTA_CHECK(in_stream.get() == ']') << " was expecting a ']' as end of a set.";
  return in_stream;
}

//--------------------------------------------------------------------------------
// std::map
//     {len| pair pair pair }
//--------------------------------------------------------------------------------
template <typename T1, typename T2>
inline std::ostream& operator<<(std::ostream& out_stream, const std::map<T1, T2>& m) {
    out_stream << "{" << m.size() << "\n";
    std::ostream_iterator<std::pair<T1,T2>> it(out_stream," ");
    std::copy(m.begin(), m.end(), it);
    out_stream << "}";
  return out_stream;
}

template <typename T1, typename T2>
inline std::istream& operator>>(std::istream& in_stream, std::map<T1, T2>& m)
{
  in_stream >> std::ws;
  NTA_CHECK(in_stream.get() == '{') << " was expecting a '{' as beginning of a map.";
  size_t n = getLength(in_stream);
  m.clear();
  std::istream_iterator<char> it(in_stream); 
  std::copy_n(it, n, inserter(m));  // insert each pair

  NTA_CHECK(in_stream.get() == '}') << "Expected a closing '}' after map object.";

  return in_stream;
}


//--------------------------------------------------------------------------------
// std::deque
//    [len| val val val ]
//--------------------------------------------------------------------------------
template <typename T>
inline std::ostream &operator<<(std::ostream &out_stream, const std::deque<T> &m) {
  out_stream << "[" << m.size() << "| ";
  for(const auto& el : m) out_stream << el << " ";
  out_stream << "]";
  return out_stream;
}

template <typename T>
inline std::istream &operator>>(std::istream &in_stream, std::deque<T> &m) {
  in_stream >> std::ws;
  NTA_CHECK(in_stream.get() == '[') << " was expecting a '[' as beginning of a set.";
  size_t n = getLength(in_stream);
  in_stream.ignore(1);
  m.clear();
  std::istream_iterator<char> it(in_stream); 
  std::copy_n(it, n, inserter(m));
  in_stream.ignore(1);
  NTA_CHECK(in_stream.get() == ']') << " was expecting a ']' as end of a set.";
  return in_stream;
}


//--------------------------------------------------------------------------------
// std::list
//    [len| val val val ]
//--------------------------------------------------------------------------------
template <typename T>
inline std::ostream &operator<<(std::ostream &out_stream, const std::list<T> &m) {
  out_stream << "[" << m.size() << "| ";
  for(const auto& el : m) out_stream << el << " ";
  out_stream << "]";
  return out_stream;
}

template <typename T>
inline std::istream &operator>>(std::istream &in_stream, std::list<T> &m) {
  in_stream >> std::ws;
  NTA_CHECK(in_stream.get() == '[') << " was expecting a '[' as beginning of a set.";
  size_t n = getLength(in_stream);
  in_stream.ignore(1);
  m.clear();
  std::istream_iterator<char> it(in_stream); 
  std::copy_n(it, n, inserter(m));
  in_stream.ignore(1);
  NTA_CHECK(in_stream.get() == ']') << " was expecting a ']' as end of a set.";
  return in_stream;
}




} // namespace nupic
//--------------------------------------------------------------------------------
#endif // NTA_STL_IO_HPP
