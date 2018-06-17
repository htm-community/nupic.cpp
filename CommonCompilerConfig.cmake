# -----------------------------------------------------------------------------
# Numenta Platform for Intelligent Computing (NuPIC)
# Copyright (C) 2013-2016, Numenta, Inc.  Unless you have purchased from
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


# Configures common compiler/linker/loader settings for internal sources.
#
# NOTE SETTINGS THAT ARE SPECIFIC TO THIS OR THAT MODULE DO NOT BELONG HERE.

# OUTPUTS:
# COMMON_COMPILER_DEFINITIONS: list of -D define flags for the compilation of  source files; 
#
# INTERNAL_CXX_FLAGS: string of C++ flags common to both release and debug.  They do contain 'generator' statements.
#                     so make sure the CMake function you use will process them.
#
# INTERNAL_LINKER_FLAGS: string of linker flags for linking internal executables
#                      and shared libraries (DLLs) with optimizations that are
#                      compatible with INTERNAL_CXX_FLAGS
#
# src_common_os_libs: the list of common runtime libraries to use for this OS.
#
# CMAKE_AR: Name of archiving tool (ar) for static libraries. See cmake documentation
#
# CMAKE_RANLIB: Name of randomizing tool (ranlib) for static libraries. See cmake documentation
#
# CMAKE_LINKER: updated, if needed; use ld.gold if available. See cmake
#               documentation
#
#
# USAGE:
# Recommended, do this for each target foo
#   	target_compile_options(foo PUBLIC "${INTERNAL_CXX_FLAGS}")
#   	target_compile_definitions(foo PRIVATE ${COMMON_COMPILER_DEFINITIONS})
#   	set_target_properties(foo PROPERTIES LINK_FLAGS ${INTERNAL_LINKER_FLAGS})
# Add any module specific options such as /DLL, etc.

######################################################

include(CheckCXXCompilerFlag)

# Set the C++ Version
#message("!REQUIRED! -- Supported features = ${cxx_std_14}")
#message("Supported features = ${cxx_std_17}")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


# Check memory limits (in megabytes)
if(CMAKE_MAJOR_VERSION GREATER 2)
  cmake_host_system_information(RESULT available_physical_memory QUERY AVAILABLE_PHYSICAL_MEMORY)
  cmake_host_system_information(RESULT available_virtual_memory QUERY AVAILABLE_VIRTUAL_MEMORY)
  math(EXPR available_memory "${available_physical_memory}+${available_virtual_memory}")
  message(STATUS "CMAKE MEMORY=${available_memory}")
  # Python bindings requires more than
  # 1GB of memory for compiling with GCC. Send a warning if available memory
  # (physical plus virtual(swap)) is less than 1GB
  if(${available_memory} LESS 1024)
    message(WARNING "Less than 1GB of memory available, compilation may run out of memory!")
  endif()
endif()


# Identify platform "bitness".
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(BITNESS 64)
else()
  set(BITNESS 32)
endif()
message(STATUS "CMAKE BITNESS=${BITNESS}")

# Identify platform name.
string(TOLOWER ${CMAKE_SYSTEM_NAME} PLATFORM)
# Define a platform suffix, eg ${PLATFORM}${BITNESS}${PLATFORM_SUFFIX}
if (MSYS OR MINGW)
  set(PLATFORM_SUFFIX -gcc)
endif()
if(NOT DEFINED PLATFORM)
    message(FATAL_ERROR "PLATFORM property not defined: PLATFORM=${CMAKE_SYSTEM_NAME}")
endif()


#
# Set linker (ld)
# use ld.gold if available
#
execute_process(COMMAND ld.gold --version RESULT_VARIABLE EXIT_CODE)
if(EXIT_CODE EQUAL 0)
  message("Using ld.gold as LINKER.")
  set(CMAKE_LINKER "ld.gold"  PARENT_SCOPE)
endif()

#################################################


