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

message(STATUS "Configuring nupic_htm src")
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_VERBOSE_MAKEFILE ON) # toggle for cmake debug


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

message(STATUS "in htm/src: EXTERNAL_INCLUDES=${EXTERNAL_INCLUDES}")

###############
# Create Version.hpp
# Extract current git sha and record in nupic/Version.hpp.in
#
execute_process(COMMAND git rev-parse HEAD
                OUTPUT_VARIABLE NUPIC_CORE_VERSION
                OUTPUT_STRIP_TRAILING_WHITESPACE)
configure_file(
  "${PROJECT_SOURCE_DIR}/nupic/Version.hpp.in"
  "${PROJECT_BINARY_DIR}/Version.hpp")
  

#################
# Setup nupic_core static library, consisting of our own sources and yaml libraries;
#
# this is an intermediate archive that will be merged with external static
# libraries in a subsequent step to produce the nupic_core "combined" static
# library.
#
set(src_lib_static_htm nupic.htm)


set(algorithm_files
    nupic/algorithms/Anomaly.cpp
    nupic/algorithms/Anomaly.hpp
    nupic/algorithms/AnomalyLikelihood.cpp
    nupic/algorithms/AnomalyLikelihood.hpp
    nupic/algorithms/ArrayBuffer.hpp
    nupic/algorithms/BacktrackingTMCpp.cpp
    nupic/algorithms/BacktrackingTMCpp.hpp
    nupic/algorithms/BitHistory.cpp
    nupic/algorithms/BitHistory.hpp
    nupic/algorithms/Cell.cpp
    nupic/algorithms/Cell.hpp
    nupic/algorithms/Cells4.cpp
    nupic/algorithms/Cells4.hpp
    nupic/algorithms/ClassifierResult.cpp
    nupic/algorithms/ClassifierResult.hpp
    nupic/algorithms/Connections.cpp
    nupic/algorithms/Connections.hpp
    nupic/algorithms/InSynapse.cpp
    nupic/algorithms/InSynapse.hpp
    nupic/algorithms/OutSynapse.cpp
    nupic/algorithms/OutSynapse.hpp
    nupic/algorithms/Scanning.hpp
    nupic/algorithms/SDRClassifier.cpp
    nupic/algorithms/SDRClassifier.hpp
    nupic/algorithms/Segment.cpp
    nupic/algorithms/Segment.hpp
    nupic/algorithms/SegmentUpdate.cpp
    nupic/algorithms/SegmentUpdate.hpp
    nupic/algorithms/SpatialPooler.cpp
    nupic/algorithms/SpatialPooler.hpp
    nupic/algorithms/TemporalMemory.cpp
    nupic/algorithms/TemporalMemory.hpp
)


set(encoders_files 
    nupic/encoders/ScalarEncoder.cpp
    nupic/encoders/ScalarEncoder.hpp
)
    
  
set(math_files
    nupic/math/ArrayAlgo.hpp
    nupic/math/DenseMatrix.hpp
    nupic/math/Domain.hpp
    nupic/math/Functions.hpp
    nupic/math/Index.hpp
    nupic/math/Math.hpp
    nupic/math/Set.hpp
    nupic/math/SparseBinaryMatrix.hpp
    nupic/math/SparseMatrix.hpp
    nupic/math/SparseTensor.hpp
    nupic/math/StlIo.cpp
    nupic/math/StlIo.hpp
    nupic/math/Topology.cpp
    nupic/math/Topology.hpp
    nupic/math/Types.hpp
    nupic/math/Utils.hpp
)


set(types_files
    nupic/types/Exception.hpp
    nupic/types/Fraction.cpp
    nupic/types/Fraction.hpp
    nupic/types/Serializable.hpp
    nupic/types/Types.hpp
    nupic/types/Serializable.hpp
)

set(utils_files
    nupic/utils/GroupBy.hpp
    nupic/utils/Log.hpp
    nupic/utils/LoggingException.cpp
    nupic/utils/LoggingException.hpp
    nupic/utils/LogItem.cpp
    nupic/utils/LogItem.hpp
    nupic/utils/MovingAverage.cpp
    nupic/utils/MovingAverage.hpp
    nupic/utils/Random.cpp
    nupic/utils/Random.hpp
    nupic/utils/SlidingWindow.hpp
    nupic/utils/StringUtils.cpp
    nupic/utils/StringUtils.hpp
    nupic/utils/VectorHelpers.hpp
    nupic/utils/Timer.cpp
    nupic/utils/Timer.hpp    
)

#set up file tabs in Visual Studio
source_group("algorithms" FILES ${algorithm_files})
source_group("encoders" FILES ${encoders_files})
source_group("math" FILES ${math_files})
source_group("types" FILES ${types_files})
source_group("utils" FILES ${utils_files})

########################################################
# Lib
add_library(${src_lib_static_htm} STATIC 
    ${algorithm_files} 
    ${encoders_files} 
    ${math_files} 
    ${types_files} 
    ${utils_files} 
)

# nupic.htm library sources depend on headers installed by these external projects
add_dependencies(${src_lib_static_htm}
        yaml-cpp
		)
target_compile_options( ${src_lib_static_htm} PUBLIC ${INTERNAL_CXX_FLAGS})
target_compile_definitions(${src_lib_static_htm} PRIVATE ${COMMON_COMPILER_DEFINITIONS})
target_include_directories(${src_lib_static_htm} PRIVATE 
		${HTM_LIB_INCLUDES} 
		${PROJECT_BINARY_DIR}  # for Version.hpp
		${EXTERNAL_INCLUDES}
		)

if(${NUPIC_IWYU})
  set_target_properties(${src_lib_static_htm}
    PROPERTIES CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path})
endif()
set(src_lib_static_htm ${src_lib_static_htm} PARENT_SCOPE)

#########################################################
# Test
# The tests were placed in a separate file to avoid clutering up this file.
add_subdirectory(test)


#########################################################
## Example custom executable
#
## Setup benchmark_hotgym
#
set(src_executable_hotgym benchmark_hotgym)
add_executable(${src_executable_hotgym} examples/Hotgym.cpp)
target_link_libraries(${src_executable_hotgym} ${src_lib_static_htm})
target_compile_options( ${src_executable_hotgym} PUBLIC ${INTERNAL_CXX_FLAGS}) #TODO how to use OPTIMIZED CXX flags? 
target_compile_definitions(${src_executable_hotgym} PRIVATE ${COMMON_COMPILER_DEFINITIONS})
target_include_directories(${src_executable_hotgym} PRIVATE 
		${HTM_LIB_INCLUDES}
		${NETAPI_LIB_INCLUDES}
		${EXTERNAL_INCLUDES}
		)
add_custom_target(hotgym
                  COMMAND ${src_executable_hotgym}
                  DEPENDS ${src_executable_hotgym}
                  COMMENT "Executing ${src_executable_hotgym}"
                  VERBATIM)


##################################################
#
# Install targets into CMAKE_INSTALL_PREFIX
#
install(TARGETS
        ${src_lib_static_htm}
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

          
