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

cmake_minimum_required(VERSION 3.7)
project(nupic_core CXX)

message(STATUS "----- SuperBuild for nupic_core -----")

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


if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()
if(NOT PY_EXTENSIONS_DIR)
	set(PY_EXTENSIONS_DIR ${PROJECT_SOURCE_DIR}/bindings/py/src/nupic/bindings)
endif()
IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  SET(CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}/build/${CMAKE_BUILD_TYPE}" CACHE PATH "Default Install location" FORCE)
ENDIF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}")

set(CMAKE_VERBOSE_MAKEFILE OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

get_filename_component(REPOSITORY_DIR ${PROJECT_SOURCE_DIR} ABSOLUTE)

set(INTERNAL_CPP_STANDARD "C++11")
set(NEEDS_BOOST ON)
# Global NuPIC CMake options
#
option(NUPIC_BUILD_PYEXT_MODULES
       "Turn on building of python extension modules for nupic.bindings; turn off to build only static nupic_core lib with full symbol visibility."
       ON)


# Identify platform "bitness".
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(BITNESS 64)
else()
  set(BITNESS 32)
endif()


# Identify platform name.
string(TOLOWER ${CMAKE_SYSTEM_NAME} PLATFORM)

# Define a platform suffix, eg ${PLATFORM}${BITNESS}${PLATFORM_SUFFIX}
if (MSYS OR MINGW)
  set(PLATFORM_SUFFIX -gcc)
endif()

message(STATUS "INTERNAL_CPP_STANDARD     = ${INTERNAL_CPP_STANDARD}")
message(STATUS "CMAKE BITNESS             = ${BITNESS}")
message(STATUS "PLATFORM                  = ${PLATFORM}")
message(STATUS "NUPIC_BUILD_PYEXT_MODULES = ${NUPIC_BUILD_PYEXT_MODULES}")
message(STATUS "PY_EXTENSIONS_DIR         = ${PY_EXTENSIONS_DIR}")

message(STATUS "CMAKE_CXX_COMPILER_ID = ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "CMAKE_CXX_COMPILER_VERSION = ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "CMAKE_C_COMPILER      = ${CMAKE_C_COMPILER}")
message(STATUS "CMAKE_CXX_COMPILER    = ${CMAKE_CXX_COMPILER}")
message(STATUS "CMAKE_BUILD_TYPE      = ${CMAKE_BUILD_TYPE}")
message(STATUS "CMAKE_INSTALL_PREFIX  = ${CMAKE_INSTALL_PREFIX}")


# Make sure we can link against any specified paths.
include_directories(SYSTEM ${CMAKE_INCLUDE_PATH})

set(EP_BASE ${CMAKE_BINARY_DIR}/ThirdParty)

# Set up builds of external dependencies and get their exports.
# (see individual external/*.cmake modules for exported settings and functions)
# We build the external modules using C++11 standard.
include(ExternalProject)

ExternalProject_Add(external
	PREFIX ${REPOSITORY_DIR}
	SOURCE_DIR ${REPOSITORY_DIR}/external
	CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
		-DREPOSITORY_DIR=${REPOSITORY_DIR}
		-DNEEDS_BOOST=${NEEDS_BOOST}
		-DEP_BASE=${EP_BASE}
		-DNUPIC_BUILD_PYEXT_MODULES=${NUPIC_BUILD_PYEXT_MODULES}
		-DPLATFORM=${PLATFORM}
		-DBITNESS=${BITNESS}
	BINARY_DIR ${CMAKE_BINARY_DIR}/external
	STAMP_DIR  ${CMAKE_BINARY_DIR}
	TMP_DIR /tmp/nupic
	INSTALL_COMMAND ""
	)



# Now build nupic_core project.
# NOTE: since this is a SuperBuild, 
#       our nupic_core target could also be an ExternalProject
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
		-DEP_BASE=${EP_BASE}
		-DPY_EXTENSIONS_DIR=${PY_EXTENSIONS_DIR}
		-DNUPIC_BUILD_PYEXT_MODULES=${NUPIC_BUILD_PYEXT_MODULES}
		-DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
		-DPLATFORM=${PLATFORM}
		-DBITNESS=${BITNESS}
	BINARY_DIR ${CMAKE_BINARY_DIR}/src
	STAMP_DIR  ${CMAKE_BINARY_DIR}
	TMP_DIR /tmp/nupic
	INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
	)

##################################
Install(CODE "MESSAGE(\"Installation Complete\")")

