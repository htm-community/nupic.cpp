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
 
#ifdef YAML_PARSER_RapidYaml

#include <htm/ntypes/ValueMap.hpp>
#include <htm/utils/Log.hpp>
#include <htm/ntypes/BasicType.hpp>

#include <ryml.hpp>
#include <ryml_std.hpp>
#include <iomanip>
#include <sstream>
#include <string>

using namespace htm;
using namespace ryml;
using namespace c4;

// some static helper functions
// The convert to and from the special string types used by ryml
static  csubstr _S(const std::string &s) { return to_csubstr(s.c_str()); }
static std::string S_(csubstr s) { return std::string(s.data(), s.size()); }

// Error handling.  This C function bootstraps into C++ to throw an Exception.
static void handleErrors_c(const char *msg, size_t msg_len, void *user_data) {
  ((ValueMap *)user_data)->handleErrors(std::string(msg, msg_len));
}

// This is just so ryml.hpp declarations are hidden from our .hpp
struct htm::ValueMap::OpaqueTree {
  c4::yml::NodeRef node;
  c4::yml::Tree t;
};
struct htm::ValueMap::iterator::OpaqueIterator {
  c4::yml::NodeRef node;
  c4::yml::NodeRef last;
  ValueMap vm;
};
struct htm::ValueMap::const_iterator::OpaqueConstIterator {
  c4::yml::NodeRef node;
  c4::yml::NodeRef last;
  const ValueMap vm;
};




// Constructor
ValueMap::ValueMap() { 
  doc_ = std::make_shared<OpaqueTree>(); // creates an empty Tree
  doc_->node = doc_->t.rootref();        // Node corresponding to the Tree root
}

// Parse YAML or JSON string document into the tree.
ValueMap &ValueMap::parse(const std::string &yaml_string) {
  set_error_flags(ON_ERROR_LOG|ON_ERROR_THROW);

  // This must be the top node so set the callbacks on this.
  //Callbacks const &cb_orig = get_callbacks();
  //Callbacks cb_new = cb_orig;
  //cb_new.m_error = handleErrors_c;
  //cb_new.m_user_data = this;
  //set_callbacks(cb_new);

  // Clear the tree, just in case there has been something already there
  // from a previous parse.
  doc_ = std::make_shared<OpaqueTree>(); // creates an empty Tree
  doc_->node = doc_->t.rootref();        // Node corresponding to the Tree root

  ryml::parse(_S(yaml_string), doc_->node); // populate tree at this node

  std::cout << "Tree: " << doc_->t;

  // If the value parsed is just a scalar, then the top node is set to a sequence
  // and the first child is the scalar. If a sequence was explictly specified with [ ]
  // or a leading '- ' and there is only one element we cannot tell the difference.
  // So, we have to see what the yaml_string starts with. If it is a '[' or '- ' then leave 
  // it as a Sequence.  If not, point to the single scalar child as the top node.
  if (doc_->node.is_seq() && doc_->node.num_children() == 1) {
    const char *p = yaml_string.c_str();
    while (isspace(*p)) p++;  // skip over leading spaces
    if (*p == '[' || (*p == '-' && *(p + 1) == ' ')) {
      // leave top node as a sequence
    } else {
      // make the top node a Scaler
      doc_->node = doc_->node.first_child();
    }
  }
  return *this;
}


// checking content of a parameter
//enum ValueMap::Category { Empty = 0, Scalar, Sequence, Map };
ValueMap::Category ValueMap::getCategory() const {
  if (doc_->node.is_map()) return ValueMap::Category::Map;
  if (doc_->node.is_seq()) return ValueMap::Category::Sequence;
  if (doc_->node.has_val()) return ValueMap::Category::Scalar;
  return ValueMap::Category::Empty;
}

bool ValueMap::contains(const std::string &key) const {return doc_->node[_S(key)].valid();}

bool ValueMap::isScalar() const { return doc_->node.has_val(); }
bool ValueMap::isSequence() const { return doc_->node.is_seq(); }
bool ValueMap::isMap() const { return doc_->node.is_map(); }
bool ValueMap::isEmpty() const { return getCategory() == ValueMap::Category::Empty; }

size_t ValueMap::size() const { return doc_->node.num_children(); }

// Accessing members of a map
ValueMap ValueMap::operator[](const std::string &key) {
  ValueMap vm = *this;
  vm.doc_->node = doc_->node[_S(key)];
  if (!vm.doc_->node.valid()) {
    NTA_THROW << "No value found for key '"+key+"'.";
  }
  return vm;
}
const ValueMap ValueMap::operator[](const std::string &key) const {
  ValueMap vm = *this;
  NTA_CHECK(doc_->node.is_map()) << "This is not a map.";
  vm.doc_->node = doc_->node[_S(key)];
  if (!vm.doc_->node.valid()) {
    NTA_THROW << "No value found for key '"+key+"'.";
  }
  return vm;
}

// accessing members of a sequence
ValueMap ValueMap::operator[](size_t index) {
  ValueMap vm = *this;
  NTA_CHECK(doc_->node.is_seq()) << "This is not a sequence.";
  vm.doc_->node = doc_->node[index];
  if (!vm.doc_->node.valid()) {
    NTA_THROW << "Index '"+std::to_string(index)+"' is out of bounds.";
  }
  return vm;
}
const ValueMap ValueMap::operator[](size_t index) const {
  ValueMap vm = *this;
  vm.doc_->node = doc_->node[index];
  if (!vm.doc_->node.valid()) {
    NTA_THROW << "Index '" + std::to_string(index) + "' is out of bounds.";
  }
  return vm;
}

std::string ValueMap::str() const {
  NodeRef r = doc_->node;
  if (r.valid() && r.has_val()) {
    return S_(r.val());
  } else {
    NTA_THROW << "Value not found.";
  }
}

