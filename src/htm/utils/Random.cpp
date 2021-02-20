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
#include <htm/utils/Log.hpp>
#include <htm/utils/Random.hpp>

using namespace htm;

bool Random::operator==(const Random &o) const {
  return seed_ == o.seed_ && \
	 steps_ == o.steps_ && \
	 gen == o.gen;
}

std::random_device rd; //HW RNG, undeterministic, platform dependant. Use only for seeding rng if random seed wanted (seed=0)

Random::Random(const UInt64 seed) {
  if (seed == 0) {
    std::mt19937 static_gen(rd());
    seed_ = static_gen(); //generate random value from HW RNG
  } else {
    seed_ = seed;
  }
  NTA_CHECK(seed_ != 0) << "Random: if seed is zero at this point, there is a logic error";
  gen.seed(static_cast<unsigned int>(seed_)); //seed the generator
  steps_ = 0;
}

namespace htm {
// helper function for seeding RNGs across the plugin barrier
UInt32 GetRandomSeed(const UInt seed) {
  return htm::Random(seed).getUInt32();
}
} // namespace htm
