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
# NOTE: not used by Windows Visual Studio; it uses std::filesystem rather than boost::filesystem.
#       However, MinGW and MSYS will use this so will need a few fixes.
#
#  Documentation: https://boostorg.github.io/build/manual/develop/index.html
#                 https://boostorg.github.io/build/tutorial.html
#
######################################################################
# MinGW Notes: MinGW needs special handling.
# 1) The default Library filename is something like libboost_filesystem-mgw49-mt-x64-1_68.a 
#    and should be libboost_filesystem.a  so that find_package() can find it.
#    To fix: specify all options.	
#               address-model=${BITNESS} 
#		threading=multi
#               architecture=x86
#		link=static
#		variant=${variant}
# 2) Include path was something like "include/boost-1_68/boost/*"  rather than "boost/*"
#    To fix: specify the include dir path
#		--includedir=${BOOST_ROOT}
# 3) Objects in the library by default are .o extension  but we need .obj so that 
#    COMBINE_UNIT_ARCHIVES() will work. Static libraries still have the .a extension.
#
# We will be eventually replace MinGW with Visual Studio as the Windows build.
######################################################################

message(STATUS "----- Boost External Project ------")


set(BOOST_ROOT "${EP_BASE}/Install/boost")

set(cxx_flags "${EXTERNAL_CXX_FLAGS_OPTIMIZED} ${COMMON_COMPILER_DEFINITIONS_STR}")
string(TOLOWER ${CMAKE_BUILD_TYPE} variant)

message(STATUS "Boost: cxx_flags=${cxx_flags}")

# Set some parameters

if (MSVC OR MSYS OR MINGW)
  if (MSYS OR MINGW)
    set(bootstrap bootstrap.bat gcc)
    set(toolset toolset=gcc architecture=x86)
  elseif(MSVC)
    set(bootstrap bootstrap.bat vc141)
    set(toolset toolset=msvc-15.0 architecture=x86 runtime-link=shared)
  endif()
  #set(BOOST_URL "${REPOSITORY_DIR}/external/common/share/boost/boost_1_68_0.zip")
  set(BOOST_URL "https://dl.bintray.com/boostorg/release/1.68.0/source/boost_1_68_0.zip")
  set(BOOST_HASH "3b1db0b67079266c40b98329d85916e910bbadfc3db3e860c049056788d4d5cd")

else()
  #set(BOOST_URL "${REPOSITORY_DIR}/external/common/share/boost/boost_1_68_0.tar.gz")
  set(BOOST_URL "https://dl.bintray.com/boostorg/release/1.68.0/source/boost_1_68_0.tar.gz")
  set(BOOST_HASH "da3411ea45622579d419bfda66f45cd0f8c32a181d84adfa936f5688388995cf")
  set(bootstrap "./bootstrap.sh")
  set(toolset) # b2 will figure out the toolset
endif()
  
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
		--layout=system
		--includedir=${BOOST_ROOT}
		address-model=${BITNESS} 
		threading=multi
		link=static ${toolset}
		variant=${variant}
		cxxflags=${cxx_flags}
		install
	BUILD_IN_SOURCE 1
	INSTALL_COMMAND ""
	INSTALL_DIR ${BOOST_ROOT}
	LOG_DOWNLOAD 1
	LOG_BUILD 1
)



message(STATUS "Boost will be installed at BOOST_ROOT = ${BOOST_ROOT}")


