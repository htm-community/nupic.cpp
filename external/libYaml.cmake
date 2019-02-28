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
# This downloads and builds the libyaml library.
#
if(EXISTS ${REPOSITORY_DIR}/build/ThirdParty/share/libyaml.zip
    set(URL ${REPOSITORY_DIR}/build/ThirdParty/share/libyaml.zip)
else()
    set(URL "https://github.com/yaml/libyaml/archive/master.zip")
endif()

message(STATUS "Obtaining libyaml")
include(DownloadProject/DownloadProject.cmake)
download_project(PROJ libyaml
	PREFIX ${EP_BASE}/libyaml
	URL ${URL}
	GIT_SHALLOW ON
	UPDATE_DISCONNECTED 1
	QUIET
	)
    
add_subdirectory(${libyaml_SOURCE_DIR} ${libyaml_BINARY_DIR})

set(libyaml_INCLUDE_DIRS ${libyaml_SOURCE_DIR}/include) 
if (MSVC)
  set(libyaml_LIBRARIES   "${libyaml_BINARY_DIR}$<$<CONFIG:Release>:/Release/liblibyamlmd.lib>$<$<CONFIG:Debug>:/Debug/liblibyamlmdd.lib>") 
else()
  set(ylibyaml_LIBRARIES   ${libyaml_BINARY_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}libyaml${CMAKE_STATIC_LIBRARY_SUFFIX}) 
endif()
FILE(APPEND "${EXPORT_FILE_NAME}" "libyaml_INCLUDE_DIRS@@@${libyaml_SOURCE_DIR}/include\n")
FILE(APPEND "${EXPORT_FILE_NAME}" "libyaml_LIBRARIES@@@${libyaml_LIBRARIES}\n")