# Init exported properties
set(INTERNAL_CXX_FLAGS)
set(INTERNAL_LINKER_FLAGS)
set(COMMON_COMPILER_DEFINITIONS)
set(src_common_os_libs)


if(MSVC)
	# on Windows using Visual Studio 2015, 2017   https://docs.microsoft.com/en-us/cpp/build/reference/compiler-options-listed-by-category
	# Release Compiler flags:
	#	Common Stuff:  /permissive- /W3 /Gy /Gm- /O2 /Oi /MD /EHsc /FC /nologo
	#      Release Only:    /O2 /Oi /Gy  /MD
	#      Debug Only:       /Od /Zi /sdl /RTC1 /MDd
	set(INTERNAL_CXX_FLAGS /permissive- /W3 /Gm- /EHsc /FC 
							$<$<CONFIG:RELEASE>:/O2 /Oi /Gy  /GL /MD> 
							$<$<CONFIG:DEBUG>:/Ob0 /Od /Zi /sdl /RTC1 /MDd>)
	
	set(COMMON_COMPILER_DEFINITIONS 	/D "_CONSOLE"
		/D "_MBCS"
		/D "NTA_OS_WINDOWS"
		/D "NTA_COMPILER_MSVC"
		/D "NTA_VS_2017"
		/D "_CRT_SECURE_NO_WARNINGS"
		/D "_SCL_SECURE_NO_WARNINGS"
		/D "_CRT_NONSTDC_NO_DEPRECATE"
		/D "_SCL_SECURE_NO_DEPRECATE"
		/D "BOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE"
		/D "BOOST_ALL_NO_LIB"
		/D "VC_EXTRALEAN"
		/D "NOMINMAX"
		/D "NOGDI"
		)
	# Note: /DEBUG or /NDEBUG are added by default.
		
	# linker flags
	if("${BITNESS}" STREQUAL "32")
		set(machine "/MACHINE:X86")
	else()
		set(machine "/MACHINE:X${BITNESS}")
	endif()
	set(INTERNAL_LINKER_FLAGS "${machine} /NOLOGO /LTCG")
	# default flags: /MANIFEST /DYNAMICBASE /NXCOMPAT  /OPT:REF(for non-debug builds)  /OPT:ICF /INCREMENTAL:NO /ERRORREPORT:PROMPT  
	#                        /PGD  /SUBSYSTEM /TLBID:1 /MANIFESTUAC:"level='asInvoker' uiAccess='false'" 
	
	# OS libraries
	set(src_common_os_libs  "kernel32.lib" 
							"user32.lib" 
							"gdi32.lib" 
							"winspool.lib" 
							"comdlg32.lib" 
							"advapi32.lib" 
							"shell32.lib" 
							"ole32.lib" 
							"oleaut32.lib" 
							"uuid.lib" 
							"odbc32.lib" 
							"odbccp32.lib"
							"Psapi.lib"
							# oldnames.lib psapi.lib ws2_32.lib rpcrt4.lib
	)

