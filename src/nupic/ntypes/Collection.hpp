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

#ifndef NTA_COLLECTION_HPP
#define NTA_COLLECTION_HPP

#include <string>
#include <vector>
#include <map>
#include <nupic/types/Serializable.hpp>
#include <nupic/utils/Log.hpp>

namespace nupic {
// A collection is a templated class that contains items of type t.
// The items are stored in a vector and keys are also stored in a map.
// It supports lookup by name and by index. O(nlogn)
// Iteration is either by consecutive indexes or by iterator. O(1)
// You can add items using the add() method. O(nlogn)
// You can delete itmes using the remove() method. O(n)
//
// The collections are expected to be fairly static and small.
// The deletions are rare so we can affort O(n) for delete.
//
// This has been re-implemnted as an inline header-only class.
// The map holds the key (always a string) and an index into vector.
// The vector holds a std::pair<key, Object>
//
// NOTE: This container can hold only items serializable by Cereal.  See Serializable.hpp


template <class T>
class Collection : public Serializable {
public:
  Collection() {}
  virtual ~Collection() {}

  typedef typename std::vector<std::pair<std::string, T>>::iterator Iterator;

  inline bool operator==(const Collection<T> &other) const {
      const static auto compare = [](std::pair<std::string, T> a,
                                     std::pair<std::string, T> b) {
          return a.first == b.first && a.second == b.second;
      };
    return std::equal(vec_.begin(), vec_.end(), other.vec_.begin(), compare);
  }
  inline bool operator!=(const Collection<T> &other) const {
    return !operator==(other);
  }
  inline size_t getCount() const { return vec_.size(); }

  // This method provides access by index to the contents of the collection
  // The indices are in insertion order.
  //
  inline const std::pair<std::string, T> &getByIndex(size_t index) const {
  	NTA_CHECK(index < vec_.size()) << "Collection index out-of-range.";
	return vec_[index];
  }
  inline std::pair<std::string, T> &getByIndex(size_t index) {
  	NTA_CHECK(index < vec_.size()) << "Collection index out-of-range.";
	return vec_[index];
  }

  inline bool contains(const std::string &name) const {
    return (map_.find(name) != map_.end());
  }

  inline T getByName(const std::string &name) const {
    auto itr = map_.find(name);
	  NTA_CHECK(itr != map_.end()) << "No item named: " << name;
	  return vec_[itr->second].second;
  }

  inline Iterator begin() {
  	return vec_.begin();
  }
  inline Iterator end() {
  	return vec_.end();
  }

  inline void add(const std::string &name, const T &item) {
    NTA_CHECK(!contains(name)) << "Unable to add item '" << name << "' to collection "
                << "because it already exists";
    // Add the new item to the vector
    vec_.push_back(std::make_pair(name, item));
	// Add the new item to the map
	size_t idx = vec_.size() - 1;
	map_[name] = idx;
  }

  void remove(const std::string &name) {
    auto itr = map_.find(name);
    NTA_CHECK(itr != map_.end()) << "No item named '" << name << "' in collection";
	size_t idx = itr->second;
	map_.erase(itr);
	vec_.erase(vec_.begin() + idx);
	// reset the indexes in the map
	for (size_t i = idx; i < vec_.size(); i++) {
		itr = map_.find(vec_[i].first);
		itr->second = i;
	 }
  }

	CerealAdapter;  // see Serializable.hpp
  template<class Archive>
  void save_ar(Archive & ar) const {
    ar(cereal::make_size_tag(static_cast<cereal::size_type>(map_.size())));
    for (auto it = vec_.begin(); it != vec_.end(); it++) {
      //vector order
      ar(cereal::make_map_item(it->first, it->second));
    }
  }
  template<class Archive>
  void load_ar(Archive & ar) {
    cereal::size_type count;
    ar(cereal::make_size_tag(count));
    for (size_t i = 0; i < static_cast<std::size_t>(count); i++) {
      std::string key;
      T value;

      ar( cereal::make_map_item(key, value) );
      add(std::move(key), std::move(value));
    }
  }

  friend std::ostream &operator<< (std::ostream &f, const Collection<T> &s) {
    f << "[\n";
    for (auto it = vec_.begin(); it != vec_.end(); it++) {
      //vector order
      f << "   {" << it->first << ": " << it->second << " }\n";
    }
    f << "]\n";
  } // namespace nupic


private:
  std::vector<std::pair<std::string, T>> vec_;
  std::map<std::string, size_t> map_;

};

} // nupic namespace

#endif // NTA_COLLECTION_HPP
