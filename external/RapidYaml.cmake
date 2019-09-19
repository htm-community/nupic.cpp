# -----------------------------------------------------------------------------
# HTM Community Edition of NuPIC
# Copyright (C) 2019, Numenta, Inc.
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
# -----------------------------------------------------------------------------

# Rapid YAML must be downloaded in four parts.
#   - ryml from repository at https://github.com/biojppm/rapidyaml     - the main library
#  - c4core from repository at https://github.com/biojppm/c4core/    - the C library of utilities
#  - cmake from repository at https://github.com/biojppm/cmake/      - Cmake scripts
#  - debugbreak from repository at https://github.com/biojppm/debugbreak/      - debug break function
# There are links from rapidyaml to a specific branch of c4core in the extern folder of rapidyaml
# but the downloader does not follow it.  So we need to do three downloads.

message(STATUS "${REPOSITORY_DIR}/build/ThirdParty/share/ryml.zip")
if(  (EXISTS "${REPOSITORY_DIR}/build/ThirdParty/share/ryml.zip")
 AND (EXISTS "${REPOSITORY_DIR}/build/ThirdParty/share/ryml_c4core.zip")
 AND (EXISTS "${REPOSITORY_DIR}/build/ThirdParty/share/ryml_cmake.zip")
 AND (EXISTS "${REPOSITORY_DIR}/build/ThirdParty/share/ryml_debugbreak.zip"))
    set(URL1 "${REPOSITORY_DIR}/build/ThirdParty/share/ryml.zip")
    set(URL2 "${REPOSITORY_DIR}/build/ThirdParty/share/ryml_c4core.zip")
    set(URL3 "${REPOSITORY_DIR}/build/ThirdParty/share/ryml_cmake.zip")
    set(URL4 "${REPOSITORY_DIR}/build/ThirdParty/share/ryml_debugbreak.zip")
else()
         #There are no releases.  Use the masters.
    set(URL1 https://github.com/biojppm/rapidyaml/archive/master.zip)
    set(URL2 https://github.com/biojppm/c4core/archive/master.zip)
    set(URL3 https://github.com/biojppm/cmake/archive/master.zip)
    set(URL4 https://github.com/biojppm/debugbreak/archive/master.zip)
endif()

message(STATUS "Obtaining RapidYAML from ${URL1}" )
include(DownloadProject/DownloadProject.cmake)
download_project(PROJ ryml
	PREFIX "${EP_BASE}/ryml"
	URL ${URL1}
	UPDATE_DISCONNECTED 1
	QUIET
	)
download_project(PROJ ryml_c4core
	PREFIX "${EP_BASE}/ryml_c4core"
	SOURCE_DIR "${ryml_SOURCE_DIR}/extern/c4core"
	URL ${URL2}
	UPDATE_DISCONNECTED 1
	QUIET
	)
download_project(PROJ ryml_cmake
	PREFIX "${EP_BASE}/ryml_cmake"
	SOURCE_DIR "${ryml_SOURCE_DIR}/extern/c4core/cmake"
	URL ${URL3}
	UPDATE_DISCONNECTED 1
	QUIET
	)
download_project(PROJ ryml_debugbreak
	PREFIX "${EP_BASE}/ryml_debugbreak"
	SOURCE_DIR "${ryml_SOURCE_DIR}/extern/c4core/extern/debugbreak"
	URL ${URL4}
	UPDATE_DISCONNECTED 1
	QUIET
	)

#Need to enable Exceptions    
set(c4path "${ryml_SOURCE_DIR}/extern/c4core/src/c4")
file(RENAME "${c4path}/config.hpp" "${c4path}/config1.hpp")
file(READ "${c4path}/config1.hpp" content1)
string(REPLACE "//#define C4_ERROR_THROWS_EXCEPTION" 
               "#define C4_ERROR_EXCEPTIONS_ENABLED\n#define C4_ERROR_THROWS_EXCEPTION" 
               content 
               "${content1}")
file(WRITE "${c4path}/config.hpp" "${content}")

file(RENAME "${c4path}/error.cpp" "${c4path}/error1.cpp")
file(READ "${c4path}/error1.cpp" content2)
string(REPLACE "if(s_error_flags & (ON_ERROR_LOG|ON_ERROR_CALLBACK))" 
               "if(s_error_flags & (ON_ERROR_LOG|ON_ERROR_CALLBACK|ON_ERROR_THROW))" 
               content1
               "${content2}")
string(REPLACE "throw Exception(ss.c_str())"
               "throw Exception(buf)"
               content
               "${content1}")
file(WRITE "${c4path}/error.cpp" "${content}")

set(C4_EXCEPTIONS_ENABLED ON)
set(C4_ERROR_THROWS_EXCEPTION ON)
add_subdirectory(${ryml_SOURCE_DIR} ${ryml_BINARY_DIR})

set(yaml_INCLUDE_DIRS "${ryml_SOURCE_DIR}/src@@@${ryml_SOURCE_DIR}/extern/c4core/src@@@${ryml_SOURCE_DIR}/extern/c4core/extern")
if (MSVC)
  set(yaml_LIBRARIES   "${ryml_BINARY_DIR}$<$<CONFIG:Release>:/Release/ryml.lib>$<$<CONFIG:Debug>:/Debug/ryml.lib>") 
else()
  set(yaml_LIBRARIES   ${ryml_BINARY_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}ryml${CMAKE_STATIC_LIBRARY_SUFFIX}) 
endif()
FILE(APPEND "${EXPORT_FILE_NAME}" "yaml_INCLUDE_DIRS@@@${yaml_INCLUDE_DIRS}\n")
FILE(APPEND "${EXPORT_FILE_NAME}" "yaml_LIBRARIES@@@${yaml_LIBRARIES}\n")

