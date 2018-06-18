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

/** 
 * @file
 * 
 * Basic C type definitions used throughout `nupic.core` . 
 * 
 * It is included by `Types.hpp` - the C++ basic types file
 */

#ifndef NTA_TYPES_H
#define NTA_TYPES_H

#include <stddef.h>
#include <stdint.h>

#if defined(NTA_OS_WINDOWS) && defined(NTA_COMPILER_MSVC) && defined(NDEBUG)
#pragma warning( disable : 4244 ) // conversion from 'double' to 'nta::Real', possible loss of data (LOTS of various type combinations)
#pragma warning( disable : 4251 ) // needs to have dll-interface to be used by clients of class 
#pragma warning( disable : 4275 ) // non dll-interface struct used as base for dll-interface class
#pragma warning( disable : 4305 ) // truncation from 'double' to 'nta::Real', possible loss of data (LOTS of various type combinations)
#pragma warning( once : 4838 ) // narrowing conversions
#endif


/*---------------------------------------------------------------------- */
//// determine the bittness that this was compiled for.   (determines size of int, etc.)
#if defined(NTA_OS_WINDOWS) && !defined(_WIN64)
#define NTA_ARCH_32
#endif
/** 
 * Basic types enumeration
 */
typedef enum NTA_BasicType
  {
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

#ifdef NTA_DOUBLE_PRECISION 
    /** TODO: document */
    NTA_BasicType_Real = NTA_BasicType_Real64,
#else 
    /** TODO: document */
    NTA_BasicType_Real = NTA_BasicType_Real32,
#endif

  } NTA_BasicType;


/**
 * @}
 */

/** 
 * This enum represents the documented logging level of the debug logger. 
 * 
 * Use it like `LDEBUG(NTA_LogLevel_XXX)`.
 */
typedef enum NTA_LogLevel
  {
    /**
     * Log level: None.
     */
    NTA_LogLevel_None,
    /**
     * Log level: Minimal.
     */
    NTA_LogLevel_Minimal,
    /**
     * Log level: Normal.
     */
    NTA_LogLevel_Normal,
    /**
     * Log level: Verbose.
     */
    NTA_LogLevel_Verbose,
  } NTA_LogLevel;

#endif /* NTA_TYPES_H */
