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
#include <algorithm>  // transform

namespace htm {



class Value {
public:
  Value();

  // Parse a Yaml or JSON string an assign it to this node in the tree.
  Value& parse(const std::string &yaml_string);


  bool contains(const std::string &key)const;
  size_t size() const;

  // type of value in the Value node
  enum Category { Empty = 0, Scalar, Sequence, Map };
  Category getCategory() const;
  bool isScalar() const;
  bool isSequence() const;
  bool isMap() const;
  bool isEmpty() const;  // false if current node has a value,sequence,or map
                         // true if operator[] did not find a value or was not assigned to.

  // Access
  Value operator[](const std::string &key);
  Value operator[](size_t index);
  const Value operator[](const std::string &key) const;
  const Value operator[](size_t index) const;
  template <typename T> T as() const;
  template <typename T> T as(T default_value) const;
  std::string str() const;
  std::vector<std::string> getKeys() const;

  /** Assign a value to a Value node in the tree.
   * If a previous operator[] did find a match, this does a replace.
   * If a previous operator[] did not find a match in the tree
   * the current Value is a Zombie and not attached to the tree.
   * But in this case its requested key is remembered.  A subsequent
   * operator= will assign this node to the tree with the remembered key.
   * The parent will become a map if it is not already.
   */
  void operator=(char *val);
  void operator=(const std::string& val);
  void operator=(int8_t val);
  void operator=(int16_t val);
  void operator=(uint16_t val);
  void operator=(int32_t val);
  void operator=(uint32_t val);
  void operator=(int64_t val);
  void operator=(uint64_t val);
  void operator=(bool val);
  void operator=(float val);
  void operator=(double val);
  void operator=(std::vector<UInt32>);

  // compare two nodes in the tree.
  bool operator==(const Value &v);

  // return false if node is empty:   if (vm) { do something }
  explicit operator bool() const { return !isEmpty(); }


  // extract a Vector
  template <typename T> std::vector<T> asVector() const {
    std::vector<T> v;
    if (!isSequence())
      NTA_THROW << "Not a sequence node.";
    for (size_t i = 0; i < size(); i++) { // iterate through the children of this node.
      const Value n = (*this)[i];
      try {
        if (n.isScalar()) {
          v.push_back(n.as<T>());
        }
      } catch (std::exception& e) {
        NTA_THROW << "Invalid vector element; " << e.what();
      }
    }
    return v;
  }

  // extract a map. Key is always a string.
  template <typename T> std::map<std::string, T> asMap() const {
    std::map<std::string, T> v;
    if (!isSequence())
      NTA_THROW << "Not a Map node.";
    for (auto iter = cbegin(); iter != cend(); iter++) { // iterate through the children of this node.
      const std::string key = iter->first;
      const Value n = iter->second;
      try {
        if (n.isScalar()) {
          v[key] = n.as<T>();
        }
        else {
          // non-scalar field.  Ignore
        }
      } catch (std::exception& e) {
        // probably bad conversion of scalar to requested type.
        NTA_THROW << "Invalid map element[" << key << "] " << e.what();
      }
    }
    return v;
  }


  // serializing routines
  std::string to_yaml() const;
  std::string to_json() const;


    // Access for backward compatability
  template <typename T> T getScalarT(const std::string &key) const { // throws
    return (*this)[key].as<T>();
  }
  template <typename T> T getScalarT(const std::string &key, T defaultValue) const { // with default
    return (*this)[key].as<T>(defaultValue);
  }
  std::string getString(const std::string &key, const std::string &defaultValue) const {
    return (*this)[key].as<std::string>(defaultValue);
  }

  friend std::ostream &operator<<(std::ostream &f, const htm::Value &vm);

  //    Iterator as a map
  class map_iterator {
  public:
    using value_type = Value;
    using reference = value_type &;
    using pointer = std::pair<std::string, Value> *;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;

    map_iterator operator++();
    map_iterator operator++(int junk);
    reference operator*();
    pointer operator->();
    bool operator==(const map_iterator &rhs) const;
    bool operator!=(const map_iterator &rhs) const;

    struct OpaqueIterator;
    std::shared_ptr<OpaqueIterator> ptr_;
  };

  class map_const_iterator {
  public:
    using value_type = const Value;
    using reference = value_type &;
    using pointer = const std::pair<std::string, Value> *;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;

    map_const_iterator operator++();
    map_const_iterator operator++(int junk);
    reference operator*();
    pointer operator->();
    bool operator==(const map_const_iterator &rhs) const;
    bool operator!=(const map_const_iterator &rhs) const;

    struct OpaqueConstIterator;
    std::shared_ptr<OpaqueConstIterator> ptr_;
  };


  map_iterator begin();
  map_iterator end();
  map_const_iterator begin() const { return cbegin(); }
  map_const_iterator end() const { return cend(); }
  map_const_iterator cbegin() const;
  map_const_iterator cend() const;


private:
  struct OpaqueTree;
  std::shared_ptr<OpaqueTree> doc_; // This is an opaque pointer to implementation Tree node object.

};

using ValueMap = Value;

} // namespace htm




#endif //  NTA_VALUE_HPP
