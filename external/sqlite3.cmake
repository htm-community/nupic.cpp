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
# -----------------------------------------------------------------------------
# This downloads and builds the sqlite3 library.
#
# SQLite is a C-language library that implements a small, fast, self-contained, 
#        high-reliability, full-featured, SQL database engine. SQLite is the most used 
#        database engine in the world.
#
#            The current release is 3.34.1 (2020-06-18)
#            The repository is at https://www.sqlite.org/2020/sqlite-autoconf-3340100.tar.gz
#
#
if(EXISTS   ${REPOSITORY_DIR}/build/ThirdParty/share/sqlite3.tar.gz)
    set(URL ${REPOSITORY_DIR}/build/ThirdParty/share/sqlite3.tar.gz)
else()
    set(URL "https://www.sqlite.org/2021/sqlite-autoconf-3340100.tar.gz")
endif()

message(STATUS "Obtaining sqlite3")
include(DownloadProject/DownloadProject.cmake)
download_project(PROJ sqlite3
	PREFIX ${EP_BASE}/sqlite3
	URL ${URL}
	GIT_SHALLOW ON
	UPDATE_DISCONNECTED 1
	QUIET
	)
    
# SQLite does not provide a CMakeList.txt to buld with so we provide the following lines to perform the compile here.
# Since we are building here and not in the source folder, the library will show up directly under build/ThirdParty.
# The library will be folded into htm.core.
# Reference the include as #include "sqlite3.h".

set(SRCS ${sqlite3_SOURCE_DIR}/sqlite3.c ${sqlite3_SOURCE_DIR}/sqlite3.h)
add_library(sqlite3 STATIC  ${SRCS} )
target_compile_definitions(sqlite3 PRIVATE ${COMMON_COMPILER_DEFINITIONS})

if (MSVC)
  set(sqlite3_LIBRARIES   "${CMAKE_BINARY_DIR}$<$<CONFIG:Release>:/Release/sqlite3.lib>$<$<CONFIG:Debug>:/Debug/sqlite3.lib>") 
else()
  set(sqlite3_LIBRARIES   ${CMAKE_BINARY_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}sqlite3${CMAKE_STATIC_LIBRARY_SUFFIX}) 
endif()

FILE(APPEND "${EXPORT_FILE_NAME}" "sqlite3_INCLUDE_DIRS@@@${sqlite3_SOURCE_DIR}\n")
FILE(APPEND "${EXPORT_FILE_NAME}" "sqlite3_LIBRARIES@@@${sqlite3_LIBRARIES}\n")

