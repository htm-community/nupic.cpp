# -----------------------------------------------------------------------------
# HTM Community Edition of NuPIC
# Copyright (C) 2020, Numenta, Inc.
#
# author: David Keeney, dkeeney@gmail.com, Feb 2020
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

# Fetch cpp-httplib package from GitHub archive
# This is a C++11 single-file header-only cross platform HTTP/HTTPS web server library.
# This is used only for the REST server example and not included in the htm.core library.
#
if(EXISTS "${REPOSITORY_DIR}/build/ThirdParty/share/cpp-httplib.zip")
    set(URL "${REPOSITORY_DIR}/build/ThirdParty/share/cpp-httplib.zip")
else()
    set(URL https://github.com/yhirose/cpp-httplib/archive/v0.8.4.zip)
endif()

message(STATUS "obtaining cpp-httplib")
include(DownloadProject/DownloadProject.cmake)
download_project(PROJ cpp-httplib
	PREFIX ${EP_BASE}/cpp-httplib
	URL ${URL}
	UPDATE_DISCONNECTED 1
	QUIET
	)

# No build. This is a header only package


FILE(APPEND "${EXPORT_FILE_NAME}" "cpp-httplib_INCLUDE_DIRS@@@${cpp-httplib_SOURCE_DIR}\n")
