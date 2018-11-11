# -----------------------------------------------------------------------------
# Numenta Platform for Intelligent Computing (NuPIC)
# Copyright (C) 2016, Numenta, Inc.  Unless you have purchased from
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

# Creates ExternalProject for building the boost system and filesystem static libraries


message(STATUS "----- Boost External Library ------")

set(cxx_flags "${EXTERNAL_CXX_FLAGS_OPTIMIZED} ${COMMON_COMPILER_DEFINITIONS_STR}")
set(BOOST_MINIMUM_VERSION 1.63.0)
# Known versions of Boost......
# 	Boost 1.63 known by CMake 3.7 or newer.  -- our minimum CMake version
# 	Boost 1.64 known by CMake 3.8 or newer.
# 	Boost 1.65 and 1.65.1 known by CMake 3.9.3 or newer.
# 	Boost 1.66 known by CMake 3.11 or newer.
# 	Boost 1.67 known by CMake 3.12 or newer.
# 	Boost 1.68, 1.69 known by CMake 3.13 or newer.
#  If your version of CMake does not know about your version of Boost, use Boost_ADDITIONAL_VERSIONS.
#      Just add your Boost version to this list:
set(Boost_ADDITIONAL_VERSIONS 1.69.0;1.69;1.68.0;1.68;1.67.0;1.67;1.66.0;1.66;1.65.1;1.65.0;1.65;1.64.0;1.64;1.63.0;1.63 )    
string (REPLACE ";" "$<SEMICOLON>" Boost_ADDITIONAL_VERSIONS "${Boost_ADDITIONAL_VERSIONS}")

#############################
#  Determine if an existing BOOST_ROOT can be found.
if(NOT BOOST_ROOT)
  if (NOT "$ENV{BOOST_ROOT}" STREQUAL "")
    set(BOOST_ROOT $ENV{BOOST_ROOT})
  endif()
endif()
if(NOT BOOST_INCLUDEDIR)
  if (NOT "$ENV{BOOST_INCLUDEDIR}" STREQUAL "")
    set(BOOST_INCLUDEDIR $ENV{BOOST_INCLUDEDIR})
  endif()
endif()
if(NOT BOOST_LIBRARYDIR)
  if (NOT "$ENV{BOOST_LIBRARYDIR}" STREQUAL "")
    set(BOOST_LIBRARYDIR $ENV{BOOST_LIBRARYDIR})
  endif()
endif()
message(STATUS "Hints for Boost find_package()")
message(STATUS "  BOOST_ROOT  = ${BOOST_ROOT}")
message(STATUS "  BOOST_INCLUDEDIR = ${BOOST_INCLUDEDIR}")
message(STATUS "  BOOST_LIBRARYDIR = ${BOOST_LIBRARYDIR}")
message(STATUS "  Minimum Boost version=${BOOST_MINIMUM_VERSION}")
message(STATUS "  expecting multithreaded=on, link=static, static_runtime=off")

set(Boost_USE_STATIC_RUNTIME OFF)  
string(TOLOWER ${CMAKE_BUILD_TYPE} variant)
set(Boost_USE_STATIC_LIBS        ON)  # only find static libs
set(Boost_USE_MULTITHREADED      ON)
message(STATUS "Expect some annoying warnings from Boost about missing dependancies.  It is ok.")
#set(Boost_DETAILED_FAILURE_MSG ON)
#set(Boost_DEBUG ON)

find_package(Boost ${BOOST_MINIMUM_VERSION} COMPONENTS system filesystem)
if(Boost_FOUND)
  message(STATUS "  Found pre-installed Boost ${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}.${Boost_SUBMINOR_VERSION} at ${Boost_LIBRARY_DIRS}")
  message(STATUS "  Boost_INCLUDE_DIRS = ${Boost_INCLUDE_DIRS}")
  message(STATUS "  Boost_LIBRARIES = ${Boost_LIBRARIES}")
  set(BOOST_INCLUDEDIR ${Boost_INCLUDE_DIRS} CACHE STRING 
     "Boost_LIBRARY_DIRS points to the boost libraries." FORCE)
  set(BOOST_LIBRARYDIR ${Boost_LIBRARY_DIRS} CACHE STRING 
     "Boost_LIBRARY_DIRS points to the boost libraries." FORCE)
else()
  message(STATUS "A sutable Boost library not found. Building an internal boost subset.")
  
#######################################



# Set some parameters
  set(BOOST_ROOT ${EP_BASE}/Source/Boost)
  if (MSVC OR MSYS OR MINGW)
    if (MSYS OR MINGW)
	  set(bootstrap bootstrap.bat gcc)
	  set(toolset toolset=gcc architecture=x86)
    elseif(MSVC)
	  set(bootstrap bootstrap.bat vc141)
	  set(toolset toolset=msvc-15.0 architecture=x86)
    endif()
    set(BOOST_URL "${REPOSITORY_DIR}/external/common/share/boost/boost_1_68_0_subset.zip")
    set(BOOST_HASH "f153a98a8b49f08ab52b97f6d8d129bae9dc4a5f2a772d96cdd2f415a87f4544")
    #set(BOOST_URL "https://dl.bintray.com/boostorg/release/1.68.0/source/boost_1_68_0.zip")
    #set(BOOST_HASH "3b1db0b67079266c40b98329d85916e910bbadfc3db3e860c049056788d4d5cd")

  else()
    set(bootstrap "./bootstrap.sh")
    set(toolset) # b2 will figure out the toolset
    set(BOOST_URL "https://dl.bintray.com/boostorg/release/1.68.0/source/boost_1_68_0.tar.gz")
    set(BOOST_HASH "da3411ea45622579d419bfda66f45cd0f8c32a181d84adfa936f5688388995cf")
  endif()
  # For Windows this will build 4 libraries per module.  32/64bit and release/debug variants
  # For Linux/OSx this will build 1 library per module, corresponding to current environment.
  # Static, multithreaded, shared runtime link.
  
  ExternalProject_Add( Boost
	URL ${BOOST_URL}
	URL_HASH SHA256=${BOOST_HASH}
	CMAKE_GENERATOR ${CMAKE_GENERATOR}
	UPDATE_COMMAND ""
	CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
		-DCMAKE_CXX_FLAGS=${cxx_flags} 
	CONFIGURE_COMMAND ${bootstrap}
		
	BUILD_COMMAND ./b2 
		--prefix=${BOOST_ROOT}
		--with-filesystem 
		--with-system
#		--layout=versioned
#		--build_type=complete
#		address-model=${BITNESS}
#		variant=${variant}
		threading=multi
		runtime-link=shared
		link=static ${toolset}
		stage
	BUILD_IN_SOURCE 1
	SOURCE_DIR ${BOOST_ROOT}
	INSTALL_COMMAND ""
	LOG_DOWNLOAD 1
	LOG_BUILD 1
  )
  set(Boost_INCLUDE_DIRS ${BOOST_ROOT})
  set(Boost_LIBRARY_DIRS ${BOOST_ROOT}/lib)


  message(STATUS "Boost will be installed at BOOST_ROOT = ${BOOST_ROOT}")
  set(BOOST_ROOT ${BOOST_ROOT} CACHE STRING  "BOOST_ROOT points to the boost Installation." FORCE)

  # use find_package to retreive results.
endif()


