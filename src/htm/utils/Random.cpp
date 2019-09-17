/* ---------------------------------------------------------------------
 * HTM Community Edition of NuPIC
 * Copyright (C) 2013, Numenta, Inc.
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
    Random Number Generator implementation
*/
#include <iostream> // for istream, ostream
#include <chrono>   // for random seeds

#include <htm/utils/Log.hpp>
#include <htm/utils/Random.hpp>

using namespace htm;

bool Random::operator==(const Random &o) const {
  return seed_ == o.seed_ && \
	 steps_ == o.steps_ && \
	 gen == o.gen;
}

bool static_gen_seeded = false;  //used only for seeding seed if 0/auto is passed for seed
std::mt19937 static_gen;

Random::Random(UInt64 seed) {
  if (seed == 0) {
    if( !static_gen_seeded ) {
      #if NDEBUG
        unsigned int static_seed = (unsigned int)std::chrono::system_clock::now().time_since_epoch().count();
      #else
        unsigned int static_seed = DEBUG_RANDOM_SEED;
      #endif
      static_gen.seed( static_seed );
      static_gen_seeded = true;
      NTA_INFO << "Random seed: " << static_seed;
    }
    seed_ = static_gen(); //generate random value from HW RNG
  } else {
    seed_ = seed;
  }
  // if seed is zero at this point, there is a logic error.
  NTA_CHECK(seed_ != 0);
  gen.seed(static_cast<unsigned int>(seed_)); //seed the generator
  steps_ = 0;
}

namespace htm {
// helper function for seeding RNGs across the plugin barrier
UInt32 GetRandomSeed() {
  return htm::Random().getUInt32();
}
} // namespace htm
