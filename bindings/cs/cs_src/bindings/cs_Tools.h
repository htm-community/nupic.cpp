/* ----------------------------------------------------------------------
# Numenta Platform for Intelligent Computing (NuPIC)
# Copyright (C) 2018, Numenta, Inc.  Unless you have an agreement
# with Numenta, Inc., for a separate license for this software code, the
# following terms and conditions apply:
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero Public License version 3 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU Affero Public License for more details.
#
# You should have received a copy of the GNU Affero Public License
# along with this program.  If not, see http://www.gnu.org/licenses.
#
# http://numenta.org/licenses/
#
# Author: David Keeney, Jan 2018
# ----------------------------------------------------------------------*/
/** @file
* This file contains some macros useful for the conversion.
*
* This is part of the Cpp/CLI code that forms the API interface for C#.
* compile with: /clr
* compile with: /EHa
*/

#ifndef NTA_CS_TOOLS_H
#define NTA_CS_TOOLS_H
#pragma once

#include <vcclr.h>  // for gcroot() and PtrToStringChars()
#include <eh.h>      // for exception handling
#include <string>

#pragma warning (default : 4692)   // tell me when I don't have public access to a class or native type



//////////////////////////////////////////////////////////////////
////               Macros                                     ////
//////////////////////////////////////////////////////////////////

// To throw an exception and make sure the exception message is logged appropriately 
#define CS_THROW(msg) throw gcnew htm_core_cs::cs_LoggingException( __FILE__, __LINE__, msg)

#define CS_CHECK(condition, msg) if (condition)  {} \
else CS_THROW( (std::string("CHECK FAILED: \"" #condition "\" ")+msg).cstr())

// capture native C++ exceptions and re-throw CLI exceptions
#define CHKEXP(statement) \
    try {                 \
      statement           \
    }catch(htm::Exception* ex) { \
      throw gcnew htm_core_cs::cs_LoggingException(__FILE__,__LINE__, ex->getMessage()); \
    }                                      \
    catch (std::exception* ex) {           \
        throw gcnew htm_core_cs::cs_LoggingException(__FILE__,__LINE__, ex->what()); \
    }                                      \
    catch (...) {                           \
        throw gcnew htm_core_cs::cs_LoggingException(__FILE__,__LINE__, "Unknown Exception"); \
    }



#endif  // NTA_CS_TOOLS_H