// Return a value converted to the specified type T.
template <typename T> T ValueMap::as() const {
  NodeRef r = doc_->node;
  if (r.valid() && r.has_val()) {
    T v;
    r >> v;
    return v;
  } else {
    NTA_THROW << "Value not found.";
  }
}
// explicit instantiation.  Can only be used with these types.
template int8_t ValueMap::as<int8_t>() const;
template int16_t ValueMap::as<int16_t>() const;
template uint16_t ValueMap::as<uint16_t>() const;
template int32_t ValueMap::as<int32_t>() const;
template uint32_t ValueMap::as<uint32_t>() const;
template int64_t ValueMap::as<int64_t>() const;
template uint64_t ValueMap::as<uint64_t>() const;
template bool ValueMap::as<bool>() const;
template float ValueMap::as<float>() const;
template double ValueMap::as<double>() const;
template std::string ValueMap::as<std::string>() const;


std::string ValueMap::key() const {
  NodeRef r = doc_->node;
  return (r.has_key()) ? S_(r.key()) : "";
}


// Iterator
ValueMap::iterator ValueMap::iterator::operator++() {
  ptr_->node = (ptr_->node == ptr_->last)? NodeRef(): ptr_->node.next_sibling();
  return *this;
}
ValueMap::iterator ValueMap::iterator::operator++(int junk) {return operator++(); }
ValueMap &ValueMap::iterator::operator*() { ptr_->vm.doc_->node = ptr_->node; return ptr_->vm;}
ValueMap *ValueMap::iterator::operator->() { ptr_->vm.doc_->node = ptr_->node; return &ptr_->vm;}
bool ValueMap::iterator::operator==(const iterator &rhs) const { return ptr_->node == rhs.ptr_->node; }
bool ValueMap::iterator::operator!=(const iterator &rhs) const { return ptr_->node != rhs.ptr_->node; }

ValueMap::iterator ValueMap::begin() {
  ValueMap::iterator itr;
  itr.ptr_ = std::make_shared<iterator::OpaqueIterator>();
  if (doc_->node.has_children()) {
    itr.ptr_->node = doc_->node.first_child();
    itr.ptr_->last = doc_->node.last_child();
  }
  else
    itr.ptr_->node = NodeRef();   // the end()
  return itr;
}
ValueMap::iterator ValueMap::end() {
  ValueMap::iterator itr;
  itr.ptr_ = std::make_shared<iterator::OpaqueIterator>();
  itr.ptr_->node = NodeRef();
  return itr;
}

ValueMap::const_iterator ValueMap::const_iterator::operator++() {
  ptr_->node = (ptr_->node == ptr_->last) ? NodeRef() : ptr_->node.next_sibling();
  return *this;
}
ValueMap::const_iterator ValueMap::const_iterator::operator++(int junk) { return operator++();}
const ValueMap &ValueMap::const_iterator::operator*()  {ptr_->vm.doc_->node = ptr_->node; return ptr_->vm;}
const ValueMap *ValueMap::const_iterator::operator->() {ptr_->vm.doc_->node = ptr_->node; return &ptr_->vm;}
bool ValueMap::const_iterator::operator==(const const_iterator &rhs) const { return ptr_->node == rhs.ptr_->node; }
bool ValueMap::const_iterator::operator!=(const const_iterator &rhs) const { return ptr_->node != rhs.ptr_->node; }

ValueMap::const_iterator ValueMap::cbegin() const {
  ValueMap::const_iterator itr;
  itr.ptr_ = std::make_shared<const_iterator::OpaqueConstIterator>();
  if (doc_->node.has_children()) {
    itr.ptr_->node = doc_->node.first_child();
    itr.ptr_->last = doc_->node.last_child();
  } else
    itr.ptr_->node = NodeRef(); // the end()
  return itr;
}

ValueMap::const_iterator ValueMap::cend() const {
  ValueMap::const_iterator itr;
  itr.ptr_ = std::make_shared<const_iterator::OpaqueConstIterator>();
  itr.ptr_->node = NodeRef();
  return itr;
}


std::vector<std::string> ValueMap::getKeys() const {
  std::vector<std::string> v;
  NodeRef root = doc_->t.rootref();
  for (NodeRef n : root.children()) {
    if (n.has_key())
      v.push_back(S_(n.key()));
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


static void to_json(std::ostream& f, const htm::ValueMap& vm) {
  bool first = true;
  switch (vm.getCategory()) {
  case ValueMap::Empty:
    return;
  case ValueMap::Scalar:
    f << '"';
    escape_json(f, vm.as<std::string>());
    f << '"';
    break;
  case ValueMap::Sequence:
    f << "[";
    for (size_t i = 0; i < vm.size(); i++) {
      if (!first)
        f << ", ";
      first = false;
      to_json(f, vm[i]);
    }
    f << "]";
    break;
  case ValueMap::Map:
    f << "{";
    for (auto it = vm.cbegin(); it != vm.cend(); ++it) {
      if (!first)
        f << ", ";
      first = false;
      f << it->key() << ": ";
      to_json(f, *it);
    }
    f << "}";
    break;
  }
}

std::string ValueMap::to_json() const {
  std::stringstream f;
  ::to_json(f, *this);
  return f.str();
}

std::string ValueMap::to_yaml() const {
  std::stringstream ss;
  ss << doc_->t;
  return ss.str();
}


std::ostream &operator<<(std::ostream &f, const htm::ValueMap &vm) {
  f << vm.to_json();
  return f;
}

void ValueMap::handleErrors(const std::string &msg) 
{
  throw htm::LoggingException(__FILE__, __LINE__) << "Yaml parse error: " + msg;
}

#endif // YAML_PARSER_RapidYaml