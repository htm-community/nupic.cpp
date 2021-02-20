/* ---------------------------------------------------------------------
 * HTM Community Edition of NuPIC
 * Copyright (C) 2016, Numenta, Inc.
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
 * ---------------------------------------------------------------------- */

/** @file
 * Topology helpers
 */

#include <htm/utils/Topology.hpp>
#include <htm/utils/Log.hpp>
#include <algorithm> // sort

using namespace htm;
using namespace std;

namespace htm {


Topology_t  DefaultTopology(
    Real potentialPct,
    Real potentialRadius,
    bool wrapAround)
{
  NTA_CHECK( potentialPct >= 0.0f );
  NTA_CHECK( potentialPct <= 1.0f );
  NTA_CHECK( potentialRadius >= 0.0f );
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
      const UInt32 columnCoord = cell.getCoordinates()[i][0];
      const Real inputCoord = (static_cast<Real>(columnCoord) + 0.5f) *
                              (inputTopology[i] / (Real)cell.dimensions[i]);
      inputCoords.push_back({ (UInt32)floor(inputCoord) });
    }
    SDR inputTopologySDR( inputTopology );
    inputTopologySDR.setCoordinates( inputCoords );
    const auto centerInput = inputTopologySDR.getSparse()[0];

    vector<UInt> columnInputs;
    for( UInt input : Neighborhood(centerInput, (UInt)floor(potentialRadius), inputTopology, wrapAround /*wrapping*/)) {
        for( UInt extra = 0; extra < extraDimensions; ++extra ) {
          columnInputs.push_back( input * extraDimensions + extra );
        }
    }
 

    const UInt numPotential = (UInt)round(columnInputs.size() * potentialPct);
    auto selectedInputs = rng.sample<UInt>(columnInputs, numPotential);
    std::sort( selectedInputs.begin(), selectedInputs.end() );
    SDR potentialPool( potentialPoolDimensions );
    potentialPool.setSparse( selectedInputs );
    return potentialPool;
  };
}


Topology_t NoTopology(Real potentialPct)
{
  NTA_CHECK( potentialPct >= 0.0f );
  NTA_CHECK( potentialPct <= 1.0f );
  return [=](const SDR& cell, const vector<UInt>& potentialPoolDimensions, Random &rng) -> SDR {
    SDR potentialPool( potentialPoolDimensions );
    potentialPool.randomize( potentialPct, rng );
    return potentialPool;
  };
}


vector<UInt> coordinatesFromIndex(UInt index, const vector<UInt> &dimensions) {
  vector<UInt> coordinates(dimensions.size(), 0);

  UInt shifted = index;
  for (size_t i = dimensions.size() - 1; i > 0; i--) {
    coordinates[i] = shifted % dimensions[i];
    shifted = shifted / dimensions[i];
  }

  NTA_ASSERT(shifted < dimensions[0]);
  coordinates[0] = shifted;

  return coordinates;
}

UInt indexFromCoordinates(const vector<UInt> &coordinates,
                          const vector<UInt> &dimensions) {
  NTA_ASSERT(coordinates.size() == dimensions.size());

  UInt index = 0;
  for (size_t i = 0; i < dimensions.size(); i++) {
    NTA_ASSERT(coordinates[i] < dimensions[i]);
    index *= dimensions[i];
    index += coordinates[i];
  }

  return index;
}

} // end namespace htm


// ============================================================================
// NEIGHBORHOOD
// ============================================================================

Neighborhood::Neighborhood(const UInt centerIndex, 
                           const UInt radius,
                           const vector<UInt> &dimensions, 
                           const bool wrap,
                           const bool skipCenter)
    : centerPosition_(coordinatesFromIndex(centerIndex, dimensions)),
      dimensions_(dimensions), radius_(radius), wrap_(wrap), skipCenter_(skipCenter), center_(centerIndex) {
        if(wrap == false) {
          NTA_WARN << "Neighborhood uses wrap=false which runs considerably slower with local inhibition!";
        }
      }

Neighborhood::Iterator::Iterator(const Neighborhood &neighborhood, bool end)
    : neighborhood_(neighborhood),
      offset_(neighborhood.dimensions_.size(), -(Int)neighborhood.radius_),
      finished_(end) {
        if(!neighborhood.wrap_) {
          // Choose the first offset that has positive resulting coordinates.
          for (size_t i = 0; i < offset_.size(); i++) {
            offset_[i] = std::max(offset_[i], -(Int)neighborhood_.centerPosition_[i]);
          }
        }
}

