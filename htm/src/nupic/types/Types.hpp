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
 * Basic C++ type definitions used throughout `nupic.core`.
 */

#ifndef NTA_TYPES_HPP
#define NTA_TYPES_HPP




#include <stddef.h>
#include <stdint.h>

#if defined(NTA_OS_WINDOWS) && defined(NTA_COMPILER_MSVC) && defined(NDEBUG)
#pragma warning(                                                               \
    disable : 4244) // conversion from 'double' to 'nta::Real', possible loss of
                    // data (LOTS of various type combinations)
#pragma warning(disable : 4251) // needs to have dll-interface to be used by
                                // clients of class
#pragma warning(disable : 4275) // non dll-interface struct used as base for
                                // dll-interface class
#pragma warning(                                                               \
    disable : 4305) // truncation from 'double' to 'nta::Real', possible loss of
                    // data (LOTS of various type combinations)
#pragma warning(once : 4838) // narrowing conversions
#endif

#define UNUSED(x) (void)(x)  // use this to avoid 'unused variable' compile errors.
#define NTA_INTERNAL 1       // just means not part of API even if exposed as public.


#if defined(NTA_OS_WINDOWS)
#define NTA_EXPORT __declspec(dllexport)
#define NTA_HIDDEN
#else
#define NTA_EXPORT __attribute__((visibility("default")))
#define NTA_HIDDEN __attribute__((visibility("hidden")))
#endif



//----------------------------------------------------------------------

namespace nupic {

///////////////////////// Basic types ///////////////////////////////////////

/**
 * @name Basic types
 *
 * @{
 */

/**
 * Represents a 8-bit byte.
 */
typedef char Byte;

/**
 * Represents a 16-bit signed integer.
 */
typedef short Int16;

/**
 * Represents a 16-bit unsigned integer.
 */
typedef unsigned short UInt16;

/**
 * Represents a 32-bit signed integer.
 */
typedef long Int32;

/**
 * Represents a 32-bit unsigned integer.
 */
typedef unsigned long UInt32;

/**
 * Represents a 64-bit signed integer.
 */
typedef long long Int64;

/**
 * Represents a 64-bit unsigned integer.
 */
typedef unsigned long long UInt64;

/**
 * Represents a 32-bit real number(a floating-point number).
 */
typedef float Real32;

/**
 * Represents a 64-bit real number(a floating-point number).
 */
typedef double Real64;

/**
 * Represents an opaque handle/pointer, same as `void *`
 */
typedef void* Handle;

/**
 * Represents an opaque pointer, same as `uintptr_t`
 */
typedef uintptr_t UIntPtr;

/**
 * Represents lengths of arrays, strings and so on.
 */
typedef size_t Size;



/**
 * @}
 */

/**
 * @name Flexible types
 *
 * The following are flexible types depending on `NTA_DOUBLE_PRECISION` and
 * `NTA_BIG_INTEGER`.
 *
 * @{
 *
 */

/**
 * Represents a real number(a floating-point number).
 *
 * Same as nupic::Real64 if `NTA_DOUBLE_PRECISION` is defined, nupic::Real32 otherwise.
 */
#ifdef NTA_DOUBLE_PRECISION
	typedef Real64 Real;
#else
	typedef Real32 Real;
#endif
/**
 * Represents a signed integer.
 *
 * Same as nupic::Int64 if `NTA_BIG_INTEGER` is defined, nupic::Int32 otherwise.
 */
#ifdef NTA_BIG_INTEGER
	typedef Int64 Int;
#else
	typedef Int32 Int;
#endif

/**
 * Represents a unsigned integer.
 *
 * Same as nupic::UInt64 if `NTA_BIG_INTEGER` is defined, nupic::UInt32
 * otherwise.
 */
#ifdef NTA_BIG_INTEGER
	typedef Uint64 UInt;
#else
	typedef UInt32 UInt;
#endif



///////////////////////// Basic type enums ///////////////////////////////////////

/**
 * Basic types enumeration
 */
typedef enum NTA_BasicType {
  /**
   * Represents a 8-bit byte.
   */
  NTA_BasicType_Byte,

  /**
   * Represents a 16-bit signed integer.
   */
  NTA_BasicType_Int16,

  /**
   * Represents a 16-bit unsigned integer.
   */
  NTA_BasicType_UInt16,

  /**
   * Represents a 32-bit signed integer.
   */
  NTA_BasicType_Int32,

  /**
   * Represents a 32-bit unsigned integer.
   */
  NTA_BasicType_UInt32,

  /**
   * Represents a 64-bit signed integer.
   */
  NTA_BasicType_Int64,

  /**
   * Represents a 64-bit unsigned integer.
   */
  NTA_BasicType_UInt64,

  /**
   * Represents a 32-bit real number(a floating-point number).
   */
  NTA_BasicType_Real32,

  /**
   * Represents a 64-bit real number(a floating-point number).
   */
  NTA_BasicType_Real64,

  /**
   * Represents a opaque handle/pointer, same as `void *`
   */
  NTA_BasicType_Handle,

  /**
   * Represents a boolean. The size is compiler-defined.
   *
   * There is no typedef'd "Bool" or "NTA_Bool". We just need a way to refer
   * to bools with a NTA_BasicType.
   */
  NTA_BasicType_Bool,

  /**
   * @note This is not an actual type, just a marker for validation purposes
   */
  NTA_BasicType_Last,

  /**
   * Represents a default-sized unsigned integer.
   */
#ifdef NTA_BIG_INTEGER
  NTA_BasicType_UInt = NTA_BasicType_UInt64,
#else
  NTA_BasicType_UInt = NTA_BasicType_UInt32,
#endif

  /**
   * Represents a default-sized real number(a floating-point number).
   */
#ifdef NTA_DOUBLE_PRECISION
  NTA_BasicType_Real = NTA_BasicType_Real64,
#else
  NTA_BasicType_Real = NTA_BasicType_Real32,
#endif

} NTA_BasicType;

///////////////////////// LogLevel ///////////////////////////////////////

/**
 * @}
 */

/**
 * This enum represents the documented logging level of the debug logger.
 *
 * Use it like `LDEBUG(nupic::LogLevel_XXX)`.
 */
enum LogLevel {
  /**
   * Log level: None.
   */
  LogLevel_None = 0,
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
