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

#ifndef NTA_TIME_HPP
#define NTA_TIME_HPP

#include <time.h>

namespace nupic {

/**
 * Returns the number of seconds which have passed since an initial timestamp
 * was taken.  Argument is a timestamp from the clock() function from <time.h>.
 *
 * Example Usage:
 *    clock_t start_time = clock();
 *    do_stuff();
 *    float elapsed_seconds = getElapsed( start_time );
 */
float getElapsed(clock_t start_time);

/**
 * Estimate speed (CPU & load) of the current system.
 * Tests must perform relative to this value
 */
float getSpeed();

} // namespace nupic
#endif // NTA_TIME_HPP
