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

message(STATUS "Configuring nupic_core tests")
###############################################################
###                   TESTS                      ##############
###############################################################
#
# Build TESTS of the nupic core static library
#   ${core_library}  references the nupic_core  static library which includes depencancy libraries
#

set(CMAKE_VERBOSE_MAKEFILE ON) # toggle for cmake debug 

message(STATUS "netapi tests NETAPI_LIB_INCLUDES=${NETAPI_LIB_INCLUDES}")
message(STATUS "netapi tests HTM_LIB_INCLUDES=${HTM_LIB_INCLUDES}")


#  Build unit_tests
set(netapi_tests_executable netapi_test)

	   
set(engine_tests
	   engine/InputTest.cpp
	   engine/LinkTest.cpp
	   engine/NetworkTest.cpp
           engine/UniformLinkPolicyTest.cpp
	   engine/YAMLUtilsTest.cpp
	   engine/WatcherTest.cpp
	   )
	   
set(ntypes_tests
	   ntypes/ArrayTest.cpp
	   ntypes/BufferTest.cpp
	   ntypes/CollectionTest.cpp
	   ntypes/DimensionsTest.cpp
	   ntypes/MemParserTest.cpp
	   ntypes/MemStreamTest.cpp
	   ntypes/NodeSetTest.cpp
	   ntypes/ScalarTest.cpp
	   ntypes/ValueTest.cpp
	   ntypes/BasicTypeTest.cpp
	   )
	   
set(os_tests
	   os/DirectoryTest.cpp
	   os/EnvTest.cpp
	   os/OSTest.cpp
	   os/PathTest.cpp
	   os/RegexTest.cpp
	   )
set(region_tests
	   regions/CppRegionTest.cpp
	   regions/HelloRegionTest.cpp
	   )
	   
	   

	   
#set up file tabs in Visual Studio
source_group("engine" FILES ${engine_tests})
source_group("ntypes" FILES ${ntypes_tests})
source_group("os" FILES ${os_tests})


set(src_executable_gtests
    UnitTestMain.cpp
    ${engine_tests} 
    ${ntypes_tests} 
    ${os_tests} 
    ${region_tests}
)

add_executable(${netapi_tests_executable} ${src_executable_gtests})
target_link_libraries(${netapi_tests_executable} 
    ${core_library}
    ${gtest_LIBRARIES}
    ${COMMON_OS_LIBS}
)
target_include_directories(${netapi_tests_executable} PRIVATE 
	${gtest_INCLUDE_DIR}
	${HTM_LIB_INCLUDES}
	${NETAPI_LIB_INCLUDES}
	${EXTERNAL_INCLUDES})
target_compile_definitions(${netapi_tests_executable} PRIVATE ${COMMON_COMPILER_DEFINITIONS})
target_compile_options(${netapi_tests_executable} PUBLIC ${INTERNAL_CXX_FLAGS})
set_target_properties(${netapi_tests_executable} PROPERTIES LINK_FLAGS "${INTERNAL_LINKER_FLAGS_STR}")
add_dependencies(${netapi_tests_executable} ${core_library} gtest)

# Create the RUN_TESTS target
enable_testing()
add_test(NAME ${netapi_tests_executable} COMMAND ${netapi_tests_executable})

add_custom_target(netapi_tests_run_with_output
                  COMMAND ${netapi_tests_executable}
                  DEPENDS ${netapi_tests_executable}
                  COMMENT "Executing test ${netapi_tests_executable}"
                  VERBATIM)
                  
		  
install(TARGETS
        ${netapi_tests_executable}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

