/* ---------------------------------------------------------------------
 * HTM Community Edition of NuPIC
 * Copyright (C) 2016, Numenta, Inc.
 *               2019, David McDougall (provided SDRClassifier from which this was derived)
 *               2021, David Keeney
 *
 * Unless you have an agreement with Numenta, Inc., for a separate license for
 * this software code, the following terms and conditions apply:
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

/** @file
 * Definitions for the Overlap Classifier.
 * 
 * `Classifier` learns mapping from SDR->input value (encoder's output). 
 * This is used when you need to "explain" the HTM network back to a real-world value, 
 * ie. mapping SDRs back to digits in MNIST digit classification task. 
 *
 * Unlike the SDRClassifier, the OverlapClassifier matches SDR patterns to its real-world value
 * by matching the patterns with the most number of bit overlap.
 */

#ifndef NTA_OVERLAP_CLASSIFIER_HPP
#define NTA_OVERLAP_CLASSIFIER_HPP

#include <deque>
#include <vector>
#include <map>

#include <htm/types/Types.hpp>
#include <htm/types/Sdr.hpp>
#include <htm/types/Serializable.hpp>

namespace htm {


/**
 * PDF: Probability Distribution Function.  Each index in this vector is a
 *      category label, and each value is the likelihood of the that category.
 *
 * See also:  https://en.wikipedia.org/wiki/Probability_distribution
 */
using PDF = std::vector<Real64>; //Real64 (not Real/float) must be used here, 
// ... otherwise precision is lost and Predictor never reaches sufficient results.




// A structure to hold the parts of the map key.
class Key : public Serializable {
public:
  Int32 bit;      // integer Bit index of an ON bit.
  Int64 category; // integer index of the category or bucket identifier

  CerealAdapter;
  template <class Archive> void save_ar(Archive &ar) const {
    ar(cereal::make_nvp("bit", bit), cereal::make_nvp("category", category));
  }
  template <class Archive> void load_ar(Archive &ar) {
    ar(cereal::make_nvp("bit", bit), cereal::make_nvp("category", category));
  }
};


/**
 * The Overlap Classifier takes the form of a multimap containing all learned SDRs
 * mapped to the indexes of the labels that created them.
 * It accepts SDRs as input and outputs a predicted distribution of categories.
 * Each input label index represents a bucket, a quantized value or category of the
 * real-world data.
 * 
 * Unlike the SDR Classifier, the Overlap Classifier requires only one sample of
 * each bucket in order to reliably identify the correct label given the SDR pattern.
 *
 * Categories are labeled using unsigned integers.  Other data types must be
 * enumerated or transformed into postitive integers.  There are as many output
 * units as the maximum category label.
 *
 * Example Usage:
 *
 *    // Example 1.
 *    // Make an SDR that is associated with the category B.
 *    // This could be as a result of passing the category through an encoder and SP and perhaps TM
 *    // For this example we will simulate it with a randomized SDR and associate it with category B.
 *    SDR inputData({ 1000 });
 *    inputData.randomize( 0.02 );
 *    enum Category { A, B, C, D };
 *    OverlapClassifier clsr;
 *    clsr.learn( inputData, { Category::B } );
 *    argmax( clsr.infer( inputData ) )  ->  Category::B
 *
 *    // Example 2
 *    // Learn a scalar value.  The Classifier only accepts categories, so
 *    // put real valued inputs into bins (AKA buckets) by subtracting the
 *    // minimum value and dividing by a resolution.
 *    double scalar = 567.8;
 *    double minimum = 500;
 *    double resolution = 10;
 *    clsr.learn( inputData, { (scalar - minimum) / resolution } );
 *    argmax( clsr.infer( inputData ) ) * resolution + minimum  ->  560
 *
 * During inference, the Overlap Classifier will locate the previously learned
 * entries by matching those with the most overlapping bits and then perform a 
 * softmax nonlinear function to get the predicted distribution of category labels.
 *
 * During learning, the SDR and its corresponding category index are stored in
 * a multimap.  This only takes one sample for each category to remember the bits.
 * If multiple SCR samples are given for a category, the OR of all SDR bits are stored 
 * for that category.
 *
 */
class OverlapClassifier : public Serializable
{
public:
  /**
   * Constructor.
   * @param theta - The threshold number of overlap bits an SDR must have to
   *                be considered a match.  A value of 0 means compute a reasonable
   *                value based on SDR size and sparsity.
   */
  OverlapClassifier(UInt theta = 0);

  /**
   * For use when deserializing.
   */
  void initialize(UInt theta);

  /**
   * Compute the likelihoods for each category / bucket.
   *
   * @param pattern: The SDR containing the active input bits.
   * @returns: The Probablility Distribution Function (PDF) of the categories.
   *           This is indexed by the category label.
   *           Or empty array ([]) if Classifier cannot find anything that matches what has been learned. 
   */
  PDF infer(const SDR & pattern) const;

  /**
   * Learn from example data.
   *
   * @param pattern:  The active input bit SDR.
   * @param categoryIdxList:  The current categories or bucket indices.
   *                          If more than one category is given in this list,
   *                          the same pattern is remembered for each.
   */
  void learn(const SDR & pattern, const std::vector<UInt> & categoryIdxList);

  CerealAdapter;
  template<class Archive>
  void save_ar(Archive & ar) const
  {
    ar(cereal::make_nvp("dimensions",    dimensions_),
       cereal::make_nvp("numCategories", numCategories_),
       cereal::make_nvp("theta",         theta_),
       cereal::make_nvp("learnedObjects",learnedObjects_));
  }

  template<class Archive>
  void load_ar(Archive & ar) {
    ar(cereal::make_nvp("dimensions",    dimensions_),
       cereal::make_nvp("numCategories", numCategories_), 
       cereal::make_nvp("theta",         theta_), 
       cereal::make_nvp("learnedObjects",learnedObjects_));
  }

  bool operator==(const OverlapClassifier &other) const;
  bool operator!=(const OverlapClassifier &other) const { return !operator==(other); }

  /**
   * Returns the category with the greatest probablility.
   */
  static UInt argmax(const PDF &data) { return UInt(max_element(data.begin(), data.end()) - data.begin()); }
  /**
   * Helper function for Classifier::infer.  Converts the raw data accumulators
   * into a PDF.
   */
  static void softmax(PDF::iterator begin, PDF::iterator end);


private:
  UInt dimensions_;       // expected dimension.  All SDR's must have the same dimensions.
  UInt numCategories_;    // The number of learned objects (size of the returned PDF array)
  UInt theta_;            // Minimum number of overlaps for a compare to be considered a match

  /**
   * A multimap used to store the learned data.
   *   Key is [ on bit index ][ category-index ]
   *   For each sample, there is an entry in the map for each [bit-index and category-index] combination.
   * The value in the map is the SDR associated during learning with the category-index.
   */
  struct KeyComp {
    bool operator()(const Key &lhs, const Key &rhs) const {
      return (lhs.bit < rhs.bit || (lhs.bit == rhs.bit && lhs.category < rhs.category)); 
    }
  };
  std::multimap<Key, SDR, KeyComp> learnedObjects_;

};





}       // End of namespace htm
#endif  // End of ifdef NTA_OVERLAP_CLASSIFIER_HPP
