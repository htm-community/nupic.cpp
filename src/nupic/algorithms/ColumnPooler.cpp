/* ---------------------------------------------------------------------
 * Numenta Platform for Intelligent Computing (NuPIC)
 * Copyright (C) 2013, Numenta, Inc.
 * Copyright (C) 2019, David McDougall
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
 *
 * http://numenta.org/licenses/
 * ----------------------------------------------------------------------
 */

/** @file
 * Implementation of ColumnPooler
 */

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <iomanip> // std::setprecision
#include <functional> // function

// #include <nupic/algorithms/ColumnPooler.hpp>
#include <nupic/types/Types.hpp>
#include <nupic/types/Serializable.hpp>
#include <nupic/types/Sdr.hpp>
#include <nupic/utils/SdrMetrics.hpp>
#include <nupic/algorithms/Connections.hpp>
#include <nupic/math/Math.hpp>
#include <nupic/math/Topology.hpp>

namespace nupic {
namespace algorithms {
namespace column_pooler {

using namespace std;
using namespace nupic;
using namespace nupic::math::topology;
using namespace nupic::algorithms::connections;


// TODO: Connections learning rules are different.


// Topology( location, potentialPool )
typedef function<void(SDR&, SDR&)> Topology_t;

class ColumnPooler // : public Serializable
{
private:
  vector<UInt> proximalInputDimensions_;
  vector<UInt> distalInputDimensions_;
  vector<UInt> inhibitionDimensions_;
  vector<UInt> cellDimensions_;
  UInt         cellsPerInhbitionArea_;
  UInt         proximalSegments_;

  vector<UInt> proximalMaxSegment_;

  Random rng_;
  vector<Real> tieBreaker_;
  UInt iterationNum_;
  UInt iterationLearnNum_;

public:
  const vector<UInt> &proximalInputDimensions = proximalInputDimensions_;
  const vector<UInt> &distalInputDimensions   = distalInputDimensions_;
  const vector<UInt> &inhibitionDimensions    = inhibitionDimensions_;
  const UInt         &cellsPerInhbitionArea   = cellsPerInhbitionArea_;
  const vector<UInt> &cellDimensions          = cellDimensions_;
  const UInt         &proximalSegments        = proximalSegments_;

  Real sparsity;

  UInt       proximalSegmentThreshold;
  Permanence proximalIncrement;
  Permanence proximalDecrement;
  Permanence proximalSynapseThreshold;

  UInt       distalMaxSegments;
  UInt       distalMaxSynapsesPerSegment;
  UInt       distalSegmentThreshold;
  Permanence distalIncrement;
  Permanence distalDecrement;
  Permanence distalMispredictDecrement;
  Permanence distalSynapseThreshold;

  SDR_ActivationFrequency *AF;
  Real stability_rate;
  Real fatigue_rate;
  vector<Real> X_act;
  vector<Real> X_inact;

  const UInt &iterationNum      = iterationNum_;
  const UInt &iterationLearnNum = iterationLearnNum_;

  /**
   * The proximal connections have regular structure.  Cells in an inhibition
   * area are contiguous and all segments on a cell are contiguous.  This
   * allows fast index math instead of slowers lists of ID's.
   */
  Connections proximalConnections;

  Connections distalConnections;

