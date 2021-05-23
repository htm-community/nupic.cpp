#     nupic/csharp/CMakeLists.txt
# -----------------------------------------------------------------------------
# Numenta Platform for Intelligent Computing (NuPIC)
# Copyright (C) 2019, Numenta, Inc.  Unless you have purchased from
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
# Author:  David Keeney, 2019 
# -----------------------------------------------------------------------------
#

cmake_minimum_required(VERSION 2.8.9)
set(CMAKE_VERBOSE_MAKEFILE ON) # toggle for cmake debugging



SET(LIB_TARGET  htm_core_cs)
message(STATUS "CSHARP_INTERFACE: ${LIB_TARGET}    ${PROJECT_SOURCE_DIR}")

##############################################################
#       Assumptions
##############################################################
#  assume include folders were set by parent CMakeLists.txt.  Should point to src/htm
##include_directories(${PROJECT_SOURCE_DIR})
GET_DIRECTORY_PROPERTY(output INCLUDE_DIRECTORIES)
message(STATUS "Include directories: ${output}")

#  assume the output folders were set by parent CMakeLists.txt
##SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
##SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
##SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

#  assume ${htm_core} points to the static htm.core libary.


##############################################################
#       Build the C# interface library  into ${LIB_TARGET}
##############################################################
# build wrappers around htm_core to create build htm_core_cs.dll
add_subdirectory(bindings) 


##############################################################
#       C# Project for testing the interface
##############################################################
enable_language(CSharp)
add_subdirectory(test)    

