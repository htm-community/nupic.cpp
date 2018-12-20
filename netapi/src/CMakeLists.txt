# -----------------------------------------------------------------------------
# Numenta Platform for Intelligent Computing (NuPIC)
# Copyright (C) 2013-2015, Numenta, Inc.  Unless you have purchased from
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

message(STATUS "Configuring nupic_netapi src")
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_VERBOSE_MAKEFILE ON) # toggle for cmake debug

include(${REPOSITORY_DIR}/NupicLibraryUtils.cmake) # for MERGE_STATIC_LIBRARIES

#
# NuPIC CMake options
#
option(NUPIC_IWYU "Enable include-what-you-use
  (http://include-what-you-use.org/). This requires the iwyu binary to be
  discoverable by CMake's find_program, with a minimum CMake version of 3.3.")
if(${NUPIC_IWYU})
  find_program(iwyu_path NAMES include-what-you-use iwyu)
  if(NOT iwyu_path)
    message(FATAL_ERROR "Could not find the program include-what-you-use")
  endif()
endif()


message(STATUS "in netapi/src: EXTERNAL_INCLUDES=${EXTERNAL_INCLUDES}")

#################
# Setup nupic_core static library, consisting of our own sources and yaml libraries;
#
# this is an intermediate archive that will be merged with htm and external static
# libraries in a subsequent step to produce the nupic_core "combined" static
# library.
#
set(src_lib_static_netapi nupic.netapi)

    
set(engine_files
    nupic/engine/Collections.cpp
    nupic/engine/Input.cpp
    nupic/engine/Input.hpp
    nupic/engine/Link.cpp
    nupic/engine/Link.hpp
    nupic/engine/LinkPolicy.hpp
    nupic/engine/LinkPolicyFactory.cpp
    nupic/engine/LinkPolicyFactory.hpp
    nupic/engine/Network.cpp
    nupic/engine/Network.hpp
    nupic/engine/NuPIC.cpp
    nupic/engine/NuPIC.hpp
    nupic/engine/Output.cpp
    nupic/engine/Output.hpp
    nupic/engine/Region.cpp
    nupic/engine/Region.hpp
    nupic/engine/RegionImpl.cpp
    nupic/engine/RegionImpl.hpp
    nupic/engine/RegionImplFactory.cpp
    nupic/engine/RegionImplFactory.hpp
    nupic/engine/RegionIo.cpp
    nupic/engine/RegionParameters.cpp
    nupic/engine/RegisteredRegionImpl.hpp
    nupic/engine/RegisteredRegionImplCpp.hpp
    nupic/engine/Spec.cpp
    nupic/engine/Spec.hpp
    nupic/engine/TestFanIn2LinkPolicy.cpp
    nupic/engine/TestFanIn2LinkPolicy.hpp
    nupic/engine/UniformLinkPolicy.cpp
    nupic/engine/UniformLinkPolicy.hpp
    nupic/engine/Watcher.cpp
    nupic/engine/Watcher.hpp
    nupic/engine/YAMLUtils.cpp
    nupic/engine/YAMLUtils.hpp
)

set(ntypes_files
    nupic/ntypes/Array.hpp
    nupic/ntypes/ArrayBase.cpp
    nupic/ntypes/ArrayBase.hpp
    nupic/ntypes/ArrayRef.hpp
    nupic/ntypes/BasicType.cpp
    nupic/ntypes/BasicType.hpp
    nupic/ntypes/Buffer.cpp
    nupic/ntypes/Buffer.hpp
    nupic/ntypes/BundleIO.hpp
    #nupic/ntypes/Collection.cpp  Included directly into Collections.cpp
    nupic/ntypes/Collection.hpp
    nupic/ntypes/Dimensions.cpp
    nupic/ntypes/Dimensions.hpp
    nupic/ntypes/MemParser.cpp
    nupic/ntypes/MemParser.hpp
    nupic/ntypes/MemStream.hpp
    nupic/ntypes/NodeSet.hpp
    nupic/ntypes/ObjectModel.h
    nupic/ntypes/ObjectModel.hpp
    nupic/ntypes/Scalar.cpp
    nupic/ntypes/Scalar.hpp
    nupic/ntypes/Value.cpp
    nupic/ntypes/Value.hpp
)
  
set(os_files
    nupic/os/Directory.cpp
    nupic/os/Directory.hpp
    nupic/os/Env.cpp
    nupic/os/Env.hpp
    nupic/os/ImportFilesystem.hpp
    nupic/os/OS.cpp
    nupic/os/OS.hpp
    nupic/os/OSUnix.cpp
    nupic/os/OSWin.cpp
    nupic/os/Path.cpp
    nupic/os/Path.hpp
    nupic/os/Regex.cpp
    nupic/os/Regex.hpp
)

set(regions_files
    nupic/regions/TestNode.cpp
    nupic/regions/TestNode.hpp
    nupic/regions/ScalarSensor.cpp
    nupic/regions/ScalarSensor.hpp
    nupic/regions/VectorFile.cpp
    nupic/regions/VectorFile.hpp
    nupic/regions/VectorFileEffector.cpp
    nupic/regions/VectorFileEffector.hpp
    nupic/regions/VectorFileSensor.cpp
    nupic/regions/VectorFileSensor.hpp   
)


#set up file tabs in Visual Studio
source_group("engine" FILES ${engine_files})
source_group("ntypes" FILES ${ntypes_files})
source_group("os" FILES ${os_files})
source_group("regions" FILES ${regions_files})

########################################################
# Lib
add_library(${src_lib_static_netapi} STATIC 
    ${engine_files} 
    ${ntypes_files} 
    ${os_files} 
    ${regions_files} 
)

# nupic_core_solo sources depend on headers installed by these external projects
add_dependencies(${src_lib_static_netapi}
        yaml-cpp
		)
target_compile_options( ${src_lib_static_netapi} PUBLIC ${INTERNAL_CXX_FLAGS})
target_compile_definitions(${src_lib_static_netapi} PRIVATE ${COMMON_COMPILER_DEFINITIONS})
target_include_directories(${src_lib_static_netapi} PRIVATE 
		${NETAPI_LIB_INCLUDES} 
		${HTM_LIB_INCLUDES} 
		${EXTERNAL_INCLUDES}
		)
if(${NUPIC_IWYU})
  set_target_properties(${src_lib_static_netapi}
    PROPERTIES CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path})
endif()

set(src_lib_static_netapi ${src_lib_static_netapi} PARENT_SCOPE)


#
# Create the nupic_core as a "combined" static library by merging nupic_netapi and nupic_htm
# with our external static libraries
#
set(core_library   nupic_core)
set(src_combined_nupiccore_source_archives
    ${src_lib_static_netapi}
    ${src_lib_static_htm}
    ${yaml-cpp_LIBRARIES}
    ${Boost_LIBRARIES}
    )
message(STATUS "creating nupic_core from Libraries: ${src_combined_nupiccore_source_archives}")
merge_static_libraries(${core_library} "${src_combined_nupiccore_source_archives}")
set(core_library ${core_library} PARENT_SCOPE)


#########################################################
# Test
# The tests were placed in a separate file to avoid clutering up this file.
add_subdirectory(test)




##################################################
#
# Install targets into CMAKE_INSTALL_PREFIX
#
install(TARGETS
        ${core_library}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

# Version.hpp is also used by the nupic.bindings release/deployment system
install(FILES ${PROJECT_BINARY_DIR}/Version.hpp
        DESTINATION include/nupic)

# install Data.csv for a test
install(FILES "${PROJECT_SOURCE_DIR}/test/unit/engine/Data.csv"
        DESTINATION "data/")

install(DIRECTORY ${PROJECT_SOURCE_DIR} DESTINATION include
	  MESSAGE_NEVER
	  FILES_MATCHING PATTERN "nupic/*/*.h*"
	  PATTERN "nupic/*/*.hpp.in" EXCLUDE)

install(DIRECTORY ${REPOSITORY_DIR}/external/common/include/gtest
	  MESSAGE_NEVER
	  DESTINATION include/gtest
	  FILES_MATCHING PATTERN "*.h*")

install(DIRECTORY "${REPOSITORY_DIR}/external/common/include/"
	  MESSAGE_NEVER
	  DESTINATION include)

install(TARGETS
        ${src_executable_hotgym}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

          
#
# `make package` results in
# nupic_core-${NUPIC_CORE_VERSION}-${PLATFORM}${BITNESS}${PLATFORM_SUFFIX}.tar.gz binary release
#

set(CPACK_GENERATOR "TGZ")
set(CPACK_PACKAGE_FILE_NAME "nupic_core-${NUPIC_CORE_VERSION}-${PLATFORM}${BITNESS}${PLATFORM_SUFFIX}")
include(CPack)
