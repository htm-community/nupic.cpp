# -----------------------------------------------------------------------------
# Numenta Platform for Intelligent Computing (NuPIC)
# Copyright (C) 2015-2018, Numenta, Inc.  Unless you have purchased from
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
# Minimal CMake configuration for nupic.cpp 
#
# To startup on Windows:
#    The scripts are found in external\Windows
#		startupVCpkg.bat  -- create nupic.base.sln using the vcpkg package manager
#       startupHunter.bat -- create nupic.base.sln using the Hunter package manager
#    You should only need to run one of these, once to create the Visual Studio solution file
#    then you can use that to start up Visual Studio and build everything. 
#
#    NOTE: with either package manager, The first time it will take a long time to 
#          download, build and install the depenancies (boost takes a long time).
#
#          If you should need to wipe out the build/ folder and start over with a fresh 
#          Visual Studio the package managers will verify all of the dependancies but 
#          they will not need to download and re-install them.
#
#
# If using vcpkg package manager:
#    Before running the startup script, you must download and install vcpkg
#    from https://github.com/Microsoft/vcpkg
#
#    startupVCpkg.bat  will download and install the dependancies using vcpkg:
#      vcpkg install  boost-system:x64-windows-static
#      vcpkg install  yaml-cpp:x64-windows-static
#      vcpkg install  boost-system:x64-windows-static
#      vcpkg install  gtest:x64-windows-static
# 
#    the script will then specify the following to cmake:
#      cmake -A x64 ^
#            -DVCPKG_TOOLCHAIN_FILE=c:/vcpkg/scripts/buildsystems/vcpkg.cmake ^
#            -DVCPKG_TARGET_TRIPLET=x64-windows-static ^
#            ...
#
# If using Hunter package manager:
#     Hunter package manger does not need to be installed prior to running.
#     It will install itself the first time the startup script is ran.
#     By default it will install itself at C:/.hunter
#
#     startupHunter.bat will download and install the dependancies using Hunter.
#     The scropt will start cmake with the following:
#         cmake -G "Visual Studio 15 2017 Win64" ...
#     Inside cmake, Hunter will install itself and all of the dependancies
#     and then complete the installation of the nupic.base.sln solution file.
#

cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
set(CMAKE_VERBOSE_MAKEFILE ON)

if (NOT VCPKG_TARGET_TRIPLET)
	include("cmake/HunterGate.cmake")
	option(HUNTER_ENABLED "Enable Hunter package manager. If OFF no installs will occur." ON)
	option(HUNTER_STATUS_DEBUG "Print info to help debug cmake scrips." ON)
	HunterGate(
		URL "https://github.com/ruslo/hunter/archive/v0.20.24.tar.gz"
		SHA1 "3e2037a462bcf2ec3440f60298d933e74ffd4df3"
	)
endif()

set(project_name nupic.base)
project(${project_name} LANGUAGES CXX)

# if install directory was not specified on the command line, default it to ./build/bin
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/bin" CACHE PATH "default install path" FORCE )
endif()

##########################################
# Find the dependencies
##########################################

if (VCPKG_TARGET_TRIPLET)
	#################### Using vcpkg Package Manager ###############
	message(STATUS "CMAKE_TOOLCHAIN_FILE= ${CMAKE_TOOLCHAIN_FILE}")
	message(STATUS "VCPKG_TARGET_TRIPLET= ${VCPKG_TARGET_TRIPLET}")

	# Boost 
	find_package(Boost  1.67.0 REQUIRED COMPONENTS system)
	include_directories(${Boost_INCLUDE_DIRS})
	message(STATUS "Boost_FOUND=${Boost_FOUND}")
	set(Boost_LIBNAME ${BoostLIBRARIES})

	# yaml-cpp  (include only package)
	find_package(yaml-cpp CONFIG REQUIRED)
	message(STATUS "yaml-cpp_FOUND=${yaml-cpp_FOUND}")
	set(yaml-cpp_LIBNAME yaml-cpp)

	# GTest: 
	find_package(GTest REQUIRED)
	message(STATUS "GTest_FOUND=${GTest_FOUND}")
	set(GTest_LIBNAME GTest::GTest GTest::Main)

