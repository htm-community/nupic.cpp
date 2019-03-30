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
#include <algorithm> // accumulate
#include <limits>  // numeric_limits
#include <iomanip> // setprecision
#include <functional> // function, multiplies

// #include <nupic/algorithms/ColumnPooler.hpp>
#include <nupic/types/Types.hpp>
#include <nupic/types/Serializable.hpp>
#include <nupic/types/Sdr.hpp>
#include <nupic/utils/SdrMetrics.hpp>
#include <nupic/algorithms/Connections.hpp>
#include <nupic/algorithms/TemporalMemory.hpp>
#include <nupic/math/Math.hpp>
#include <nupic/math/Topology.hpp>

namespace nupic {
namespace algorithms {
namespace column_pooler {

using namespace std;
using namespace nupic;
using namespace nupic::sdr;
using namespace nupic::math::topology;
using namespace nupic::algorithms::connections;
using nupic::algorithms::temporal_memory::TemporalMemory;


// TODO: Connections learning rules are different...
// Don't apply the same learning update to a synapse twice in a row, because
// otherwise staring at the same object for too long will mess up the synapses.
// This change allows it to work with timeseries data which moves very slowly,
// instead of the usual HTM inputs which reliably change every cycle. See kropff
// & treves 2008

typedef function<SDR (const SDR&, const vector<UInt>&, Random&)> Topology_t;

typedef function<Permanence(Random&)> InitialPermanence_t;

struct Parameters
{
public:
  vector<UInt> proximalInputDimensions;
  vector<UInt> distalInputDimensions;
  vector<UInt> inhibitionDimensions;
  UInt         cellsPerInhibitionArea;

  Real sparsity;

  Topology_t  potentialPool;
  UInt        proximalSegments;
  UInt        proximalSegmentThreshold;
  Permanence  proximalIncrement;
  Permanence  proximalDecrement;
  Permanence  proximalSynapseThreshold;

  UInt       distalMaxSegments;
  UInt       distalMaxSynapsesPerSegment;
  UInt       distalSegmentThreshold;
  UInt       distalSegmentMatch;
  UInt       distalAddSynapses;
  Permanence distalInitialPermanence;
  Permanence distalIncrement;
  Permanence distalDecrement;
  Permanence distalMispredictDecrement;
  Permanence distalSynapseThreshold;

  Real stability_rate;    // TODO: Fix naming convention
  Real fatigue_rate;      // TODO: Fix naming convention

  UInt period;
  Int  seed;
  bool verbose;
};

const Parameters DefaultParameters = {
  // TODO
};

const Parameters DefaultParametersNoDistalDendrites = {
  // TODO
};

class ColumnPooler // : public Serializable
{
private:
  Parameters args_;
  vector<UInt> cellDimensions_;

  vector<UInt32> rawOverlaps_;
  vector<UInt> proximalMaxSegment_;

  vector<vector<Permanence>> proximalUpdates;

  Random rng_;
  vector<Real> tieBreaker_;
  UInt iterationNum_;
  UInt iterationLearnNum_;
  vector<Real> X_act;
  vector<Real> X_inact;

  // This is used by boosting.
  ActivationFrequency *AF_;

public:
  const Parameters   &parameters     = args_;
  const vector<UInt> &cellDimensions = cellDimensions_;

  const UInt &iterationNum      = iterationNum_;
  const UInt &iterationLearnNum = iterationLearnNum_;

  /**
   * The proximal connections have regular structure.  Cells in an inhibition
   * area are contiguous and all segments on a cell are contiguous.  This
   * allows fast index math instead of slowers lists of ID's.
   */
  Connections proximalConnections;

  TemporalMemory distalConnections;

  ColumnPooler() {}; //default constructor, must call initialize to setup properly

  ColumnPooler( const Parameters &parameters )
    { initialize( parameters ); }