else()

	# Compiler `-D*` definitions
	if(UNIX OR MSYS OR MINGW) #  UNIX like (i.e. APPLE and CYGWIN) and Unix on Windows variants.
	  set(COMMON_COMPILER_DEFINITIONS
		  ${COMMON_COMPILER_DEFINITIONS}
		  -DHAVE_UNISTD_H)
	endif()
	if(MSYS OR MINGW)
	  set(COMMON_COMPILER_DEFINITIONS
		  ${COMMON_COMPILER_DEFINITIONS}
		  -DPSAPI_VERSION=1
		  -DWIN32
		  -D_WINDOWS
		  -D_MBCS
		  -D_CRT_SECURE_NO_WARNINGS
		  -D_SCL_SECURE_NO_WARNINGS
		  -DNDEBUG
		  -D_VARIADIC_MAX=10
		  -DNOMINMAX)
	endif()

	#
	# Determine stdlib settings
	#
	set(stdlib_cxx "")
	set(stdlib_common "")

	if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	  set(stdlib_cxx "${stdlib_cxx} -stdlib=libc++")
	endif()

	if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
	  if ("${PLATFORM}" STREQUAL "linux")
		# NOTE When building manylinux python extensions, we want the static
		# libstdc++ due to differences in c++ ABI between the older toolchain in the
		# manylinux Docker image and libstdc++ in newer linux distros that is
		# compiled with the c++11 ABI. for example, with shared libstdc++, the
		# manylinux-built extension is unable to catch std::ios::failure exception
		# raised by the shared libstdc++.so while running on Ubuntu 16.04.
		set(stdlib_cxx "${stdlib_cxx} -static-libstdc++")

		# NOTE We need to use shared libgcc to be able to throw and catch exceptions
		# across different shared libraries, as may be the case when our python
		# extensions runtime-link to capnproto symbols in pycapnp's extension.
		set(stdlib_common "${stdlib_common} -shared-libgcc")
	  else()
		set(stdlib_common "${stdlib_common} -static-libgcc")
		set(stdlib_cxx "${stdlib_cxx} -static-libstdc++")
	  endif()
	endif()


	#
	# Determine Optimization flags here
	# These are quite aggresive flags, if your code misbehaves for strange reasons,
	# try compiling without them.
	#
	# Run-time error checks are a way for you to find problems in your running code; for more information, see "How to: Use Native Run-Time Checks".
	# 
	# If you compile your program at the command line using any of the /RTC compiler options, any pragma optimize instructions in your code will silently fail. 
	# This is because run-time error checks are not valid in a release (optimized) build.  You should use /RTC for development builds; /RTC should not be used 
	# for a retail build. /RTC cannot be used with compiler optimizations (/O Options (Optimize Code)).  A program image built with /RTC will be slightly larger 
	# and slightly slower than an image built with /Od (up to 5 percent slower than an /Od build).

	set(optimization_flags_cc "${optimization_flags_cc} -O2")
	set(optimization_flags_cc "-pipe ${optimization_flags_cc}") #TODO use -Ofast instead of -O3
	set(optimization_flags_lt "-O2 ${optimization_flags_lt}")

	if(NOT ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "armv7l")
		set(optimization_flags_cc "${optimization_flags_cc} -mtune=generic")
	endif()

	if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" AND NOT MINGW)
		set(optimization_flags_cc "${optimization_flags_cc} -fuse-ld=gold")
		# NOTE -flto must go together in both cc and ld flags; also, it's presently incompatible
		# with the -g option in at least some GNU compilers (saw in `man gcc` on Ubuntu)
		set(optimization_flags_cc "${optimization_flags_cc} -fuse-linker-plugin -flto-report -flto") #TODO fix LTO for clang
		set(optimization_flags_lt "${optimization_flags_lt} -flto") #TODO LTO for clang too
	endif()


	#
	# compiler specific settings and warnings here
	#

	set(shared_compile_flags "")
	set(internal_compiler_warning_flags "")
	set(cxx_flags_unoptimized "")
	set(shared_linker_flags_unoptimized "")
	set(fail_link_on_undefined_symbols_flags "")
	set(allow_link_with_undefined_symbols_flags "")


	  # LLVM Clang / Gnu GCC
	set(cxx_flags_unoptimized "${cxx_flags_unoptimized} ${stdlib_cxx} -std=c++11")

	if(${NUPIC_BUILD_PYEXT_MODULES})
		# Hide all symbols in DLLs except the ones with explicit visibility;
		# see https://gcc.gnu.org/wiki/Visibility
		set(cxx_flags_unoptimized "${cxx_flags_unoptimized} -fvisibility-inlines-hidden")
		set(shared_compile_flags "${shared_compile_flags} -fvisibility=hidden")
	endif()

	set(shared_compile_flags "${shared_compile_flags} ${stdlib_common} -fdiagnostics-show-option")
	set(internal_compiler_warning_flags "${internal_compiler_warning_flags} -Werror -Wextra -Wreturn-type -Wunused -Wno-unused-variable -Wno-unused-parameter -Wno-missing-field-initializers")
	set (external_compiler_warning_flags "${external_compiler_warning_flags} -Wno-unused-variable -Wno-unused-parameter -Wno-incompatible-pointer-types -Wno-deprecated-declarations")

	CHECK_CXX_COMPILER_FLAG(-m${BITNESS} compiler_supports_machine_option)
	if(compiler_supports_machine_option)
		set(shared_compile_flags "${shared_compile_flags} -m${BITNESS}")
		set(shared_linker_flags_unoptimized "${shared_linker_flags_unoptimized} -m${BITNESS}")
	endif()
	if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "armv7l")
		set(shared_compile_flags "${shared_compile_flags} -marm")
		set(shared_linker_flags_unoptimized "${shared_linker_flags_unoptimized} -marm")
	endif()

	if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
		set(shared_compile_flags "${shared_compile_flags} -fPIC")
		set (internal_compiler_warning_flags "${internal_compiler_warning_flags} -Wall")
	endif()
	
	if(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
		set(shared_compile_flags "${shared_compile_flags} -Wno-deprecated-register")
	endif()

	set(shared_linker_flags_unoptimized "${shared_linker_flags_unoptimized} ${stdlib_common} ${stdlib_cxx}")

	# Don't allow undefined symbols when linking executables
	if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
	  set(fail_link_on_undefined_symbols_flags "-Wl,--no-undefined")
	elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
	  set(fail_link_on_undefined_symbols_flags "-Wl,-undefined,error")
	endif()

	# Don't force python extensions to link to specific libpython during build:
	# python symbols are made available to extensions atomatically once loaded
	#
	# NOTE Windows DLLs are shared executables with their own main; they require
	# all symbols to resolve at link time.
	if(NOT "${PLATFORM}" STREQUAL "windows")
	  if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
		set(allow_link_with_undefined_symbols_flags "-Wl,--allow-shlib-undefined")
	  elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
		set(allow_link_with_undefined_symbols_flags "-Wl,-undefined,dynamic_lookup")
	  endif()
	endif()


	# Compatibility with gcc >= 4.9 which requires the use of gcc's own wrappers for
	# ar and ranlib in combination with LTO works also with LTO disabled
	if(UNIX AND CMAKE_COMPILER_IS_GNUCXX AND (NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Debug") AND
		  (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER "4.9" OR
		   CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL "4.9"))
		set(CMAKE_AR "gcc-ar")
		set(CMAKE_RANLIB "gcc-ranlib")
	endif()

	#
	# Set up Debug vs. Release options
	#
	set(debug_specific_compile_flags)
	set(debug_specific_linker_flags)

	if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
	  set (debug_specific_compile_flags "${debug_specific_compile_flags} -g")

	  set(debug_specific_linker_flags "${debug_specific_linker_flags} -O0")

	  if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" OR MINGW)
		set (debug_specific_compile_flags "${debug_specific_compile_flags} -Og")

		# Enable diagnostic features of standard class templates, including ability
		# to examine containers in gdb.
		# See https://gcc.gnu.org/onlinedocs/libstdc++/manual/debug_mode_using.html
		list(APPEND COMMON_COMPILER_DEFINITIONS -D_GLIBCXX_DEBUG)
	  elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
		# NOTE: debug mode is immature in Clang, and values of _LIBCPP_DEBUG above 0
		# require  the debug build of libc++ to be present at linktime on OS X.
		list(APPEND COMMON_COMPILER_DEFINITIONS -D_LIBCPP_DEBUG=0)
	  endif()

	  # Disable optimizations
	  set(optimization_flags_cc)
	  set(optimization_flags_lt)
	endif()


	#
	# Assemble compiler and linker properties
	#

	# Settings for internal nupic.core code
	set(INTERNAL_CXX_FLAGS_OPTIMIZED "${debug_specific_compile_flags} ${shared_compile_flags} ${cxx_flags_unoptimized} ${internal_compiler_warning_flags} ${optimization_flags_cc}")

	set(complete_linker_flags_unoptimized "${debug_specific_linker_flags} ${shared_linker_flags_unoptimized}")
	set(complete_linker_flags_unoptimized "${complete_linker_flags_unoptimized} ${fail_link_on_undefined_symbols_flags}")
	set(INTERNAL_LINKER_FLAGS_OPTIMIZED "${complete_linker_flags_unoptimized} ${optimization_flags_lt}")


	# Set up compile flags for internal sources
	if(MINGW)
	  # This is for GCC 4.8.x
	  # http://stackoverflow.com/questions/10660524/error-building-boost-1-49-0-with-gcc-4-7-0
	  set(INTERNAL_CXX_FLAGS_OPTIMIZED "${INTERNAL_CXX_FLAGS_OPTIMIZED} -include cmath")
	endif()

	# Compiler definitions specific to nupic.core code
	string(TOUPPER ${PLATFORM} platform_uppercase)
	set(COMMON_COMPILER_DEFINITIONS
		${COMMON_COMPILER_DEFINITIONS}
		-DNTA_OS_${platform_uppercase}
		-DNTA_ARCH_${BITNESS}
		-DHAVE_CONFIG_H
		-DNTA_INTERNAL
		-DBOOST_MATH_NO_LONG_DOUBLE_MATH_FUNCTIONS
		-DBOOST_NO_WREGEX
		-DNUPIC2
		)

	if(NOT "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "armv7l")
	  set(COMMON_COMPILER_DEFINITIONS
		  ${COMMON_COMPILER_DEFINITIONS}
		  -DNTA_ASM)
	endif()

	if(NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Release")
	  set(COMMON_COMPILER_DEFINITIONS
		  ${COMMON_COMPILER_DEFINITIONS}
		  -DNTA_ASSERTIONS_ON)
	endif()

	if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
	  set(COMMON_COMPILER_DEFINITIONS
		  ${COMMON_COMPILER_DEFINITIONS}
		  -DNTA_COMPILER_GNU)
	elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
	  set(COMMON_COMPILER_DEFINITIONS
		  ${COMMON_COMPILER_DEFINITIONS}
		  -DNTA_COMPILER_CLANG)
	elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "MinGW")
	  set(COMMON_COMPILER_DEFINITIONS
		  ${COMMON_COMPILER_DEFINITIONS}
		  -DNTA_COMPILER_GNU
		  -D_hypot=hypot)
	endif()

	# Common system libraries for shared libraries and executables
	set(src_common_os_libs)
	if("${PLATFORM}" STREQUAL "linux")
		set(COMMON_COMPILER_DEFINITIONS
		  ${COMMON_COMPILER_DEFINITIONS}
		  -DNTA_OS_LINUX)
		list(APPEND src_common_os_libs pthread dl)
	elseif("${PLATFORM}" STREQUAL "darwin")
		set(COMMON_COMPILER_DEFINITIONS
		  ${COMMON_COMPILER_DEFINITIONS}
		  -DNTA_OS_DARWIN)
		list(APPEND src_common_os_libs c++abi)
	elseif(MSYS OR MINGW)
	  list(APPEND src_common_os_libs psapi ws2_32 wsock32 rpcrt4)
	endif()
endif()

message(STATUS "INTERNAL_CXX_FLAGS             =${INTERNAL_CXX_FLAGS}")
message(STATUS "INTERNAL_LINKER_FLAGS          =${INTERNAL_LINKER_FLAGS}")
message(STATUS "COMMON_COMPILER_DEFINITIONS    =${COMMON_COMPILER_DEFINITIONS}")
message(STATUS "src_common_os_libs             =${src_common_os_libs}")