  ColumnPooler(
        const vector<UInt> proximalInputDimensions,
        const vector<UInt> distalInputDimensions,
        const vector<UInt> inhibitionDimensions,
        UInt               cellsPerInhbitionArea,

        Real sparsity,

        Topology_t potentialPool,
        UInt       proximalSegments,
        UInt       proximalSegmentThreshold,
        Permanence proximalIncrement,
        Permanence proximalDecrement,
        Permanence proximalSynapseThreshold,

        UInt       distalMaxSegments,
        UInt       distalMaxSynapsesPerSegment,
        UInt       distalSegmentThreshold,
        Permanence distalIncrement,
        Permanence distalDecrement,
        Permanence distalMispredictDecrement,
        Permanence distalSynapseThreshold,

        Real stability_rate,
        Real fatigue_rate,

        Real period,
        Int  seed,
        bool verbose) {
    proximalInputDimensions_ = proximalInputDimensions;
    distalInputDimensions_   = distalInputDimensions;
    inhibitionDimensions_    = inhibitionDimensions;
    cellsPerInhbitionArea_   = cellsPerInhbitionArea;
    proximalSegments_        = proximalSegments;
    this->sparsity                    = sparsity;
    this->proximalSegmentThreshold    = proximalSegmentThreshold;
    this->proximalIncrement           = proximalIncrement;
    this->proximalDecrement           = proximalDecrement;
    this->proximalSynapseThreshold    = proximalSynapseThreshold;
    this->distalMaxSegments           = distalMaxSegments;
    this->distalMaxSynapsesPerSegment = distalMaxSynapsesPerSegment;
    this->distalSegmentThreshold      = distalSegmentThreshold;
    this->distalIncrement             = distalIncrement;
    this->distalDecrement             = distalDecrement;
    this->distalSynapseThreshold      = distalSynapseThreshold;
    this->stability_rate              = stability_rate;
    this->fatigue_rate                = fatigue_rate;

    SDR proximalInputs(  proximalInputDimensions );
    SDR inhibitionAreas( inhibitionDimensions );
    cellDimensions_ = inhibitionAreas.dimensions;
    cellDimensions_.push_back( cellsPerInhbitionArea );
    SDR cells( cellDimensions_ );

    // Setup the proximal segments & synapses.
    proximalConnections.initialize(cells.size, proximalSynapseThreshold);
    SDR_Sparsity PP_Sp(            proximalInputs.dimensions, 10 * proximalInputs.size);
    SDR_ActivationFrequency PP_AF( proximalInputs.dimensions, 10 * proximalInputs.size);
    UInt cell = 0u;
    for(auto inhib = 0u; inhib < inhibitionAreas.size; ++inhib) {
      inhibitionAreas.setFlatSparse(SDR_flatSparse_t{ inhib });
      for(auto c = 0u; c < cellsPerInhbitionArea; ++c, ++cell) {
        for(auto s = 0u; s < proximalSegments; ++s) {
          auto segment = proximalConnections.createSegment( cell );

          // Make synapses.
          potentialPool( inhibitionAreas, proximalInputs );
          for(const auto presyn : proximalInputs.getFlatSparse() ) {
            auto permanence = initProximalPermanence();
            proximalConnections.createSynapse( segment, presyn, permanence);
          }
          proximalConnections.raisePermanencesToThreshold( segment,
                          proximalSynapseThreshold, proximalSegmentThreshold );
          PP_Sp.addData( proximalInputs );
          PP_AF.addData( proximalInputs );
        }
      }
    }

    rng_ = Random(seed);
    tieBreaker_.resize( proximalConnections.numSegments() );
    for(auto i = 0u; i < tieBreaker_.size(); ++i) {
      tieBreaker_[i] = 0.01f * rng_.getReal64();
    }
    proximalMaxSegment_.resize( proximalConnections.numCells() );
    AF = new SDR_ActivationFrequency( {proximalConnections.numCells(), proximalSegments}, period );
    // AF->activationFrequency_.assign( AF->activationFrequency.size(), sparsity / proximalSegments );
    iterationNum_      = 0u;
    iterationLearnNum_ = 0u;

    reset();

    if( PP_Sp.min() * proximalInputs.size < proximalSegmentThreshold )
      cerr << "Proximal segment has fewer synapses than the segment threshold." << endl;
    if( PP_AF.min() == 0.0f )
      cerr << "Proximal input is unused." << endl;

    if( verbose ) {
      // TODO: Print all parameters
      cout << "Potential Pool Statistics:" << endl
           << PP_Sp
           << PP_AF << endl;
    }
  }


  void reset() {
    X_act.assign( proximalConnections.numCells(), 0.0f );
    X_inact.assign( proximalConnections.numCells(), 0.0f );
    // TODO Zero Previous Updates
  }


  void compute(
        SDR& proximalInputActive,
        bool learn,
        SDR& active) {
    SDR none( distalInputDimensions );
    SDR none2( active.dimensions );
    compute(proximalInputActive, proximalInputActive, none, none, learn, active, none2 );
  }