  void initialize( const Parameters &parameters ) {
    NTA_CHECK( parameters.proximalSegments > 0u );
    NTA_CHECK( parameters.cellsPerInhibitionArea > 0u );

    args_ = parameters;
    SDR proximalInputs(  args_.proximalInputDimensions );
    SDR inhibitionAreas( args_.inhibitionDimensions );
    cellDimensions_ = inhibitionAreas.dimensions;
    cellDimensions_.push_back( args_.cellsPerInhibitionArea );
    SDR cells( cellDimensions_ );
    rng_ = Random(args_.seed);

    // Setup the proximal segments & synapses.
    proximalConnections.initialize(cells.size, args_.proximalSynapseThreshold);
    Sparsity            PP_Sp( proximalInputs.dimensions, cells.size * args_.proximalSegments * 2);
    ActivationFrequency PP_AF( proximalInputs.dimensions, cells.size * args_.proximalSegments * 2);
    UInt cell = 0u;
    for(auto inhib = 0u; inhib < inhibitionAreas.size; ++inhib) {
      inhibitionAreas.setSparse(SDR_sparse_t{ inhib });
      for(auto c = 0u; c < args_.cellsPerInhibitionArea; ++c, ++cell) {
        for(auto s = 0u; s < args_.proximalSegments; ++s) {
          auto segment = proximalConnections.createSegment( cell );

          // Make synapses.
          proximalInputs.setSDR(
            args_.potentialPool( inhibitionAreas, args_.proximalInputDimensions, rng_ ));
          NTA_CHECK(proximalInputs.getSum() > 0);
          for(const auto presyn : proximalInputs.getSparse() ) {
            auto permanence = initProximalPermanence();
            proximalConnections.createSynapse( segment, presyn, permanence);
          }
          proximalConnections.raisePermanencesToThreshold( segment,
                          args_.proximalSynapseThreshold, args_.proximalSegmentThreshold );
          PP_Sp.addData( proximalInputs );
          PP_AF.addData( proximalInputs );
        }
      }
    }
    tieBreaker_.resize( proximalConnections.numSegments() );
    for(auto i = 0u; i < tieBreaker_.size(); ++i) {
      tieBreaker_[i] = 0.01f * rng_.getReal64();
    }
    proximalMaxSegment_.resize( cells.size );
    AF_ = new ActivationFrequency( {cells.size, args_.proximalSegments}, args_.period );
    AF_->initializeToValue( args_.sparsity / args_.proximalSegments );

    // Setup the distal dendrites
    distalConnections.initialize(
        /* columnDimensions */            cellDimensions,
        /* cellsPerColumn */              1,
        /* activationThreshold */         args_.distalSegmentThreshold,
        /* initialPermanence */           args_.distalInitialPermanence,
        /* connectedPermanence */         args_.distalSynapseThreshold,
        /* minThreshold */                args_.distalSegmentMatch,
        /* maxNewSynapseCount */          args_.distalAddSynapses,
        /* permanenceIncrement */         args_.distalIncrement,
        /* permanenceDecrement */         args_.distalDecrement,
        /* predictedSegmentDecrement */   args_.distalMispredictDecrement,
        /* seed */                        rng_(),
        /* maxSegmentsPerCell */          args_.distalMaxSegments,
        /* maxSynapsesPerSegment */       args_.distalMaxSynapsesPerSegment,
        /* checkInputs */                 true,
        /* extra */                       SDR(args_.distalInputDimensions).size);

    iterationNum_      = 0u;
    iterationLearnNum_ = 0u;

    reset();

    if( PP_Sp.min() * proximalInputs.size < args_.proximalSegmentThreshold ) {
      NTA_WARN << "WARNING: Proximal segment has fewer synapses than the segment threshold." << endl;
    }
    if( PP_AF.min() == 0.0f ) {
      NTA_WARN << "WARNING: Proximal input is unused." << endl;
    }

    if( args_.verbose ) {
      // TODO: Print all parameters
      cout << "Potential Pool Statistics:" << endl
           << PP_Sp
           << PP_AF << endl;
    }
  }


  Real initProximalPermanence(Real connectedPct = 0.5f) {
    // TODO: connectedPct as a true parameter? Maybe? Someday?
    if( rng_.getReal64() <= connectedPct )
        return args_.proximalSynapseThreshold +
          (1.0f - args_.proximalSynapseThreshold) * rng_.getReal64();
      else
        return args_.proximalSynapseThreshold * rng_.getReal64();
  }


