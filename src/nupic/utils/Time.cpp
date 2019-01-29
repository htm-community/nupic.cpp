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
 * --------------------------------------------------------------------- */

#include <time.h>
#include <algorithm> //max
#include <cmath> //for sins
#include <sstream>
#include <vector>
#include <nupic/utils/Random.hpp>
#include <nupic/utils/Log.hpp>

namespace nupic {

float getElapsed(clock_t t)
  { return (float)(clock() - t) / CLOCKS_PER_SEC; }

float SPEED = -1;

/**
 * Estimate speed (CPU & load) of the current system.
 * Tests must perform relative to this value
 */
float getSpeed() {
  if (SPEED == -1) {

    // This code just wastes CPU time to estimate speed.
    clock_t timer = clock();

    Random rng(42);
    // Make 10 million 4-byte Reals.  Each synapse take approx 30-40 bytes to
    // represent, so this is enough memory for 1 million synapses.
    std::vector<Real> data(10000000);
    for( Size i = 0; i < data.size(); i++ ) {
      data[i] = (Real)rng.getUInt32(80085);
      auto t  = data[i];
      data[i] = data[data.size()-i-1];
      data[data.size()-i-1] = t;
    }
    // Hurt the cache with random accesses.
    rng.shuffle(begin(data), end(data));
    // Test floating-point arithmatic.
    std::vector<Real> sins;
    for (auto d : data) {
      sins.push_back( sin(d) / cos(d) );
    }
    data = rng.sample<Real>(sins, 666);
    NTA_CHECK(data.size() == 666);
    SPEED = std::max(1.0f, (float)(clock() - timer) / CLOCKS_PER_SEC);
    NTA_INFO << "Time::getSpeed() -> " << SPEED << " seconds.";
  }
  return SPEED;
}

} // namespace nupic
