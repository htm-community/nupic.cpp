/* ---------------------------------------------------------------------
 * HTM Community Edition of NuPIC
 * Copyright (C) 2016, Numenta, Inc.
 *               2021, David Keeney
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

#include <cmath> // exp
#include <numeric> // accumulate

#include <htm/algorithms/OverlapClassifier.hpp>
#include <htm/utils/Log.hpp>

using namespace htm;
using namespace std;


/******************************************************************************/

OverlapClassifier::OverlapClassifier(UInt theta)
  { initialize(theta); }

void OverlapClassifier::initialize(UInt theta) {
  dimensions_ = 0;
  numCategories_ = 0u;
  learnedObjects_.clear();
  theta_ = theta;
}


void OverlapClassifier::learn(const SDR &pattern, const vector<UInt> &categoryIdxList) {
  // If this is the first time the Classifier is being used, the map is empty,
  // so we set the dimensions to that of the input `pattern`
  if (dimensions_ == 0) {
    dimensions_ = pattern.size;
  }
  NTA_CHECK(pattern.size > 0) << "No Data passed to OverlapClassifier. Pattern is empty.";
  NTA_ASSERT(pattern.size == dimensions_) << "Input SDR does not match previously seen size!";

  // If threshold theta is not specified in the constructor, assume it is 10% of sparsity with a minumum of 2.
  // So, if there are 40 out of 2000 1's in the first pattern, the theta threshold will be 4.
  // At least 4 bits must match before we say there is a probability of a match.
  if (theta_ == 0) {
    auto sparse_array = pattern.getSparse();
    theta_ = static_cast<UInt>(sparse_array.size() * 0.1);
    if (theta_ < 2) theta_ = 2;
  }

  // Check if this is a new category and adjust the numCategories accordingly.
  const auto maxCategoryIdx = *max_element(categoryIdxList.cbegin(), categoryIdxList.cend());
  if (maxCategoryIdx >= numCategories_) {
    numCategories_ = maxCategoryIdx + 1;
  }

  // Add this pattern to the map.
  for (const auto &bit : pattern.getSparse()) {
    for (UInt i : categoryIdxList) {
      // Insert this pattern if this bit/category combination is not in the map.
      Key key;
      key.bit = bit;
      key.category = i;
      std::multimap<Key, SDR>::iterator it = learnedObjects_.find(key);
      if (it == learnedObjects_.end() || it->first.bit != key.bit || it->first.category != key.category)
        learnedObjects_.insert(std::pair<Key, SDR>(key, pattern));
    }
  }
}


PDF OverlapClassifier::infer(const SDR &pattern) const {
  // NOTE: if the classifier cannot find any category that matches, the returned PDF array is empty.

  // Check input dimensions, or if this is the first time the Classifier is used and dimensions
  // are unset, return zeroes.
  NTA_CHECK(pattern.size > 0) << "No Data pased to Classifier. Pattern is empty.";
  if (dimensions_ == 0) {
    NTA_WARN << "Classifier: must call `learn` before `infer`.";
    return PDF(numCategories_, std::nan("")); //empty array []
  }
  NTA_ASSERT(pattern.size == dimensions_) << "Input SDR does not match previously seen size!";
  PDF probabilities(numCategories_, 0.0); // starts out being number of overlap bits then is converted to probability of match.

  // Accumulate the number of overlaps this pattern has for each category.
  bool has_match = false;
  for( const auto bit : pattern.getSparse() ) {
    // for each bit, use the multimap to get a list of all entries that contain at least this one bit.
    // This should shorten the search somewhat.
    Key key1;
    key1.bit = bit;
    key1.category = 0;
    std::multimap<Key, SDR>::const_iterator it = learnedObjects_.lower_bound(key1);
    while (it != learnedObjects_.end() && it->first.bit == key1.bit) {
      if (probabilities[it->first.category] == 0.0) {   // category already processed?
        // found at least one SDR in the map with this bit turned on that we have not yet seen.
        // See if that SDR has any other overlapping bits
        UInt cnt = pattern.getOverlap(it->second);
        probabilities[it->first.category] = static_cast<Real64>(cnt);
        if (cnt >= theta_)
          has_match = true;
      }
      it++;
    }
  }

  // Convert from accumulated votes to probability density function.
  if (has_match)
    softmax(probabilities.begin(), probabilities.end());
  else
    probabilities.clear();
  return probabilities;
}