  void reset() {
    X_act.assign( proximalConnections.numCells(), 0.0f );
    X_inact.assign( proximalConnections.numCells(), 0.0f );
    distalConnections.reset();
    // Zero Previous Updates
    proximalUpdates.assign( proximalConnections.segmentFlatListLength(), {} );
  }


  void compute(
        SDR& proximalInputActive,
        SDR& distalInputActive,
        bool learn,
        SDR& active) {
  //   SDR none( args_.distalInputDimensions );
  //   SDR none2( active.dimensions );
  //   compute_(proximalInputActive, proximalInputActive, none, none, learn, active, none2 );
  // }

  // void compute_(
  //       const SDR& proximalInputActive,
  //       const SDR& proximalInputLearning,
  //       const SDR& distalInputActive,
  //       const SDR& distalInputLearning,
  //       bool learn,
  //       SDR& active,
  //       SDR& learning) {
    NTA_CHECK( proximalInputActive.dimensions   == args_.proximalInputDimensions );
    // NTA_CHECK( proximalInputLearning.dimensions == args_.proximalInputDimensions );
    // NTA_CHECK( distalInputActive.dimensions     == args_.distalInputDimensions );
    // NTA_CHECK( distalInputLearning.dimensions   == args_.distalInputDimensions );
    NTA_CHECK( active.dimensions                == cellDimensions );
    // NTA_CHECK( learning.dimensions              == cellDimensions );

    // Update bookkeeping
    iterationNum_++;
    if( learn )
      iterationLearnNum_++;

    // Feed Forward Input / Proximal Dendrites
    vector<Real> cellOverlaps( active.size );
    activateProximalDendrites( proximalInputActive, cellOverlaps );

    // distalConnections.activateDendrites(learn, distalInputActive, distalInputLearning);
    distalConnections.activateDendrites(learn, distalInputActive, distalInputActive);
    SDR predictedCells( cellDimensions );
    distalConnections.getPredictiveCells( predictedCells );

    activateCells( cellOverlaps, predictedCells, active );

    // Learn
    std::sort( active.getSparse().begin(), active.getSparse().end() );
    distalConnections.activateCells( active, learn );
    if( learn ) {
      // learnProximalDendrites( proximalInputActive, proximalInputLearning, active );
      learnProximalDendrites( proximalInputActive, proximalInputActive, active );
    }
  }