  void compute(
        SDR& proximalInputActive,
        SDR& proximalInputLearning,
        SDR& distalInputActive,
        SDR& distalInputLearning,
        bool learn,
        SDR& active,
        SDR& learning) {
    NTA_CHECK( proximalInputActive.dimensions   == proximalInputDimensions );
    NTA_CHECK( proximalInputLearning.dimensions == proximalInputDimensions );
    NTA_CHECK( distalInputActive.dimensions     == distalInputDimensions );
    NTA_CHECK( distalInputLearning.dimensions   == distalInputDimensions );
    NTA_CHECK( active.dimensions                == cellDimensions );
    NTA_CHECK( learning.dimensions              == cellDimensions );

    // Update bookkeeping
    iterationNum_++;
    if( learn )
      iterationLearnNum_++;

    vector<Real> cellExcitements( active.size );
    activateProximalDendrites( proximalInputActive, cellExcitements );
    // auto predictiveCells = activateDistalDendrites( distalInputActive );

    activateCells( cellExcitements,
        // predictiveCells,
        active );

    if( learn ) {
      learnProximalDendrites( proximalInputActive, proximalInputLearning, active );
      // learnDistalDendrites( distalInputActive, distalInputLearning );
    }
  }


  // TODO: apply segment overlap threshold
  void activateProximalDendrites( SDR          &feedForwardInputs,
                             vector<Real> &cellExcitements )
  {
    // Proximal Feed Forward Excitement
    vector<UInt32> rawOverlaps( proximalConnections.numSegments(), 0.0f );
    proximalConnections.computeActivity(rawOverlaps, feedForwardInputs.getFlatSparse());

    const Real denominator = 1.0f / log2( sparsity / proximalSegments ); // For Boosting
    const auto &af = AF->activationFrequency; // For Boosting

    // Process Each Segment of Each Cell
    for(auto cell = 0u; cell < proximalConnections.numCells(); ++cell) {
      Real maxOverlap    = -1.0;
      UInt maxSegment    = -1;
      // UInt maxRawOverlap = 0u;
      for(const auto segment : proximalConnections.segmentsForCell( cell ) ) {
        const auto raw = rawOverlaps[segment];
        // maxRawOverlap = raw > maxRawOverlap ? raw : maxRawOverlap;

        Real overlap = (Real) raw; // Typecase to floating point.

        // Proximal Tie Breaker
        // NOTE: Apply tiebreakers before boosting, so that boosting is applied
        //   to the tiebreakers.  This is important so that the tiebreakers don't
        //   hurt the entropy of the result by biasing some mini-columns to
        //   activte more often than others.
        overlap += tieBreaker_[segment];

        // Normalize Proximal Excitement by the number of connected synapses.
        const auto nConSyns = proximalConnections.dataForSegment( segment ).numConnected;
        if( nConSyns == 0 )
          overlap = 1.0f;
          // overlap = 0.0f;
        else
          overlap /= nConSyns;

        // Boosting Function
        overlap *= log2( af[segment] ) * denominator;

        // Maximum Segment Overlap Becomes Cell Overlap
        if( overlap > maxOverlap ) {
          maxOverlap = overlap;
          maxSegment = segment;
        }
      }
      proximalMaxSegment_[cell] = maxSegment - cell;

      cellExcitements[cell] = maxOverlap;
      // Apply Stability & Fatigue
      // X_act[cell]   += stability_rate * (maxOverlap - X_act[cell] - X_inact[cell]);
      // X_inact[cell] += fatigue_rate   * (maxOverlap - X_inact[cell]);
      // cellExcitements[cell] = X_act[cell];
    }
  }


  // vector<UInt>& activateDistalDendrites() {
    // Concatenate external predictive inputs with self.active
    // TODO

    // Compute Feed Forward Overlaps.
    // TODO

    // Find Active Segments
    // TODO

    // Update book keeping
    // lastUsedIterationForSegment += active

    // Find Matching Segments
    // TODO

    // return cellsWithActiveSegments;
  // }


