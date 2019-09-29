/* ---------------------------------------------------------------------
 * HTM Community Edition of NuPIC
 * Copyright (C) 2019, Numenta, Inc.
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
#ifdef YAML_PARSER_yamlcpp

#include <htm/ntypes/BasicType.hpp>
#include <htm/ntypes/Value.hpp>
#include <htm/utils/Log.hpp>

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <yaml-cpp/yaml.h>

using namespace htm;

// This is just so yaml-cpp/yaml.h declarations are hidden from our .hpp
struct htm::Value::OpaqueTree {
  YAML::Node node;
};
struct htm::Value::map_iterator::OpaqueIterator {
  YAML::iterator it;
  std::pair<std::string, Value> current_pair;
  Value current_node;
};
struct htm::Value::map_const_iterator::OpaqueConstIterator {
  YAML::const_iterator it;
  std::pair<std::string, Value> current_pair;
  Value current_node;
};

// Constructor
Value::Value() {
  doc_ = std::make_shared<OpaqueTree>(); // creates an empty Tree
}

// Parse YAML or JSON string document into the tree.
Value &Value::parse(const std::string &yaml_string) {
  doc_->node = YAML::Load(yaml_string);
  return *this;
}

// checking content of a parameter
// enum ValueMap::Category { Empty = 0, Scalar, Sequence, Map };
ValueMap::Category Value::getCategory() const {
  if (doc_->node.IsMap())
    return ValueMap::Category::Map;
  if (doc_->node.IsSequence())
    return ValueMap::Category::Sequence;
  if (doc_->node.IsScalar())
    return ValueMap::Category::Scalar;
  return ValueMap::Category::Empty;
}


bool Value::contains(const std::string &key) const { return (doc_->node[key]); }

bool Value::isScalar() const { return doc_->node.IsScalar(); }
bool Value::isSequence() const { return doc_->node.IsSequence(); }
bool Value::isMap() const { return doc_->node.IsMap(); }
bool Value::isEmpty() const { return getCategory() == Value::Category::Empty; }

size_t Value::size() const { return doc_->node.size(); }

// Accessing members of a map
// If not found, a Zombie Value object is returned.
// An error will be displayed when you try to access the value in the Zombie Value object.
// If you assign something to the Zombie Value, it will insert it into the tree with the saved key.
Value Value::operator[](const std::string &key) {
  Value v;
  v.doc_->node = doc_->node[key];
  return v;
}
const Value Value::operator[](const std::string &key) const {
  Value v;
  v.doc_->node = doc_->node[key];
  return v;
}


// accessing members of a sequence
Value Value::operator[](size_t index) {
  Value v;
  v.doc_->node = doc_->node[index];
  return v;
}
const Value Value::operator[](size_t index) const {
  Value v;
  v.doc_->node = doc_->node[index];
  return v;
}

std::string Value::str() const {
  return doc_->node.as<std::string>();
}

// Assign a value converted from a specified type T.
void Value::operator=(char *val) { doc_->node = val; }
void Value::operator=(const std::string&val) { doc_->node = val; }
void Value::operator=(int8_t val) { doc_->node = val; }
void Value::operator=(int16_t val) { doc_->node = val; }
void Value::operator=(uint16_t val) { doc_->node = val; }
void Value::operator=(int32_t val) { doc_->node = val; }
void Value::operator=(uint32_t val) { doc_->node = val; }
void Value::operator=(int64_t val) { doc_->node = val; }
void Value::operator=(uint64_t val) { doc_->node = val; }
void Value::operator=(bool val) { doc_->node = val; }
void Value::operator=(float val) { doc_->node = val; }
void Value::operator=(double val) { doc_->node = val; }
void Value::operator=(std::vector<UInt32> val) { doc_->node = val; }

// Compare two nodes recursively to see if content is same.
// yaml-cpp does equals by compairing pointers so we have to do our own.
static bool equals(const YAML::Node &n1, const YAML::Node &n2) {
  if (n1.IsScalar() && n2.IsScalar() && n1.as<std::string>() == n2.as<std::string>())
    return true;
  if (n1.IsSequence() && n2.IsSequence() && n1.size() == n2.size()) {
    for (size_t i = 0; i < n1.size(); i++)
      if (!equals(n1[i], n2[i])) return false;
    return true;
  }
  if (n1.IsMap() && n2.IsMap() && n1.size() == n2.size()) {
    for (auto it : n1) {
      if (!n2[it.first.as<std::string>()])
        return false;
      if (!equals(it.second, n2[it.first.as<std::string>()]))
        return false;
    }
    return true;
  }
  return false;
}
bool Value::operator==(const Value &v) { return equals(v.doc_->node,doc_->node); }

// Return a value converted to the specified type T.
template <typename T> T Value::as() const {
  return doc_->node.as<T>();
}
// explicit instantiation.  Can only be used with these types.
template int8_t Value::as<int8_t>() const;
template int16_t Value::as<int16_t>() const;
template uint16_t Value::as<uint16_t>() const;
template int32_t Value::as<int32_t>() const;
template uint32_t Value::as<uint32_t>() const;
template int64_t Value::as<int64_t>() const;
template uint64_t Value::as<uint64_t>() const;
template float Value::as<float>() const;
template double Value::as<double>() const;
template std::string Value::as<std::string>() const;
template <> bool Value::as<bool>() const { std::string val = doc_->node.as<std::string>();
  transform(val.begin(), val.end(), val.begin(), ::tolower);
  if (val == "true" || val == "on" || val == "1")
    return true;
  if (val == "false" || val == "off" || val == "0")
    return false;
  NTA_THROW << "Invalid value for a boolean. " << val;
}


template <typename T> T Value::as(T default_value) const { return doc_->node.as<T>(default_value); }
// explicit instantiation.  Can only be used with these types.
template int8_t Value::as<int8_t>(int8_t d) const;
template int16_t Value::as<int16_t>(int16_t d) const;
template uint16_t Value::as<uint16_t>(uint16_t d) const;
template int32_t Value::as<int32_t>(int32_t d) const;
template uint32_t Value::as<uint32_t>(uint32_t d) const;
template int64_t Value::as<int64_t>(int64_t d) const;
template uint64_t Value::as<uint64_t>(uint64_t d) const;
template float Value::as<float>(float d) const;
template double Value::as<double>(double d) const;
template std::string Value::as<std::string>(std::string d) const;
template <> bool Value::as<bool>(bool d) const {
  std::string val = doc_->node.as<std::string>(d?"true":"false");
  transform(val.begin(), val.end(), val.begin(), ::tolower);
  if (val == "true" || val == "on" || val == "1")
    return true;
  if (val == "false" || val == "off" || val == "0")
    return false;
  NTA_THROW << "Invalid value for a boolean. " << val;
}

// Map Iterator
Value::map_iterator Value::begin()  {
  NTA_CHECK(!isEmpty()) << "Not found";
  Value::map_iterator itr;
  itr.ptr_ = std::make_shared<Value::map_iterator::OpaqueIterator>();
  itr.ptr_->it = doc_->node.begin();
  return itr;
}
Value::map_iterator Value::end() {
  NTA_CHECK(!isEmpty()) << "Not found";
  Value::map_iterator itr;
  itr.ptr_ = std::make_shared<Value::map_iterator::OpaqueIterator>();
  itr.ptr_->it = doc_->node.end();
  return itr;
}
Value::map_iterator Value::map_iterator::operator++() {
  ptr_->it++;
  return *this;
}
Value::map_iterator Value::map_iterator::operator++(int junk) { return operator++(); }

Value &Value::map_iterator::operator*() {
  ptr_->current_node.doc_->node.reset(ptr_->it->second);
  return ptr_->current_node;
}
std::pair<std::string, Value> *Value::map_iterator::operator->() {
  ptr_->current_node.doc_->node.reset(ptr_->it->second);
  std::pair<std::string, Value> p(ptr_->it->first.as<std::string>(), ptr_->current_node);
  ptr_->current_pair = p;
  return &ptr_->current_pair;
}
bool Value::map_iterator::operator==(const Value::map_iterator &rhs) const { return ptr_->it == rhs.ptr_->it; }
bool Value::map_iterator::operator!=(const Value::map_iterator &rhs) const { return ptr_->it != rhs.ptr_->it; }


// Const Iterator
Value::map_const_iterator Value::cbegin() const {
  Value::map_const_iterator itr;
  itr.ptr_ = std::make_shared<Value::map_const_iterator::OpaqueConstIterator>();
  itr.ptr_->it = doc_->node.begin();
  return itr;
}

Value::map_const_iterator Value::cend() const {
  Value::map_const_iterator itr;
  itr.ptr_ = std::make_shared<Value::map_const_iterator::OpaqueConstIterator>();
  itr.ptr_->it = doc_->node.end();
  return itr;
}

Value::map_const_iterator Value::map_const_iterator::operator++() {
  ptr_->it++;
  return *this;
}
Value::map_const_iterator Value::map_const_iterator::operator++(int junk) { return operator++(); }

const Value &Value::map_const_iterator::operator*() {
  ptr_->current_node.doc_->node.reset( ptr_->it->second );
  return ptr_->current_node;
}
const std::pair<std::string, Value> *Value::map_const_iterator::operator->() {
  ptr_->current_node.doc_->node.reset(ptr_->it->second);
  ptr_->current_pair.first = ptr_->it->first.as<std::string>();
  ptr_->current_pair.second = ptr_->current_node;
  return &ptr_->current_pair;
}

bool Value::map_const_iterator::operator==(const Value::map_const_iterator &rhs) const {
  return ptr_->it == rhs.ptr_->it;
}
bool Value::map_const_iterator::operator!=(const Value::map_const_iterator &rhs) const {
  return ptr_->it != rhs.ptr_->it;
}




std::vector<std::string> Value::getKeys() const {
  NTA_CHECK(isEmpty()) << "Not found";
  NTA_CHECK(isMap()) << "This is not a map.";
  std::vector<std::string> v;
  for (auto it = doc_->node.begin(); it != doc_->node.end(); it++) {
    v.push_back(it->first.as<std::string>());
  }
  return v;
}

/**
 * a local function to apply escapes for a JSON string.
 */
