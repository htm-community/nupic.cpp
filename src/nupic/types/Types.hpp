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

/** @file
 * Basic C++ type definitions used throughout `nupic.core` and rely on `Types.h`
 */

#ifndef NTA_TYPES_HPP
#define NTA_TYPES_HPP

#include <nupic/types/Types.h>

//----------------------------------------------------------------------

namespace nupic
{
  /**
   * @name Basic types
   *
   * @{
   */

  /**
   * Represents a 8-bit byte.
   */
  typedef char            Byte;

  /**
   * Represents a 16-bit signed/unsigned integer.
   */
  typedef short           Int16;
  typedef unsigned short  UInt16;

  /**
   * Represents a 32-bit signed/unsigned integer.
   */
#if defined(NTA_OS_WINDOWS) && defined(NTA_ARCH_32)
  // (LP32 data models)
  typedef long Int32;
  typedef unsigned long UInt32;
#else  
   // (LLP64 data models)
  typedef int Int32;
  typedef unsigned int UInt32;
#endif

  /**
   * Represents a 64-bit signed/unsigned integer.
   */
  typedef long long           Int64;
  typedef unsigned long long  UInt64;

  /**
   * Represents a 32-bit real number(a floating-point number).
   */

  typedef float          Real32;

  /**
   * Represents a 64-bit real number(a floating-point number).
   */
  typedef double          Real64;

  /**
   * Represents an opaque handle/pointer, same as `void *`
   */
  typedef void *       Handle;

  /**
  * Represents an opaque pointer, 
  */
  typedef uintptr_t         UIntPtr;


#ifdef NTA_DOUBLE_PRECISION
  /**
   * Real - Represents a real number(a floating-point number).
   * Same as Real64 if `NTA_DOUBLE_PRECISION` is defined, else Real32
   */
  typedef Real64 Real;
#define NTA_REAL_TYPE_STRING "Real64"
#else
  typedef Real32 Real;
#define NTA_REAL_TYPE_STRING "Real32"
#endif

#ifdef NTA_BIG_INTEGER
  /**
   * UInt, Int Represents a signed/Unsigned integer.
   * Same as Int64 if `NTA_BIG_INTEGER` is defined, Int32 otherwise.
   */
  typedef Int64 Int;
  typedef UInt64 UInt;
#else
  typedef Int32 Int;
  typedef UInt32 UInt;
#endif
  

  /**
   * Represents lengths of arrays, strings and so on.
   */
  typedef size_t Size;

  /**
   * @}
   */

  /** 
   * This enum represents the documented logging level of the debug logger. 
   * 
   * Use it like `LDEBUG(nupic::LogLevel_XXX)`.
   */
  enum LogLevel
  {
    /**
     * Log level: None.
     */
    LogLevel_None = NTA_LogLevel_None,
    /**
     * Log level: Minimal.
     */
    LogLevel_Minimal,
    /**
     * Log level: Normal.
     */
    LogLevel_Normal,
    /**
     * Log level: Verbose.
     */
    LogLevel_Verbose,
  };

} // end namespace nupic

#endif // NTA_TYPES_HPP