bool OverlapClassifier::operator==(const OverlapClassifier &other) const {
  if (theta_ != other.theta_) return false;
  if (dimensions_ != other.dimensions_) return false; 
  if (numCategories_ != other.numCategories_) return false;
  if (learnedObjects_.size() != other.learnedObjects_.size())
    return false;
  std::multimap<Key, SDR>::const_iterator a, b;
  for (a = learnedObjects_.begin(), b = other.learnedObjects_.begin(); a != learnedObjects_.end(); a++, b++) {
    if (a->first.bit != b->first.bit)
      return false;
    if (a->first.category != b->first.category)
      return false;
    if (a->second != b->second)
      return false;
  }
  return true;
}

/**
 * Helper function for Classifier::infer.  Converts the raw data accumulators
 * into a PDF.
 */
void OverlapClassifier::softmax(PDF::iterator begin, PDF::iterator end) {
  if (begin == end) {
    return;
  }
  const auto maxVal = *max_element(begin, end);
  for (auto itr = begin; itr != end; ++itr) {
    *itr = std::exp(*itr - maxVal); // x[i] = e ^ (x[i] - maxVal)
  }
  // Sum of all elements raised to exp(elem) each.
  const Real sum = (Real)std::accumulate(begin, end, 0.0);
  NTA_ASSERT(sum > 0.0f);
  for (auto itr = begin; itr != end; ++itr) {
    *itr /= sum;
  }
}


/**************************************************************************


Predictor::Predictor(const vector<UInt> &steps, const Real alpha)
  { initialize(steps, alpha); }

void Predictor::initialize(const vector<UInt> &steps, const Real alpha)
{
  NTA_CHECK( not steps.empty() ) << "Required argument steps is empty!";
  steps_ = steps;
  sort(steps_.begin(), steps_.end());

  for( const auto step : steps_ ) {
    classifiers_.emplace( step, alpha );
  }

  reset();
}


void Predictor::reset() {
  patternHistory_.clear();
  recordNumHistory_.clear();
}


Predictions Predictor::infer(const SDR &pattern) const {
  Predictions result;
  for( const auto step : steps_ ) {
    result.insert({step, classifiers_.at(step).infer( pattern )});
  }
  return result;
}


void Predictor::learn(const UInt recordNum, //TODO make recordNum optional, autoincrement as steps 
		      const SDR &pattern,
                      const std::vector<UInt> &bucketIdxList)
{
  checkMonotonic_(recordNum);

  // Update pattern history if this is a new record.
  const UInt lastRecordNum = recordNumHistory_.empty() ? 0 : recordNumHistory_.back();
  if (recordNumHistory_.size() == 0u || recordNum > lastRecordNum) {
    patternHistory_.emplace_back( pattern );
    recordNumHistory_.push_back(recordNum);
    if (patternHistory_.size() > steps_.back() + 1u) { //steps_ are sorted, so steps_.back() is the "oldest/deepest" N-th step (ie 10 of [1,2,10])
      patternHistory_.pop_front();
      recordNumHistory_.pop_front();
    }
  }

  // Iterate through all recently given inputs, starting from the furthest in the past.
  auto pastPattern   = patternHistory_.begin();
  auto pastRecordNum = recordNumHistory_.begin();
  for( ; pastRecordNum != recordNumHistory_.cend(); pastPattern++, pastRecordNum++ )
  {
    const UInt nSteps = recordNum - *pastRecordNum;

    // Update weights.
    if( binary_search( steps_.begin(), steps_.end(), nSteps )) {
      classifiers_.at(nSteps).learn( *pastPattern, bucketIdxList );
    }
  }
}


void Predictor::checkMonotonic_(const UInt recordNum) const {
  // Ensure that recordNum increases monotonically.
  const UInt lastRecordNum = recordNumHistory_.empty() ? 0 : recordNumHistory_.back();
  NTA_CHECK(recordNum >= lastRecordNum) << "The record number must increase monotonically.";
}

**/