static void escape_json(std::ostream &o, const std::string &s) {
  for (auto c = s.cbegin(); c != s.cend(); c++) {
    switch (*c) {
    case '"':
      o << "\\\"";
      break;
    case '\\':
      o << "\\\\";
      break;
    case '\b':
      o << "\\b";
      break;
    case '\f':
      o << "\\f";
      break;
    case '\n':
      o << "\\n";
      break;
    case '\r':
      o << "\\r";
      break;
    case '\t':
      o << "\\t";
      break;
    default:
      if ('\x00' <= *c && *c <= '\x1f') {
        o << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (int)*c;
      } else {
        o << *c;
      }
    }
  }
}

static void to_json(std::ostream &f, const htm::Value &v) {
  bool first = true;
  switch (v.getCategory()) {
  case Value::Empty:
    return;
  case Value::Scalar:
    f << '"';
    escape_json(f, v.as<std::string>());
    f << '"';
    break;
  case Value::Sequence:
    f << "[";
    for (size_t i = 0; i < v.size(); i++) {
      if (!first)
        f << ", ";
      first = false;
      Value n = v[i];
      to_json(f, n);
    }
    f << "]";
    break;
  case Value::Map:
    f << "{";
    for (auto it = v.cbegin(); it != v.cend(); it++) {
      if (!first)
        f << ", ";
      first = false;
      f << it->first << ": ";
      Value n = *it;
      to_json(f, n);
    }
    f << "}";
    break;
  }
}

std::string Value::to_json() const {
  std::stringstream f;
  ::to_json(f, *this);
  return f.str();
}

std::string Value::to_yaml() const {
  std::stringstream ss;
  ss << doc_->node;
  return ss.str();
}

namespace htm {
std::ostream &operator<<(std::ostream &f, const htm::Value &v) {
  f << v.to_json();
  return f;
}
} // htm namespace

#endif // YAML_PARSER_yamlcpp
