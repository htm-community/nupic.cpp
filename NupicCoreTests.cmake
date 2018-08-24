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
	   ${test_src}/unit/algorithms/BacktrackingTMCppTest.cpp
	   )
               

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
	   
set(engine_tests
	   ${test_src}/unit/engine/InputTest.cpp
	   ${test_src}/unit/engine/LinkTest.cpp
	   ${test_src}/unit/engine/NetworkTest.cpp
	   ${test_src}/unit/engine/YAMLUtilsTest.cpp
	   )
	   
set(regions_tests
	   ${test_src}/unit/region/regionTestUtilities.cpp
	   ${test_src}/unit/region/regionTestUtilities.hpp
	   ${test_src}/unit/region/SPRegionTest.cpp
	   ${test_src}/unit/region/TMRegionTest.cpp
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
#	${Boost_LIBRARIES}
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
                  
		  
		  
set (src_common_test_exe_libs
	${lib_name}
	${yaml-cpp_LIBNAME}
)

		  
# Run examples
set(HelloSP_TP_src ${CMAKE_SOURCE_DIR}/src/examples/algorithms/HelloSP_TP.cpp)

add_executable(HelloSP_TP ${HelloSP_TP_src})
target_link_libraries(HelloSP_TP PRIVATE
	${src_common_test_exe_libs}
	-OPT:NOREF
)
target_compile_definitions(HelloSP_TP PRIVATE ${COMMON_COMPILER_DEFINITIONS})
target_compile_options(HelloSP_TP PUBLIC "${INTERNAL_CXX_FLAGS}")
set_target_properties(HelloSP_TP PROPERTIES LINK_FLAGS "${INTERNAL_LINKER_FLAGS}")
add_dependencies(HelloSP_TP ${lib_name})

set(HelloRegions_src  ${CMAKE_SOURCE_DIR}/src/examples/regions/HelloRegions.cpp)
add_executable(HelloRegions ${HelloRegions_src})
target_link_libraries(HelloRegions PRIVATE
	${src_common_test_exe_libs}
	-OPT:NOREF
)
target_compile_definitions(HelloRegions PRIVATE ${COMMON_COMPILER_DEFINITIONS})
target_compile_options(HelloRegions PUBLIC "${INTERNAL_CXX_FLAGS}")
set_target_properties(HelloRegions PROPERTIES LINK_FLAGS "${INTERNAL_LINKER_FLAGS}")
add_dependencies(HelloRegions ${lib_name})



#
# Setup test_cpp_region
#
set(src_executable_cppregiontest cpp_region_test)
add_executable(${src_executable_cppregiontest} ${CMAKE_SOURCE_DIR}/src/test/integration/CppRegionTest.cpp)
target_link_libraries(${src_executable_cppregiontest} ${src_common_test_exe_libs})
set_target_properties(${src_executable_cppregiontest} PROPERTIES COMPILE_FLAGS "${INTERNAL_CXX_FLAGS}")
set_target_properties(${src_executable_cppregiontest} PROPERTIES LINK_FLAGS "${INTERNAL_LINKER_FLAGS}")
#add_custom_target(tests_cpp_region
#                  COMMAND ${src_executable_cppregiontest}
#                  DEPENDS ${src_executable_cppregiontest}
#                  COMMENT "Executing test ${src_executable_cppregiontest}"
#                  VERBATIM)

#
# Setup test_py_region
#
#set(src_executable_pyregiontest py_region_test)
#add_executable(${src_executable_pyregiontest} ${CMAKE_SOURCE_DIR}/src/test/integration/PyRegionTest.cpp)
#target_link_libraries(${src_executable_pyregiontest} ${src_common_test_exe_libs})
#set_target_properties(${src_executable_pyregiontest}
#                      PROPERTIES COMPILE_FLAGS ${src_compile_flags})
#set_target_properties(${src_executable_pyregiontest}
#                      PROPERTIES LINK_FLAGS "${INTERNAL_LINKER_FLAGS_OPTIMIZED}")
#add_custom_target(tests_py_region
#                  COMMAND ${src_executable_pyregiontest}
#                  DEPENDS ${src_executable_pyregiontest}
#                  COMMENT "Executing test ${src_executable_pyregiontest}"
#                  VERBATIM)


#
# Setup test_connections_performance
#
set(src_executable_connectionsperformancetest connections_performance_test)
add_executable(${src_executable_connectionsperformancetest}
               ${CMAKE_SOURCE_DIR}/src/test/integration/ConnectionsPerformanceTest.cpp)
target_link_libraries(${src_executable_connectionsperformancetest}
                      ${src_common_test_exe_libs})
set_target_properties(${src_executable_connectionsperformancetest}
                      PROPERTIES COMPILE_FLAGS "${INTERNAL_CXX_FLAGS}")
set_target_properties(${src_executable_connectionsperformancetest}
                      PROPERTIES LINK_FLAGS "${INTERNAL_LINKER_FLAGS_OPTIMIZED}")
add_custom_target(tests_connections_performance
                  COMMAND ${src_executable_connectionsperformancetest}
                  DEPENDS ${src_executable_connectionsperformancetest}
                  COMMENT "Executing test ${src_executable_connectionsperformancetest}"
                  VERBATIM)


#
# tests_all just calls other targets
#
# TODO This doesn't seem to have any effect; it's probably because the DEPENDS
# of add_custom_target must be files, not other high-level targets. If really
# need to run these tests during build, then either the individual
# add_custom_target of the individual test runners should be declared with the
# ALL option, or tests_all target whould be declared without DEPENDS, and
# add_dependencies should be used to set it's dependencies on the custom targets
# of the inidividual test runners.
add_custom_target(tests_all
#                  DEPENDS tests_cpp_region
                  DEPENDS tests_unit
                  COMMENT "Running all tests"
                  VERBATIM)
