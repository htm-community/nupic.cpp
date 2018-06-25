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


###############################################################
######################                   TESTS                      ######################
###############################################################
#
# Build TESTS of the nupic core static library
#   ${core_library}  references the nupic_core_cpp static library which includes depencancy libraries
#

set(CMAKE_VERBOSE_MAKEFILE ON) # toggle for cmake debug 
                                 

set(unit_tests_executable unit_tests)

set(test_src ${CMAKE_SOURCE_DIR}/src/test)

set(math_tests
	   ${test_src}/unit/math/DenseTensorUnitTest.cpp
	   ${test_src}/unit/math/DomainUnitTest.cpp
	   ${test_src}/unit/math/IndexUnitTest.cpp
	   ${test_src}/unit/math/MathsTest.cpp
	   ${test_src}/unit/math/SegmentMatrixAdapterTest.cpp
	   ${test_src}/unit/math/SparseBinaryMatrixTest.cpp
	   ${test_src}/unit/math/SparseMatrix01UnitTest.cpp
	   ${test_src}/unit/math/SparseMatrixTest.cpp
	   ${test_src}/unit/math/SparseMatrixUnitTest.cpp
	   ${test_src}/unit/math/SparseTensorUnitTest.cpp
	   ${test_src}/unit/math/TopologyTest.cpp
	   )
	   
set(ntypes_tests
	   ${test_src}/unit/ntypes/ArrayTest.cpp
	   ${test_src}/unit/ntypes/BufferTest.cpp
	   ${test_src}/unit/ntypes/CollectionTest.cpp
	   ${test_src}/unit/ntypes/MemParserTest.cpp
	   ${test_src}/unit/ntypes/MemStreamTest.cpp
	   ${test_src}/unit/ntypes/ScalarTest.cpp
	   ${test_src}/unit/ntypes/ValueTest.cpp
	   )
	   
set(os_tests
	   ${test_src}/unit/os/DirectoryTest.cpp
	   ${test_src}/unit/os/EnvTest.cpp
	   ${test_src}/unit/os/OSTest.cpp
	   ${test_src}/unit/os/PathTest.cpp
	   ${test_src}/unit/os/RegexTest.cpp
	   ${test_src}/unit/os/TimerTest.cpp
	   )
	   
set(types_tests
	   ${test_src}/unit/types/BasicTypeTest.cpp
	   ${test_src}/unit/types/ExceptionTest.cpp
	   ${test_src}/unit/types/FractionTest.cpp
	   )
	   
set(utils_tests
	   ${test_src}/unit/utils/GroupByTest.cpp
	   ${test_src}/unit/utils/MovingAverageTest.cpp
	   ${test_src}/unit/utils/RandomTest.cpp
	   ${test_src}/unit/utils/WatcherTest.cpp
	   )
	   
set(algorithm_tests
	   ${test_src}/unit/algorithms/AnomalyTest.cpp
	   ${test_src}/unit/algorithms/Cells4Test.cpp
	   ${test_src}/unit/algorithms/CondProbTableTest.cpp
	   ${test_src}/unit/algorithms/ConnectionsTest.cpp
	   ${test_src}/unit/algorithms/NearestNeighborUnitTest.cpp
	   ${test_src}/unit/algorithms/SDRClassifierTest.cpp
	   ${test_src}/unit/algorithms/SegmentTest.cpp
	   ${test_src}/unit/algorithms/SpatialPoolerTest.cpp
	   ${test_src}/unit/algorithms/SvmTest.cpp
	   ${test_src}/unit/algorithms/TemporalMemoryTest.cpp
	   )
               
set(engine_tests
	   ${test_src}/unit/engine/InputTest.cpp
	   ${test_src}/unit/engine/LinkTest.cpp
	   ${test_src}/unit/engine/NetworkTest.cpp
	   ${test_src}/unit/engine/YAMLUtilsTest.cpp
	   )
	   
set(regions_tests
	   ${test_src}/unit/region/SPRegionTest.cpp
	   )
	   
set(encoders_test
	   ${test_src}/unit/encoders/ScalarEncoderTest.cpp
	   )
	   
#set up file tabs in Visual Studio
source_group("algorithm" FILES ${algorithm_tests})
source_group("encoders" FILES ${encoders_tests})
source_group("engine" FILES ${engine_tests})
source_group("math" FILES ${math_tests})
source_group("ntypes" FILES ${ntypes_tests})
source_group("os" FILES ${os_tests})
source_group("regions" FILES ${regions_tests})
source_group("types" FILES ${types_tests})
source_group("utils" FILES ${utils_tests})


set(src_executable_gtests
    ${algorithm_tests} 
    ${encoders_tests} 
    ${engine_tests} 
    ${math_tests} 
    ${ntypes_tests} 
    ${os_tests} 
    ${regions_tests} 
    ${types_tests} 
    ${utils_tests} 
)

add_executable(${unit_tests_executable} ${src_executable_gtests})
target_link_libraries(${unit_tests_executable} PRIVATE
	${lib_name}
	${Boost_LIBRARIES}
    ${GTest_LIBNAME}
    ${yaml-cpp_LIBNAME}
    -OPT:NOREF
)
target_compile_definitions(${unit_tests_executable} PRIVATE ${COMMON_COMPILER_DEFINITIONS})
target_compile_options(${unit_tests_executable} PUBLIC "${INTERNAL_CXX_FLAGS}")
set_target_properties(${unit_tests_executable} PROPERTIES LINK_FLAGS "${INTERNAL_LINKER_FLAGS}")
add_dependencies(${unit_tests_executable} ${lib_name})

# Create the RUN_TESTS target
enable_testing()
add_test(NAME ${unit_tests_executable} COMMAND ${unit_tests_executable})

add_custom_target(unit_tests_run_with_output
                  COMMAND ${unit_tests_executable}
                  DEPENDS ${unit_tests_executable}
                  COMMENT "Executing test ${unit_tests_executable}"
                  VERBATIM)