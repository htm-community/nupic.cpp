# -----------------------------------------------------------------------------
# Numenta Platform for Intelligent Computing (NuPIC)
# Copyright (C) 2015-2018, Numenta, Inc.  Unless you have purchased from
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
# -----------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.3)
project(nupic_core CXX)

message(STATUS "----- SuperBuild for nupic_core -----")

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}")

set(CMAKE_VERBOSE_MAKEFILE OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

get_filename_component(REPOSITORY_DIR ${PROJECT_SOURCE_DIR} ABSOLUTE)

if( POLICY CMP0046 )
  cmake_policy(VERSION 3.3)
  # The new policy for add_dependencies is to now error if a dependency target
  # is not found. The old policy didn't care and continued through configuration
  cmake_policy(SET CMP0046 OLD)
endif()

if( POLICY CMP0054 )
  # The OLD behavior for this policy is to dereference variables and interpret
  # keywords even if they are quoted or bracketed. The NEW behavior is to not
  # dereference variables or interpret keywords that have been quoted or bracketed.
  cmake_policy(SET CMP0054 OLD)
endif()

##############################################
# Compiler support for <filesystem> in C++17:
# https://en.cppreference.com/w/cpp/compiler_support
#    GCC 7.1 has <experimental/filesystem>, link with -libc++experimental or -lstdc++fs
#    GCC 8 has <filesystem>   link with -lstdc++fs
#    GCC 9   expected to support <filesystem>
#    Clang 4 (XCode10) has no support for <filesystem>, partial C++17
#    Clang 7 has complete <filesystem> support for C++17
#    Visual Studio 2017 15.7 (v19.14)supports <filesystem> with C++17
#    MinGW has no support for filesystem.
#
# If we have support for <filesystem> and C++17, turn on the C++17 standard flag, 
# else set standard to C++11 and install boost filesystem
# Also allow external library for <filesystem>

set(extra_lib_for_filesystem)   # sometimes -libc++experimental or -lstdc++fs
set(INTERNAL_CPP_STANDARD "c++11")
set(NEEDS_BOOST ON)

### comment this out and save for next PR
#if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
#    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "9")
#         set(INTERNAL_CPP_STANDARD "c++17")
#	 set(NEEDS_BOOST OFF)
#    elseif(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "8")
#         set(INTERNAL_CPP_STANDARD "c++17")
#	 set(extra_lib_for_filesystem "-lstdc++fs")
#	 set(NEEDS_BOOST "OFF")
#    endif()	 
#elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
#    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "7")
#         set(INTERNAL_CPP_STANDARD "c++17")
#	 set(NEEDS_BOOST OFF)
#    endif()
#elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC")
#      if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "19.14")
#            set(INTERNAL_CPP_STANDARD "c++17")
#	    set(NEEDS_BOOST OFF)
#      endif()
#endif()


##############################################
# Global NuPIC CMake options
#
option(NUPIC_BUILD_PYEXT_MODULES
       "Turn on building of python extension modules for nupic.bindings; turn off to build only static nupic_core lib with full symbol visibility."
       ON)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()
if(NOT PY_EXTENSIONS_DIR)
	set(PY_EXTENSIONS_DIR ${REPOSITORY_DIR}/bindings/py/src/nupic/bindings)
endif()
if(NOT CMAKE_INSTALL_PREFIX)
	string(TOLOWER ${CMAKE_BUILD_TYPE} dir)
	set(CMAKE_INSTALL_PREFIX "${REPOSITORY_DIR}/build/${dir}")
endif()


message(STATUS "REPOSITORY_DIR        = ${REPOSITORY_DIR}")
message(STATUS "CMAKE_SOURCE_DIR      = ${CMAKE_SOURCE_DIR}")
message(STATUS "CMAKE_BINARY_DIR      = ${CMAKE_BINARY_DIR}")
message(STATUS "NUPIC_BUILD_PYEXT_MODULES = ${NUPIC_BUILD_PYEXT_MODULES}")
message(STATUS "PY_EXTENSIONS_DIR     = ${PY_EXTENSIONS_DIR}")

message(STATUS "CMAKE_CXX_COMPILER_ID = ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "CMAKE_CXX_COMPILER_VERSION = ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "CMAKE_C_COMPILER      = ${CMAKE_C_COMPILER}")
message(STATUS "CMAKE_CXX_COMPILER    = ${CMAKE_CXX_COMPILER}")
message(STATUS "CMAKE_SYSTEM_NAME     = ${CMAKE_SYSTEM_NAME}")
message(STATUS "INTERNAL_CPP_STANDARD = ${INTERNAL_CPP_STANDARD}")
message(STATUS "extra_lib_for_filesystem = ${extra_lib_for_filesystem}")
message(STATUS "CMAKE_BUILD_TYPE      = ${CMAKE_BUILD_TYPE}")
message(STATUS "CMAKE_INSTALL_PREFIX  = ${CMAKE_INSTALL_PREFIX}")


# Make sure we can link against any specified paths.
include_directories(SYSTEM ${CMAKE_INCLUDE_PATH})

set(EP_BASE ${CMAKE_BINARY_DIR}/ThirdParty)

include(ExternalProject)

# Set up builds of external dependencies and get their exports.
# (see individual external/*.cmake modules for exported settings and functions)
# contains an ExternalProject for each dependancy.
#add_subdirectory(external)
ExternalProject_Add(external
	PREFIX ${REPOSITORY_DIR}
	SOURCE_DIR ${REPOSITORY_DIR}/external
        CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
		-DREPOSITORY_DIR=${REPOSITORY_DIR}
		-DNEEDS_BOOST=${NEEDS_BOOST}
		-DEP_BASE=${EP_BASE}
		-DNUPIC_BUILD_PYEXT_MODULES=${NUPIC_BUILD_PYEXT_MODULES}
	BINARY_DIR ${CMAKE_BINARY_DIR}/external
	STAMP_DIR  ${CMAKE_BINARY_DIR}
	TMP_DIR /tmp/nupic
	INSTALL_COMMAND ""
	)

# Now build nupic_core project.
# NOTE: since this is a SuperBuild, 
#       our nupic_core target could also be an ExternalProject_Add
#	When we move Python interface outside of the nupic_core build 
#	it will be another ExternalProject.
#
ExternalProject_Add(nupic_core
	PREFIX ${REPOSITORY_DIR}
	DEPENDS external
	SOURCE_DIR ${REPOSITORY_DIR}/src
        CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
		-DREPOSITORY_DIR=${REPOSITORY_DIR}
		-DINTERNAL_CPP_STANDARD=${INTERNAL_CPP_STANDARD}
		-DNEEDS_BOOST=${NEEDS_BOOST}
		-Dextra_lib_for_filesystem=${extra_lib_for_filesystem}
		-DEP_BASE=${EP_BASE}
		-DPY_EXTENSIONS_DIR=${PY_EXTENSIONS_DIR}
		-DNUPIC_BUILD_PYEXT_MODULES=${NUPIC_BUILD_PYEXT_MODULES}
		-DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
	BINARY_DIR ${CMAKE_BINARY_DIR}/src
	STAMP_DIR  ${CMAKE_BINARY_DIR}
	TMP_DIR /tmp/nupic
	INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
	)

##################################
Install(CODE "MESSAGE(\"Installation Complete\")")

