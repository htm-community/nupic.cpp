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

cmake_minimum_required(VERSION 3.7)
project(nupic_core CXX)

message(STATUS "Configuring nupic_htm tests")
###############################################################
###                   TESTS                      ##############
###############################################################
#
# Build TESTS of the nupic htm static library
#   ${core_library}  references the nupic_core  static library which includes depencancy libraries
#

set(CMAKE_VERBOSE_MAKEFILE ON) # toggle for cmake debug 

message(STATUS "htm tests NETAPI_LIB_INCLUDES=${NETAPI_LIB_INCLUDES}")
message(STATUS "htm tests HTM_LIB_INCLUDES=${HTM_LIB_INCLUDES}")

#  Build unit_tests
set(htm_tests_executable htm_tests)

set(algorithm_tests
	   algorithms/AnomalyTest.cpp
	   algorithms/Cells4Test.cpp
	   algorithms/ConnectionsPerformanceTest.cpp
	   algorithms/ConnectionsTest.cpp
	   algorithms/HelloSPTPTest.cpp
	   algorithms/SDRClassifierTest.cpp
	   algorithms/SegmentTest.cpp
	   algorithms/SpatialPoolerTest.cpp
	   algorithms/TemporalMemoryTest.cpp
	   )
               
set(encoders_tests
           encoders/ScalarEncoderTest.cpp
           )

set(math_tests
	   math/DenseTensorUnitTest.cpp
	   math/DenseTensorUnitTest.hpp
	   math/DomainUnitTest.cpp
	   math/DomainUnitTest.hpp
	   math/IndexUnitTest.cpp
	   math/IndexUnitTest.hpp
	   math/MathsTest.cpp
	   math/MathsTest.hpp
	   math/SparseBinaryMatrixTest.cpp
	   math/SparseMatrixTest.cpp
	   math/SparseMatrixUnitTest.cpp
	   math/SparseMatrixUnitTest.hpp
	   math/SparseTensorUnitTest.cpp
	   math/SparseTensorUnitTest.hpp
	   math/TopologyTest.cpp
	   )
	   
set(types_tests
	   types/ExceptionTest.cpp
	   types/FractionTest.cpp
	   types/SdrTest.cpp
	   )
	   
set(utils_tests
	   utils/GroupByTest.cpp
	   utils/MovingAverageTest.cpp
	   utils/RandomTest.cpp
	   utils/TimerTest.cpp
	   utils/VectorHelpersTest.cpp
	   )
	   

	   
#set up file tabs in Visual Studio
source_group("algorithm" FILES ${algorithm_tests})
source_group("encoders" FILES ${encoders_tests})
source_group("math" FILES ${math_tests})
source_group("types" FILES ${types_tests})
source_group("utils" FILES ${utils_tests})


set(htm_tests_src
    UnitTestMain.cpp
    ${algorithm_tests} 
    ${encoders_tests} 
    ${math_tests} 
    ${types_tests} 
    ${utils_tests} 
)

add_executable(${htm_tests_executable} ${htm_tests_src})
target_link_libraries(${htm_tests_executable} 
    ${core_library}
    ${gtest_LIBRARIES}
    ${COMMON_OS_LIBS}
)
target_include_directories(${htm_tests_executable} PRIVATE 
	${gtest_INCLUDE_DIR}
	${HTM_LIB_INCLUDES}
	${NETAPI_LIB_INCLUDES}
	${EXTERNAL_INCLUDES})
target_compile_definitions(${htm_tests_executable} PRIVATE ${COMMON_COMPILER_DEFINITIONS})
target_compile_options(${htm_tests_executable} PUBLIC ${INTERNAL_CXX_FLAGS})
set_target_properties(${htm_tests_executable} PROPERTIES LINK_FLAGS "${INTERNAL_LINKER_FLAGS_STR}")
add_dependencies(${htm_tests_executable} ${core_library} gtest)



add_custom_target(htm_tests_run_with_output
                  COMMAND ${htm_tests_executable}
                  DEPENDS ${htm_tests_executable}
                  COMMENT "Executing test ${htm_tests_executable}"
                  VERBATIM)
                  

		  
install(TARGETS
        ${htm_tests_executable}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