else()
	#################### Using Hunter Package Manager ###############
	# Boost 
	hunter_add_package(Boost COMPONENTS system)
	find_package(Boost CONFIG REQUIRED system)
	include_directories(${Boost_INCLUDE_DIRS})
	message(STATUS "Boost_INCLUDE_DIRS = ${Boost_INCLUDE_DIRS}")
	set(Boost_LIBNAME Boost::system)

	# yaml-cpps
	hunter_add_package(yaml-cpp)
	find_package(yaml-cpp CONFIG REQUIRED)
	message(STATUS "yaml-cpp_FOUND=${yaml-cpp_FOUND}")
	set(yaml-cpp_LIBNAME yaml-cpp::yaml-cpp)


	# GTest: link with GTest::main and GTest::gtest will be linked too
	hunter_add_package(GTest)
	find_package(GTest CONFIG REQUIRED)
	include_directories(${GTEST_ROOT}/include)
	message(STATUS "GTest_FOUND=${GTest_FOUND}")
	set(GTest_LIBNAME GTest::main)

endif()

##########################################
# Determine common toolchain settings, compiler and link flags
##########################################
include(CommonCompilerConfig.cmake)


message(STATUS "PROJECT_BINARY_DIR          = ${PROJECT_BINARY_DIR}")
message(STATUS "CMAKE_INSTALL_PREFIX        = ${CMAKE_INSTALL_PREFIX}")



###################
# nupic static lib
###################
message(STATUS "Configuring library")

set(lib_name ${project_name})
set(nupic_src ${CMAKE_SOURCE_DIR}/src/nupic)

set(algorithm_files
    ${nupic_src}/algorithms/Anomaly.cpp
    ${nupic_src}/algorithms/Anomaly.hpp
    ${nupic_src}/algorithms/ArrayBuffer.hpp
    ${nupic_src}/algorithms/BitHistory.cpp
    ${nupic_src}/algorithms/BitHistory.hpp
    ${nupic_src}/algorithms/Cell.cpp
    ${nupic_src}/algorithms/Cell.hpp
    ${nupic_src}/algorithms/Cells4.cpp
    ${nupic_src}/algorithms/Cells4.hpp
    ${nupic_src}/algorithms/ClassifierResult.cpp
    ${nupic_src}/algorithms/ClassifierResult.hpp
    ${nupic_src}/algorithms/CondProbTable.cpp
    ${nupic_src}/algorithms/CondProbTable.hpp
    ${nupic_src}/algorithms/Connections.cpp
    ${nupic_src}/algorithms/Connections.hpp
    ${nupic_src}/algorithms/GaborNode.cpp
    ${nupic_src}/algorithms/GaborNode.hpp
    ${nupic_src}/algorithms/ImageSensorLite.cpp
    ${nupic_src}/algorithms/ImageSensorLite.hpp
    ${nupic_src}/algorithms/InSynapse.cpp
    ${nupic_src}/algorithms/InSynapse.hpp
    ${nupic_src}/algorithms/OutSynapse.cpp
    ${nupic_src}/algorithms/OutSynapse.hpp
    ${nupic_src}/algorithms/Scanning.hpp
    ${nupic_src}/algorithms/SDRClassifier.cpp
    ${nupic_src}/algorithms/SDRClassifier.hpp
    ${nupic_src}/algorithms/Segment.cpp
    ${nupic_src}/algorithms/Segment.hpp
    ${nupic_src}/algorithms/SegmentUpdate.cpp
    ${nupic_src}/algorithms/SegmentUpdate.hpp
    ${nupic_src}/algorithms/SpatialPooler.cpp
    ${nupic_src}/algorithms/SpatialPooler.hpp
    ${nupic_src}/algorithms/Svm.cpp
    ${nupic_src}/algorithms/Svm.hpp
    ${nupic_src}/algorithms/SvmT.hpp
    ${nupic_src}/algorithms/TemporalMemory.cpp
    ${nupic_src}/algorithms/TemporalMemory.hpp
    ${nupic_src}/algorithms/BacktrackingTMCpp.cpp
    ${nupic_src}/algorithms/BacktrackingTMCpp.hpp
)

set(encoders_files 
    ${nupic_src}/encoders/ScalarEncoder.cpp
    ${nupic_src}/encoders/ScalarEncoder.hpp
    ${nupic_src}/encoders/ScalarSensor.cpp
    ${nupic_src}/encoders/ScalarSensor.hpp
)