  void activateProximalDendrites( const SDR &feedForwardInputs,
                                  vector<Real> &cellOverlaps )
  {
    // Proximal Feed Forward Excitement
    rawOverlaps_.assign( proximalConnections.numSegments(), 0.0f );
    proximalConnections.computeActivity(rawOverlaps_, feedForwardInputs.getSparse());

    // Setup for Boosting
    const Real denominator = 1.0f / log2( args_.sparsity / args_.proximalSegments );
    const auto &af = AF_->activationFrequency;

    // Process Each Segment of Each Cell
    for(auto cell = 0u; cell < proximalConnections.numCells(); ++cell) {
      Real maxOverlap    = -1.0;
      UInt maxSegment    = -1;
      // TODO: THIS FOR LOOP SHOULD USE INDEX MATH!
      for(const auto &segment : proximalConnections.segmentsForCell( cell ) ) {
        const auto raw = rawOverlaps_[segment];

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
          // overlap = 1.0f;
          overlap = 0.0f;
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
      proximalMaxSegment_[cell] = maxSegment;

      // TODO: apply stability & fatigue before boosting?  My previous
      // experiments w/ grid cells apply stab+fatigue b4 boosting...

      // Apply Stability & Fatigue
      X_act[cell]   += (1.0f - args_.stability_rate) * (maxOverlap - X_act[cell] - X_inact[cell]);
      X_inact[cell] += args_.fatigue_rate * (maxOverlap - X_inact[cell]);
      cellOverlaps[cell] = X_act[cell];
    }
  }


  void applyProximalSegmentThreshold( vector<UInt> &cells, UInt threshold )
  {
    for( UInt idx = 0; idx < cells.size(); )
    {
      const auto &maxSeg  = proximalMaxSegment_[ cells[idx] ];
      const auto &segOvlp = rawOverlaps_[ maxSeg ];

      if( segOvlp < threshold ) {
        cells[idx] = cells.back();
        cells.pop_back();
      }
      else {
        ++idx;
      }
    }
  }


  void learnProximalDendrites( const SDR &proximalInputActive,
                               const SDR &proximalInputLearning,
                               const SDR &active ) {
    SDR AF_SDR( AF_->dimensions );
    auto &activeSegments = AF_SDR.getSparse();
    auto &sparse = active.getSparse();
    sort( sparse.begin(), sparse.end() );
    Int prevSegmentUpdate = -1;
    for(const auto &cell : sparse)
    {
      const auto &maxSegment = proximalMaxSegment_[cell];
      activeSegments.push_back(maxSegment);

      // Adapt Proximal Segments
      proximalConnections.adaptSegment(maxSegment, proximalInputActive,
                                       args_.proximalIncrement, args_.proximalDecrement,
                                       proximalUpdates[maxSegment]);

      // Zero the permanence changes for the inactive cells.
      for(UInt z = prevSegmentUpdate + 1; z < maxSegment; ++z) {
        proximalUpdates[z].clear();
      }
      NTA_CHECK( prevSegmentUpdate < (Int) maxSegment );
      prevSegmentUpdate = maxSegment;

      proximalConnections.raisePermanencesToThreshold(maxSegment,
                                   args_.proximalSynapseThreshold,
                                   args_.proximalSegmentThreshold);
    }
    // Zero the permanence changes for the last cells in the list.
    for(UInt z = prevSegmentUpdate + 1; z < proximalMaxSegment_.size(); ++z) {
      proximalUpdates[z].clear();
    }
    AF_SDR.setSparse( activeSegments );
    AF_->addData( AF_SDR );

    // TODO: Grow new synapses from the learning inputs?
  }


  void activateCells( const vector<Real> &overlaps,
                      const SDR          &predictiveCells,
                            SDR          &activeCells)
  {
    const UInt inhibitionAreas = activeCells.size / args_.cellsPerInhibitionArea;
    const UInt numDesired = (UInt) std::round(args_.sparsity * args_.cellsPerInhibitionArea);
    NTA_CHECK(numDesired > 0) << "Not enough cellsPerInhibitionArea ("
      << args_.cellsPerInhibitionArea << ") for desired density (" << args_.sparsity << ").";

    auto &allActive = activeCells.getSparse();
    allActive.clear();
    allActive.reserve(numDesired * inhibitionAreas );

    for(UInt offset = 0u; offset < activeCells.size; offset += args_.cellsPerInhibitionArea)
    {
      activateInhibitionArea(
          overlaps, predictiveCells,
          offset, offset + args_.cellsPerInhibitionArea,
          numDesired,
          allActive);
    }
    activeCells.setSparse( allActive );
  }


  void activateInhibitionArea(
      const vector<Real> &overlaps,
      const SDR          &predictiveCells,
            UInt          areaStart,
            UInt          areaEnd,
            UInt          numDesired,
            vector<UInt> &active)
  {
    // Inhibition areas are contiguous blocks of cells, find the size of it.
    const auto areaSize = areaEnd - areaStart;
    const auto &predictiveCellsDense = predictiveCells.getDense();
    // Compare the cell indexes by their overlap.
    auto compare = [&overlaps](const UInt &a, const UInt &b) -> bool
                    { return overlaps[a] > overlaps[b]; };
    // Make a sorted list of all of the cell indexes which are in this inhibition area.
    vector<UInt> cells;
    cells.resize( areaSize );
    std::iota( cells.begin(), cells.end(), areaStart );

    // PHASE ONE: Predicted cells compete to activate.

    // Select the predicted cells.
    vector<UInt> phase1Active;
    for( const auto &idx : cells ) {
      if( predictiveCellsDense[idx] ) {
        phase1Active.push_back( idx );
      }
    }
    if( !phase1Active.empty() ) {
      if( phase1Active.size() > numDesired ) {
        // Do a partial sort to divide the winners from the losers.  This sort is
        // faster than a regular sort because it stops after it partitions the
        // elements about the Nth element, with all elements on their correct side of
        // the Nth element.
        std::nth_element(
          phase1Active.begin(),
          phase1Active.begin() + numDesired,
          phase1Active.end(),
          compare);
        // Remove cells which lost the competition.
        phase1Active.resize( numDesired );
      }

      // Apply proximal segments activation threshold.
      applyProximalSegmentThreshold( phase1Active, args_.proximalSegmentThreshold );
    }

    // PHASE TWO: All remaining inactive cells compete to activate.

    const UInt phase2NumDesired = max((UInt) (numDesired - phase1Active.size()), (UInt)0u );

    // Sort the phase 1 active cell indexes for fast access.
    std::sort( phase1Active.begin(), phase1Active.end() );
    phase1Active.push_back( std::numeric_limits<UInt>::max() ); // Append a sentinel
    auto phase1ActiveIter = phase1Active.begin();
    // Select all cells which are still inactive.
    vector<UInt> phase2Active;
    phase2Active.reserve( areaSize );
    for( const auto &idx : cells ) {
      if( idx == *phase1ActiveIter ) {
        ++phase1ActiveIter;
      }
      else {
        phase2Active.push_back( idx );
      }
    }
    phase1Active.pop_back(); // Remove the sentinel
    // Do a partial sort to divide the winners from the losers.
    std::nth_element(
      phase2Active.begin(),
      phase2Active.begin() + phase2NumDesired,
      phase2Active.end(),
      compare);
    // Remove cells which lost the competition.
    phase2Active.resize( phase2NumDesired );

    // Apply activation threshold to proximal segments.
    applyProximalSegmentThreshold( phase2Active, args_.proximalSegmentThreshold );

    for( const auto &idx : phase1Active ) {
      active.push_back( idx );
    }
    for( const auto &idx : phase2Active ) {
      active.push_back( idx );
    }
  }

  void setParameters( Parameters &newParameters )
  {
    args_ = newParameters;

    if( newParameters.proximalInputDimensions     != parameters.proximalInputDimensions ) {
      NTA_THROW << "Setter unimplemented.";
    }
    if( newParameters.distalInputDimensions       != parameters.distalInputDimensions ) {
      NTA_THROW << "Setter unimplemented.";
    }
    if( newParameters.inhibitionDimensions        != parameters.inhibitionDimensions ) {
      NTA_THROW << "Setter unimplemented.";
    }
    if( newParameters.cellsPerInhibitionArea      != parameters.cellsPerInhibitionArea ) {
      NTA_THROW << "Setter unimplemented.";
    }
    if( newParameters.sparsity                    != parameters.sparsity ) {
      NTA_THROW << "Setter unimplemented.";
    }
    // if( newParameters.potentialPool               != parameters.potentialPool ) {
    //   NTA_THROW << "Setter unimplemented.";
    // }
    if( newParameters.proximalSegments            != parameters.proximalSegments ) {
      NTA_THROW << "Setter unimplemented.";
    }
    if( newParameters.proximalSegmentThreshold    != parameters.proximalSegmentThreshold ) {
      NTA_THROW << "Setter unimplemented.";
    }
    if( newParameters.proximalIncrement           != parameters.proximalIncrement ) {
      NTA_THROW << "Setter unimplemented.";
    }
    if( newParameters.proximalDecrement           != parameters.proximalDecrement ) {
      NTA_THROW << "Setter unimplemented.";
    }
    if( newParameters.proximalSynapseThreshold    != parameters.proximalSynapseThreshold ) {
      NTA_THROW << "Setter unimplemented.";
    }
    if( newParameters.distalMaxSegments           != parameters.distalMaxSegments ) {
      NTA_THROW << "Setter unimplemented.";
    }
    if( newParameters.distalMaxSynapsesPerSegment != parameters.distalMaxSynapsesPerSegment ) {
      NTA_THROW << "Setter unimplemented.";
    }
    if( newParameters.distalSegmentThreshold      != parameters.distalSegmentThreshold ) {
      NTA_THROW << "Setter unimplemented.";
    }
    if( newParameters.distalSegmentMatch          != parameters.distalSegmentMatch ) {
      NTA_THROW << "Setter unimplemented.";
    }
    if( newParameters.distalAddSynapses           != parameters.distalAddSynapses ) {
      NTA_THROW << "Setter unimplemented.";
    }
    if( newParameters.distalIncrement             != parameters.distalIncrement ) {
      NTA_THROW << "Setter unimplemented.";
    }
    if( newParameters.distalDecrement             != parameters.distalDecrement ) {
      NTA_THROW << "Setter unimplemented.";
    }
    if( newParameters.distalMispredictDecrement   != parameters.distalMispredictDecrement ) {
      NTA_THROW << "Setter unimplemented.";
    }
    if( newParameters.distalSynapseThreshold      != parameters.distalSynapseThreshold ) {
      NTA_THROW << "Setter unimplemented.";
    }
    if( newParameters.stability_rate              != parameters.stability_rate ) {
      NTA_THROW << "Setter unimplemented.";
    }
    if( newParameters.fatigue_rate                != parameters.fatigue_rate ) {
      NTA_THROW << "Setter unimplemented.";
    }
    if( newParameters.period                      != parameters.period ) {
      NTA_THROW << "Setter unimplemented.";
    }
    if( newParameters.seed                        != parameters.seed ) {
      NTA_THROW << "Setter unimplemented.";
    }
    if( newParameters.verbose                     != parameters.verbose ) {
      NTA_THROW << "Setter unimplemented.";
    }
  }
};


Topology_t  DefaultTopology(
    Real potentialPct,
    Real potentialRadius,
    bool wrapAround)
  {
  return [=] (const SDR& cell, const vector<UInt>& potentialPoolDimensions, Random &rng) -> SDR {
    // Uniform topology over trailing input dimensions.
    auto inputTopology = potentialPoolDimensions;
    UInt extraDimensions = 1u;
    while( inputTopology.size() > cell.dimensions.size() ) {
      extraDimensions *= inputTopology.back();
      inputTopology.pop_back();
    }

    // Convert the coordinates of the target cell, from a location in
    // cellDimensions to inputTopology.
    NTA_ASSERT( cell.getSum() == 1u );
    vector<vector<UInt>> inputCoords;
    for(auto i = 0u; i < cell.dimensions.size(); i++)
    {
      const Real columnCoord = cell.getCoordinates()[i][0];
      const Real inputCoord = (columnCoord + 0.5f) *
                              (inputTopology[i] / (Real)cell.dimensions[i]);
      inputCoords.push_back({ (UInt32)floor(inputCoord) });
    }
    SDR inputTopologySDR( inputTopology );
    inputTopologySDR.setCoordinates( inputCoords );
    const auto centerInput = inputTopologySDR.getSparse()[0];

    vector<UInt> columnInputs;
    if( wrapAround ) {
      for( UInt input : WrappingNeighborhood(centerInput, potentialRadius, inputTopology)) {
        for( UInt extra = 0; extra < extraDimensions; ++extra ) {
          columnInputs.push_back( input * extraDimensions + extra );
        }
      }
    }
    else {
      for( UInt input :
           Neighborhood(centerInput, potentialRadius, inputTopology)) {
        for( UInt extra = 0; extra < extraDimensions; ++extra ) {
          columnInputs.push_back( input * extraDimensions + extra );
        }
      }
    }

    const UInt numPotential = (UInt)round(columnInputs.size() * potentialPct);
    const auto selectedInputs = rng.sample<UInt>(columnInputs, numPotential);
    SDR potentialPool( potentialPoolDimensions );
    potentialPool.setSparse( selectedInputs );
    return potentialPool;
  };
}

// TODO: Test this!
// TODO: Document this because this is the one users should copy-paste to make their own topology.
Topology_t NoTopology(Real potentialPct)
{
  return [=](const SDR& cell, const vector<UInt>& potentialPoolDimensions, Random &rng) -> SDR {
    SDR potentialPool( potentialPoolDimensions );
    potentialPool.randomize( potentialPct, rng );
    return potentialPool;
  };
}

// TODO: USE THIS!
InitialPermanence_t DefaultInitialPermanence(Permanence connectedThreshold) {
  return [=](Random &rng) -> Permanence {
    return rng.getReal64(); // placeholder
  };
}


} // End namespace column_pooler
} // End namespace algorithmn
} // End namespace nupic
