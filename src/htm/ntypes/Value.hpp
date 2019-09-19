/* ---------------------------------------------------------------------
 * HTM Community Edition of NuPIC
 * Copyright (C) 2019, Numenta, Inc.
 *
 * David Keeney dkeeney@gmail.com
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
 * --------------------------------------------------------------------- */
#ifndef NTA_VALUE_HPP
#define NTA_VALUE_HPP

#include <htm/utils/Log.hpp>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <iostream>
#include <iterator>

namespace htm {

class Value_iterator;
class Value_const_iterator;

class Value {
public:
  Value();
  Value& parse(const std::string &yaml_string);

  bool contains(const std::string &key)const;
  size_t size() const;

  // type of value in the Value
  enum Category { Empty = 0, Scalar, Sequence, Map };
  Category getCategory() const;
  bool isScalar() const;
  bool isSequence() const;
  bool isMap() const;
  bool isEmpty() const;

  // Access
  Value operator[](const std::string &key);
  Value operator[](size_t index);
  const Value operator[](const std::string &key) const;
  const Value operator[](size_t index) const;
  template <typename T> T as() const;
  std::string str() const;
  std::vector<std::string> getKeys() const;


  class iterator {
    public:
      using value_type = Value;
      using reference = value_type &;
      using pointer = iterator *;
      using difference_type = std::ptrdiff_t;
      using iterator_category = std::forward_iterator_tag;
      iterator operator++();
      iterator operator++(int junk);
      reference operator*();
      pointer operator->();
      bool operator==(const iterator &rhs) const;
      bool operator!=(const iterator &rhs) const;

      struct OpaqueIterator;
      std::shared_ptr<OpaqueIterator> ptr_;
      std::string first;
      htm::Value second;
      template <typename T> T as() { return second->as<T>(); }
    };

    class const_iterator {
    public:
      using value_type = const Value;
      using reference = value_type &;
      using pointer = const_iterator *;
      using difference_type = std::ptrdiff_t;
      using iterator_category = std::forward_iterator_tag;
      const_iterator operator++();
      const_iterator operator++(int junk);
      reference operator*();
      pointer operator->();
      bool operator==(const const_iterator &rhs) const;
      bool operator!=(const const_iterator &rhs) const;

      struct OpaqueConstIterator;
      std::shared_ptr<OpaqueConstIterator> ptr_;
      std::string first;
      const htm::Value second;
      template <typename T> T as() { return second->as<T>(); }
    };

    iterator begin();
    iterator end();
    const_iterator cbegin();
    const_iterator cend();

  


    template <typename T> std::vector<T> asVector() const {
    std::vector<T> v;
    if (!isSequence())
      NTA_THROW << "Not a sequence node.";
    for (auto iter = cbegin(); iter != cend(); iter++) { // iterate through the children of this node.
      const Value& n = *iter;
      try {
        if (n.isScalar()) {
          v.push_back(n.as<T>());
        }
      } catch (std::exception e) {
        NTA_THROW << "Invalid vector element; " << e.what();
      }
    }
    return v;
  }
  template <typename T> std::map<std::string, T> asMap() const {
    std::map<std::string, T> v;
    if (!isSequence())
      NTA_THROW << "Not a dictionary node.";
    for (auto iter = cbegin(); iter != cend(); iter++) { // iterate through the children of this node.
      const Value& n = *iter;
      try {
        if (n.isScalar() && n.hasKey()) {
          v[n.key()] = n.as<T>();
        }
      } catch (std::exception e) {
        NTA_THROW << "Invalid map element; " << e.what;
      }
    }
  }

  // serializing routines
  std::string to_yaml() const;
  std::string to_json() const;


private:
  struct OpaqueTree; 
  std::shared_ptr<OpaqueTree> doc_; // This is an opaque pointer to a ryml Tree object.

};




class ValueMap : public Value {
public:
  // Access for backward compatability
  template <typename T> T getScalarT(const std::string &key) const {                   // throws
    return (*this)[key].as<T>();
  }
  template <typename T> T getScalarT(const std::string &key, T defaultValue) const {   // with default
    if (contains(key))
      return (*this)[key].as<T>();
    else
      return defaultValue;
  }
  std::string getString(const std::string &key, const std::string& defaultValue) const {
    if (contains(key))
      return (*this)[key].str();
    else
      return defaultValue;
  }
};

} // namespace htm

std::ostream &operator<<(std::ostream &f, const htm::ValueMap &vm);

#endif //  NTA_VALUEMAP_HPP