set(engine_files
	${nupic_src}/engine/Collections.cpp
    ${nupic_src}/engine/Input.cpp
    ${nupic_src}/engine/Input.hpp
    ${nupic_src}/engine/Link.cpp
    ${nupic_src}/engine/Link.hpp
    ${nupic_src}/engine/Network.cpp
    ${nupic_src}/engine/Network.hpp
    ${nupic_src}/engine/NuPIC.cpp
    ${nupic_src}/engine/NuPIC.hpp
    ${nupic_src}/engine/Output.cpp
    ${nupic_src}/engine/Output.hpp
    ${nupic_src}/engine/Region.cpp
    ${nupic_src}/engine/Region.hpp
    ${nupic_src}/engine/RegionImpl.cpp
    ${nupic_src}/engine/RegionImpl.hpp
    ${nupic_src}/engine/RegionImplFactory.cpp
    ${nupic_src}/engine/RegionImplFactory.hpp
    ${nupic_src}/engine/RegionIo.cpp
    ${nupic_src}/engine/RegionParameters.cpp
    ${nupic_src}/engine/RegisteredRegionImpl.hpp
    ${nupic_src}/engine/Spec.cpp
    ${nupic_src}/engine/Spec.hpp
    ${nupic_src}/engine/TestNode.cpp
    ${nupic_src}/engine/TestNode.hpp
    ${nupic_src}/engine/YAMLUtils.cpp
    ${nupic_src}/engine/YAMLUtils.hpp
)

set(math_files
    ${nupic_src}/math/Array2D.hpp
    ${nupic_src}/math/ArrayAlgo.hpp
    ${nupic_src}/math/Convolution.hpp
    ${nupic_src}/math/DenseMatrix.hpp
    ${nupic_src}/math/Domain.hpp
    ${nupic_src}/math/Erosion.hpp
    ${nupic_src}/math/Functions.hpp
    ${nupic_src}/math/GraphAlgorithms.hpp
    ${nupic_src}/math/Index.hpp
    ${nupic_src}/math/Math.hpp
    ${nupic_src}/math/NearestNeighbor.hpp
    ${nupic_src}/math/Rotation.hpp
    ${nupic_src}/math/SegmentMatrixAdapter.hpp
    ${nupic_src}/math/Set.hpp
    ${nupic_src}/math/SparseBinaryMatrix.hpp
    ${nupic_src}/math/SparseMatrix.hpp
    ${nupic_src}/math/SparseMatrix01.hpp
    ${nupic_src}/math/SparseMatrixAlgorithms.cpp
    ${nupic_src}/math/SparseMatrixAlgorithms.hpp
    ${nupic_src}/math/SparseMatrixConnections.cpp
    ${nupic_src}/math/SparseMatrixConnections.hpp
    ${nupic_src}/math/SparseRLEMatrix.hpp
    ${nupic_src}/math/SparseTensor.hpp
    ${nupic_src}/math/StlIo.cpp
    ${nupic_src}/math/StlIo.hpp
    ${nupic_src}/math/Topology.cpp
    ${nupic_src}/math/Topology.hpp
    ${nupic_src}/math/Types.hpp
    ${nupic_src}/math/Utils.hpp
)

set(ntypes_files
    ${nupic_src}/ntypes/Array.hpp
    ${nupic_src}/ntypes/ArrayBase.cpp
    ${nupic_src}/ntypes/ArrayBase.hpp
    ${nupic_src}/ntypes/ArrayRef.hpp
    ${nupic_src}/ntypes/Buffer.cpp
    ${nupic_src}/ntypes/Buffer.hpp
    ${nupic_src}/ntypes/BundleIO.cpp
    ${nupic_src}/ntypes/BundleIO.hpp
    ${nupic_src}/ntypes/Collection.cpp
    ${nupic_src}/ntypes/Collection.hpp
    ${nupic_src}/ntypes/MemParser.cpp
    ${nupic_src}/ntypes/MemParser.hpp
    ${nupic_src}/ntypes/MemStream.hpp
    ${nupic_src}/ntypes/ObjectModel.h
    ${nupic_src}/ntypes/ObjectModel.hpp
    ${nupic_src}/ntypes/Scalar.cpp
    ${nupic_src}/ntypes/Scalar.hpp
    ${nupic_src}/ntypes/Value.cpp
    ${nupic_src}/ntypes/Value.hpp
)