  void activateCells( vector<Real> &overlaps,
                 // predictiveCells,
                 SDR &activeCells)
  {
    const UInt inhibitionAreas = activeCells.size / cellsPerInhbitionArea;
    const UInt numDesired = (UInt) std::round(sparsity * cellsPerInhbitionArea);
    NTA_CHECK(numDesired > 0) << "Not enough cellsPerInhbitionArea ("
      << cellsPerInhbitionArea << ") for desired density (" << sparsity << ").";

    // Compare the cell indexes by their overlap.
    auto compare = [&overlaps](const UInt &a, const UInt &b) -> bool
      {return overlaps[a] > overlaps[b];};

    auto &active = activeCells.getFlatSparse();
    active.clear();
    active.reserve(cellsPerInhbitionArea + numDesired * inhibitionAreas );

    for(UInt offset = 0u; offset < activeCells.size; offset += cellsPerInhbitionArea)
    {
      // Sort the columns by the amount of overlap.  First make a list of all of
      // the mini-column indexes.
      auto activeBegin = active.end();
      for(UInt i = 0u; i < cellsPerInhbitionArea; i++)
        active.push_back( i + offset );
      // Do a partial sort to divide the winners from the losers.  This sort is
      // faster than a regular sort because it stops after it partitions the
      // elements about the Nth element, with all elements on their correct side of
      // the Nth element.
      std::nth_element(
        activeBegin,
        activeBegin + numDesired,
        active.end(),
        compare);
      // Remove the columns which lost the competition.
      active.resize( active.size() - (cellsPerInhbitionArea - numDesired) );
      // Finish sorting the winner columns by their overlap.
      // std::sort(activeBegin, active.end(), compare);
      // Remove sub-threshold winners
      // while( active.size() > offset &&
      //        overlaps[active.back()] < stimulusThreshold_)
      //     active.pop_back();
    }
    activeCells.setFlatSparse( active );
  }


  void learnProximalDendrites( SDR &proximalInputActive,
                               SDR &proximalInputLearning,
                               SDR &active ) {
    SDR AF_SDR( AF->dimensions );
    auto &activeSegments = AF_SDR.getSparse();
    for(const auto &cell : active.getFlatSparse())
    {
      // Adapt Proximal Segments
      const auto maxSegment = proximalMaxSegment_[cell];
      proximalConnections.adaptSegment(maxSegment, proximalInputActive,
                                       proximalIncrement, proximalDecrement);
      // connections_.raisePermanencesToThreshold(
      //                             cell, synPermConnected_, stimulusThreshold_);

      activeSegments[0].push_back(cell);
      activeSegments[1].push_back(maxSegment);
    }
    // TODO: Grow new synapses from the learning inputs?

    AF_SDR.setSparse( activeSegments );
    AF->addData( AF_SDR );
  }


  void learnDistalDendrites( SDR &activeCells ) {

    // Adapt Predicted Active Cells
    // TODO

    // Grow Unpredicted Active Cells
    // TODO

    // Punish Predicted Inactive Cells
    // TODO
  }


  Real initProximalPermanence(Real connectedPct = 0.5f) {
    if( rng_.getReal64() <= connectedPct )
        return proximalSynapseThreshold +
          (1.0f - proximalSynapseThreshold) * rng_.getReal64();
      else
        return proximalSynapseThreshold * rng_.getReal64();
  }
};


class DefaultTopology : public Topology_t
{
public:
  Real potentialPct;
  Real potentialRadius;
  bool wrapAround;

  DefaultTopology(Real potentialPct, Real radius, bool wrapAround)
  : potentialPct(potentialPct), potentialRadius(radius), wrapAround(wrapAround) 
  {}

  void operator()(SDR& cell, SDR& potentialPool) {

    vector<vector<UInt>> inputCoords;//(cell.dimensions.size());
    for(auto i = 0u; i < cell.dimensions.size(); i++)
    {
      const Real columnCoord = cell.getSparse()[i][0];
      const Real inputCoord = (columnCoord + 0.5f) *
                              (potentialPool.dimensions[i] / (Real)cell.dimensions[i]);
      inputCoords.push_back({ (UInt32)floor(inputCoord) });
    }
    potentialPool.setSparse(inputCoords);
    NTA_CHECK(potentialPool.getFlatSparse().size() == 1u);
    const auto centerInput = potentialPool.getFlatSparse()[0];

    vector<UInt> columnInputs;
    if (wrapAround) {
      for (UInt input : WrappingNeighborhood(centerInput, potentialRadius, potentialPool.dimensions)) {
        columnInputs.push_back(input);
      }
    } else {
      for (UInt input :
           Neighborhood(centerInput, potentialRadius, potentialPool.dimensions)) {
        columnInputs.push_back(input);
      }
    }

    const UInt numPotential = (UInt)round(columnInputs.size() * potentialPct);
    const auto selectedInputs = Random().sample<UInt>(columnInputs, numPotential);
    potentialPool.setFlatSparse( selectedInputs );
  }
};

}}} //-end ns
