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
  * This is part of the Cpp/CLI code that forms the API interface for C#.
  * compile with: /clr
  * compile with: /EHa 
  */
  
  // NOTE:  in C++/CLI all implementation resides in .hpp files.
  //        Use #includes <xxx.hpp>  for macros and to resolve references to native C++ or C classes/routines
  //        Use #using xxx.obj       to resolve references to other C++/CLI modules
  //        The .cpp files are optional but are placeholders for the Visual Studio Build system.

#include <cs_Types.hpp>
#include <cs_Utils.hpp>
#include <cs_Engine.hpp>
