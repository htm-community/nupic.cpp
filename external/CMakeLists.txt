# -----------------------------------------------------------------------------
# Numenta Platform for Intelligent Computing (NuPIC)
# Copyright (C) 2015, Numenta, Inc.  Unless you have purchased from
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

set(CMAKE_VERBOSE_MAKEFILE OFF)
include(GNUInstallDirs)

message(STATUS "----- external -----")
message(STATUS "REPOSITORY_DIR = ${REPOSITORY_DIR}")
message(STATUS "EP_BASE    = ${EP_BASE}")
set_directory_properties(PROPERTIES EP_BASE "${EP_BASE}")

# Determine common toolchain settings, compiler and link flags
set(INTERNAL_CPP_STANDARD C++11)
include("${REPOSITORY_DIR}/CommonCompilerConfig.cmake")


# Shorter aliases for static library prefix and suffix.
set(STATIC_PRE ${CMAKE_STATIC_LIBRARY_PREFIX})
set(STATIC_SUF ${CMAKE_STATIC_LIBRARY_SUFFIX})
# This is the library prefix path.
set(LIB_PRE ${EP_BASE}/Install/${CMAKE_INSTALL_LIBDIR})
# This is the include path.
set(INCLUDE_PRE ${EP_BASE}/Install/include)
# The full prefix path for binaries.
set(BIN_PRE ${EP_BASE}/Install/bin)

include(ExternalProject)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}")

include(Swig)

include(Apr1Lib)

# AprUtil1Lib MUST be imported after Apr1Lib due to depencies on exported
# properties
include(AprUtil1Lib)

include(YamlLib)

include(YamlCppLib)

if(${NEEDS_BOOST})
  include(Boost)
endif()