set(os_files
    ${nupic_src}/os/Directory.cpp
    ${nupic_src}/os/Directory.hpp
    ${nupic_src}/os/Env.cpp
    ${nupic_src}/os/Env.hpp
    ${nupic_src}/os/FStream.cpp
    ${nupic_src}/os/FStream.hpp
    ${nupic_src}/os/OS.cpp
    ${nupic_src}/os/OS.hpp
    ${nupic_src}/os/OSUnix.cpp
    ${nupic_src}/os/OSWin.cpp
    ${nupic_src}/os/Path.cpp
    ${nupic_src}/os/Path.hpp
    ${nupic_src}/os/Regex.cpp
    ${nupic_src}/os/Regex.hpp
    ${nupic_src}/os/Timer.cpp
    ${nupic_src}/os/Timer.hpp    
)

set(regions_files
    ${nupic_src}/regions/VectorFile.cpp
    ${nupic_src}/regions/VectorFile.hpp
    ${nupic_src}/regions/VectorFileEffector.cpp
    ${nupic_src}/regions/VectorFileEffector.hpp
    ${nupic_src}/regions/VectorFileSensor.cpp
    ${nupic_src}/regions/VectorFileSensor.hpp   
    ${nupic_src}/regions/SPRegion.cpp 
    ${nupic_src}/regions/SPRegion.hpp 
    ${nupic_src}/regions/TMRegion.cpp 
    ${nupic_src}/regions/TMRegion.hpp 
)

set(types_files
    ${nupic_src}/types/BasicType.cpp
    ${nupic_src}/types/BasicType.hpp
    ${nupic_src}/types/Exception.hpp
    ${nupic_src}/types/Fraction.cpp
    ${nupic_src}/types/Fraction.hpp
    ${nupic_src}/types/ptr_types.hpp
    ${nupic_src}/types/Types.h
    ${nupic_src}/types/Types.hpp
)

set(utils_files
    ${nupic_src}/utils/GroupBy.hpp
    ${nupic_src}/utils/Log.hpp
    ${nupic_src}/utils/LoggingException.cpp
    ${nupic_src}/utils/LoggingException.hpp
    ${nupic_src}/utils/LogItem.cpp
    ${nupic_src}/utils/LogItem.hpp
    ${nupic_src}/utils/MovingAverage.cpp
    ${nupic_src}/utils/MovingAverage.hpp
    ${nupic_src}/utils/Random.cpp
    ${nupic_src}/utils/Random.hpp
    ${nupic_src}/utils/StringUtils.cpp
    ${nupic_src}/utils/StringUtils.hpp
    ${nupic_src}/utils/TRandom.cpp
    ${nupic_src}/utils/TRandom.hpp
    ${nupic_src}/utils/Watcher.cpp
    ${nupic_src}/utils/Watcher.hpp
)

#set up file tabs in Visual Studio
source_group("algorithms" FILES ${algorithm_files})
source_group("encoders" FILES ${encoders_files})
source_group("engine" FILES ${engine_files})
source_group("math" FILES ${math_files})
source_group("ntypes" FILES ${ntypes_files})
source_group("os" FILES ${os_files})
source_group("regions" FILES ${regions_files})
source_group("types" FILES ${types_files})
source_group("utils" FILES ${utils_files})

########################################################
# Lib
add_library(${lib_name} STATIC 
    ${algorithm_files} 
    ${encoders_files} 
    ${engine_files} 
    ${math_files} 
    ${ntypes_files} 
    ${os_files} 
    ${regions_files} 
    ${types_files} 
    ${utils_files} 
)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_definitions(${lib_name} PRIVATE ${COMMON_COMPILER_DEFINITIONS})
target_compile_options(${lib_name} PUBLIC ${INTERNAL_CXX_FLAGS})


#########################################################
# Test
# The tests were placed in a separate file to avoid clutering up this file.
include(NupicCoreTests.cmake)


##########################################################
#
# Install core library into CMAKE_INSTALL_PREFIX
#
install(TARGETS
        ${lib_name} ${test_app}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/nupic DESTINATION include
	  MESSAGE_NEVER
	  FILES_MATCHING PATTERN "*.h*"
	  PATTERN "*.hpp.in" EXCLUDE)
	  
        
#
# `make package` results in
# nupic_core-${NUPIC_CORE_VERSION}-${PLATFORM}${BITNESS}${PLATFORM_SUFFIX}.tar.gz binary release
#
set(CPACK_GENERATOR "TGZ")
set(CPACK_PACKAGE_FILE_NAME "${project_name}-${NUPIC_CORE_VERSION}-${PLATFORM}${BITNESS}${PLATFORM_SUFFIX}")
include(CPack)

##########################################################
#  Interfaces
#
# add_subdirectory(src/python)        #Runs python interface build
# add_subdirectory(src/csharp)        #Runs CSharp interface build
