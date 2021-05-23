#     nupic/csharp/test/CMakeLists.txt
# -----------------------------------------------------------------------------
# Numenta Platform for Intelligent Computing (NuPIC)
# Copyright (C) 2018, Numenta, Inc.  Unless you have purchased from
# Numenta, Inc. a separate commercial license for this software code, the
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
# Author:  David Keeney, 2018 
# -----------------------------------------------------------------------------
#  taken from example: https://github.com/awakecoding/CMakeSharp  (uses Mono C# compiler)

cmake_minimum_required(VERSION 3.8)
set(CMAKE_VERBOSE_MAKEFILE ON) # toggle for cmake debugging

set(MODULE_NAME "NupicCoreTest")

set(moduleSources
	NupicCoreTest.cs)

add_executable(${MODULE_NAME} ${moduleSources})
target_link_libraries(${MODULE_NAME} ${LIB_TARGET})
set_target_properties(${MODULE_NAME} PROPERTIES LINKER_LANGUAGE CSharp)
target_compile_options(${MODULE_NAME} PRIVATE "/langversion:6")
add_dependencies( ${MODULE_NAME}
                  ${LIB_TARGET})


