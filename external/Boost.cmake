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
# OUTPUT VARIABLES:
#
#   BOOST_ROOT_DIR: directory where boost is installed
#   BOOST_INCLUDE_DIR: directory where boost includes are located
message(STATUS "----- Boost External Project ------")


# Set some parameters

if (MSYS OR MINGW)
#set(BOOST_URL "${REPOSITORY_DIR}/external/common/share/boost/boost_1_68_0.zip")
set(BOOST_URL "https://dl.bintray.com/boostorg/release/1.68.0/source/boost_1_68_0.zip")
set(BOOST_HASH "3b1db0b67079266c40b98329d85916e910bbadfc3db3e860c049056788d4d5cd")
set(bootstrap "bootstrap.bat")
else()
#set(BOOST_URL "${REPOSITORY_DIR}/external/common/share/boost/boost_1_68_0.tar.gz")
set(BOOST_URL "https://dl.bintray.com/boostorg/release/1.68.0/source/boost_1_68_0.tar.gz")
set(BOOST_HASH "da3411ea45622579d419bfda66f45cd0f8c32a181d84adfa936f5688388995cf")
set(bootstrap "./bootstrap.sh")
endif()

set(BOOST_ROOT "${EP_BASE}/Install/boost")



set(c_flags "${EXTERNAL_C_FLAGS_OPTIMIZED} ${COMMON_COMPILER_DEFINITIONS_STR}")
set(cxx_flags "${EXTERNAL_CXX_FLAGS_OPTIMIZED} ${COMMON_COMPILER_DEFINITIONS_STR}")

ExternalProject_Add( Boost
	URL ${BOOST_URL}
	URL_HASH SHA256=${BOOST_HASH}
	UPDATE_COMMAND ""
        CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
	           -DCMAKE_C_FLAGS=${c_flags}
                   -DCMAKE_CXX_FLAGS=${cxx_flags}
	CONFIGURE_COMMAND ${bootstrap}
		--with-libraries=filesystem 
		--with-libraries=system
		--prefix=${BOOST_ROOT}
	BUILD_COMMAND ./b2 link=static 
		--prefix=${BOOST_ROOT} install
	BUILD_IN_SOURCE 1
	INSTALL_COMMAND ""
	INSTALL_DIR ${BOOST_ROOT}
	LOG_DOWNLOAD 1
	LOG_BUILD 1
)

message(STATUS "Boost at installed at BOOST_ROOT = ${BOOST_ROOT}")
message(STATUS "Boost_system = ${BOOST_ROOT}/lib/${STATIC_PRE}boost_system${STATIC_SUF}")
message(STATUS "Boost_filesystem = ${BOOST_ROOT}/lib/${STATIC_PRE}boost_filesystem${STATIC_SUF}")