bool Neighborhood::Iterator::operator!=(const Iterator &other) const {
  return finished_ != other.finished_;
}

UInt Neighborhood::Iterator::operator*() {
  UInt index = 0;
  
  NTA_ASSERT(neighborhood_.dimensions_.size() == offset_.size() and offset_.size() == neighborhood_.centerPosition_.size()); 
  for (size_t i = 0; i < neighborhood_.dimensions_.size(); i++) {
    Int coordinate = neighborhood_.centerPosition_[i] + offset_[i];

    if(neighborhood_.wrap_) {
      // With a large radius, it may have wrapped around multiple times, so use
      // `while`, not `if`.
      while (coordinate < 0) { //lower bound
        coordinate += neighborhood_.dimensions_[i];
      }
      while (coordinate >= static_cast<Int>(neighborhood_.dimensions_[i])) { //upper b
        coordinate -= neighborhood_.dimensions_[i];
      }
    }

    NTA_ASSERT(coordinate >= 0);
    NTA_ASSERT(coordinate < (Int)neighborhood_.dimensions_[i]);

    index *= neighborhood_.dimensions_[i];
    index += coordinate;
  }

  if(neighborhood_.skipCenter_ and index == neighborhood_.center_) { 
    advance_();
    return this->operator*();
  }

  return index;
}

const Neighborhood::Iterator &Neighborhood::Iterator::operator++() {
  advance_();
  return *this;
}

void Neighborhood::Iterator::advance_() {
  // When it overflows, we need to "carry the 1" to the next dimension.
  bool overflowed = true;

  for (Int i = static_cast<Int>(offset_.size()) - 1; i >= 0; i--) {
    offset_[i]++;

    if(!neighborhood_.wrap_) {
      overflowed = offset_[i] > (Int)neighborhood_.radius_ ||
                 (((Int)neighborhood_.centerPosition_[i] + offset_[i]) >= (Int)neighborhood_.dimensions_[i]);
    } else {
    // If the offset has moved by more than the dimension size, i.e. if
    // offset_[i] - (-radius) is greater than the dimension size, then we're
    // about to run into points that we've already seen. This happens when given
    // small dimensions, a large radius, and wrap-around.
    overflowed = offset_[i] > (Int)neighborhood_.radius_ ||
                 offset_[i] + (Int)neighborhood_.radius_ >= (Int)neighborhood_.dimensions_[i];
    }

    if (overflowed) {
      if(!neighborhood_.wrap_) {
        // Choose the first offset that has a positive resulting coordinate.
        offset_[i] = std::max(-(Int)neighborhood_.radius_, -(Int)neighborhood_.centerPosition_[i]);
      } else {
        offset_[i] = -(Int)neighborhood_.radius_;
      }
    } else {
      // There's no overflow. The remaining coordinates don't need to change.
      break;
    }
  }

  // When the final coordinate overflows, we're done.
  if (overflowed) {
    finished_ = true;
  }
}

unordered_map<CellIdx, vector<CellIdx>> Neighborhood::updateAllNeighbors(
		const UInt radius,
                const vector<UInt> dimensions,
                const bool wrapAround,
                const bool skip_center) { //TODO  move the cache logic to Neighbor class

  std::unordered_map<CellIdx, vector<CellIdx>> neighborMap;
  UInt numColumns = 1;
  for(const auto dim: dimensions) {
    numColumns*= dim;
  }
  neighborMap.reserve(numColumns);


  for(UInt column=0; column < numColumns; column++) {
    vector<CellIdx> neighbors; //of the current column
    for(const auto neighbor: Neighborhood(column, radius, dimensions, wrapAround, skip_center)) {
      neighbors.push_back(neighbor);
    }
    std::sort(neighbors.begin(), neighbors.end()); //sort for better cache locality
    neighbors.shrink_to_fit();

    neighborMap[column] = neighbors;
  }
  return neighborMap;
}


Neighborhood::Iterator Neighborhood::begin() const { return {*this, false}; }
Neighborhood::Iterator Neighborhood::end() const { return {*this, true}; }
