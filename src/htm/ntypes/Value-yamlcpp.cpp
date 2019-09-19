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
#include <sstream>
#include <string>
#include <yaml-cpp/yaml.h>

using namespace htm;

// This is just so yaml-cpp/yaml.h declarations are hidden from our .hpp
struct htm::Value::OpaqueTree {
  YAML::Node node;
};
struct htm::Value::iterator::OpaqueIterator {
  YAML::iterator it;
  YAML::iterator end;
};
struct htm::Value::const_iterator::OpaqueConstIterator {
  YAML::const_iterator it;
  YAML::const_iterator end;
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
Value Value::operator[](const std::string &key) {
  NTA_CHECK(isMap()) << "This is not a map.";
  Value v;
  if (doc_->node[key]) {
    v.doc_->node = doc_->node[key];
    return v;
  } else {
    NTA_THROW << "No value found for key '" + key + "'.";
  }
}
const Value Value::operator[](const std::string &key) const {
  NTA_CHECK(isMap()) << "This is not a map.";
  Value v;
  if (doc_->node[key]) {
    v.doc_->node = doc_->node[key];
    return v;
  } else {
    NTA_THROW << "No value found for key '" + key + "'.";
  }
}


// accessing members of a sequence
Value Value::operator[](size_t index) {
  NTA_CHECK(isSequence()) << "This is not a sequence.";
  Value v;
  if (doc_->node.size() > index) {
    v.doc_->node = doc_->node[index];
    return v;
  } else {
    NTA_THROW << "Index '" + std::to_string(index) + "' out of range.";
  }
}
const Value Value::operator[](size_t index) const {
  NTA_CHECK(isSequence()) << "This is not a sequence.";
  Value v;
  if (doc_->node.size() > index) {
    v.doc_->node = doc_->node[index];
    return v;
  } else {
    NTA_THROW << "Index '" + std::to_string(index) + "' out of range.";
  }
}

std::string Value::str() const {
  NTA_CHECK(isScalar()) << "This is not a scalar.";
  return doc_->node.as<std::string>();
}

// Return a value converted to the specified type T.
template <typename T> T Value::as() const {
  NTA_CHECK(isScalar()) << "This is not a scalar.";
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
template bool Value::as<bool>() const;
template float Value::as<float>() const;
template double Value::as<double>() const;
template std::string Value::as<std::string>() const;

// Iterator
Value::iterator Value::begin() {
  Value::iterator itr;
  itr.ptr_ = std::make_shared<Value::iterator::OpaqueIterator>();
  itr.ptr_->it = doc_->node.begin();
  itr.ptr_->end = doc_->node.end();
  return itr;
}
Value::iterator Value::end() {
  Value::iterator itr;
  itr.ptr_ = std::make_shared<Value::iterator::OpaqueIterator>();
  itr.ptr_->it = doc_->node.end();
  return itr;
}
Value::iterator Value::iterator::operator++() {
  ptr_->it++;
  return *this;
}
Value::iterator Value::iterator::operator++(int junk) { return operator++(); }

Value &Value::iterator::operator*() {
  second.doc_->node = ptr_->it->second;
  return second;
}
Value::iterator *Value::iterator::operator->() {
  first = ptr_->it->first.as<std::string>();
  second.doc_->node = ptr_->it->second;
  return this;
}
bool Value::iterator::operator==(const Value::iterator &rhs) const { return ptr_->it == rhs.ptr_->it; }
bool Value::iterator::operator!=(const Value::iterator &rhs) const { return ptr_->it != rhs.ptr_->it; }


// Const Iterator
Value::const_iterator Value::cbegin() const {
  Value::const_iterator itr;
  itr.ptr_ = std::make_shared<Value::const_iterator::OpaqueConstIterator>();
  itr.ptr_->it = doc_->node.begin();
  itr.ptr_->end = doc_->node.end();
  return itr;
}

Value_const::iterator Value::cend() const {
  Value::const_iterator itr;
  itr.ptr_ = std::make_shared<Value::const_iterator::OpaqueConstIterator>();
  itr.ptr_->it = doc_->node.end();
  return itr;
}

Value::const_iterator Value::const_iterator::operator++() {
  ptr_->it++;
  return *this;
}
Value::const_iterator Value::const_iterator::operator++(int junk) { return operator++(); }

const Value &Value::const_iterator::operator*() {
  second.doc_->node = ptr_->it->second;
  return second;
}
Value::const_iterator *Value::const_iterator::operator->() {
  first = ptr_->it->first.as<std::string>();
  second.doc_->node = ptr_->it->second;
  return this;
}
bool Value_const::iterator::operator==(const Value::const_iterator &rhs) const { return ptr_->it == rhs.ptr_->it; }
bool Value_const::iterator::operator!=(const Value::const_iterator &rhs) const { return ptr_->it != rhs.ptr_->it; }

std::vector<std::string> Value::getKeys() const {
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
      to_json(f, v[i]);
    }
    f << "]";
    break;
  case Value::Map:
    f << "{";
    for (auto it = v.cbegin(); it != v.cend(); ++it) {
      if (!first)
        f << ", ";
      first = false;
      f << it->first << ": ";
      to_json(f, *it);
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

std::ostream &operator<<(std::ostream &f, const htm::Value &v) {
  f << v.to_json();
  return f;
}


#endif // YAML_PARSER_yamlcpp
