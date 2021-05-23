#    nupic/csharp/wrappers/CMakeLists.txt
# This will build htm_core_cs.dll   into  output folder CMAKE_LIBRARY_OUTPUT_DIRECTORY
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
# Author:  David Keeney, 2019 
# -----------------------------------------------------------------------------
#
# The CSharp (and Managed C++) Interface build
#
# NOTE: The managed code must be compiled with /EHa rather than /EHs
#       All non-managed code is compiled with /EHs.  This defines the type of
#       exception handling is to be used.  What this means is that when managed code 
#       calls into unmanaged code, there may be cases where the stack is not unwound 
#       or destructors are not called when exceptions cross the unmanaged/managed boundary.  
#       This can leak memory.
#
#       To avoid this problem, create a try-catch block within the unmanaged code, and 
#       ensure that no exceptions are passed between the unmanaged world and the managed world. 
#       Catch it as an unmanaged exception and re-throw it as a managed exception.
#
# NOTE: See https://docs.microsoft.com/en-us/cpp/build/reference/clr-restrictions
#       The following compile options are NOT supported when using C++ /clr
#
#        /EHsc and /EHs (/clr implies /EHa (see /EH (Exception Handling Model))
#        /fp:strict and /fp:except (see /fp (Specify Floating-Point Behavior))
#        /Zd
#        /Gm
#        /MT  (/clr implies MD)
#        /RTC and /RTC1
#        /ZI

cmake_minimum_required(VERSION 2.8.9)


      ############################
      #
      # temporary library to include RegionImplFactory.cpp that was compiled with CSHARP_INTERFACE defined
      #    
      set(tmp_lib regionImplFactory)
      add_library(${tmp_lib}  STATIC
              "../../engine/RegionImplFactory.cpp")
      set_target_properties(${tmp_lib} PROPERTIES COMPILE_FLAGS "${INTERNAL_CXX_FLAGS_OPTIMIZED} /DCSHARP_INTERFACE")
      set_target_properties(${tmp_lib} PROPERTIES LINK_FLAGS "${INTERNAL_LINKER_FLAGS_OPTIMIZED}")

      # htm_core_cs.dll depends on htm_core.lib, and Csharp support files
      add_dependencies(${tmp_lib}
                            ${src_external_static_libs}
                            ${src_lib_static_nupiccore_solo})
                            
      ########################
      #
      # CSharp interface library
      #
      # Build a DLL library containing the Managed C++ wrapper code.
      # Compile with /clr /EHa and /LD or /LDd
      #
      set(src_cs_wrapper_files
          AssemblyInfo.cpp
      #  cs_Types.cpp   ### all of these are included in cs_Engine.cpp
      #  cs_Utils.cpp 
      #  cs_Math.cpp 
          cs_Engine.cpp
      )
      
      set(FORCE_CXX ON)
      set(MANAGED_CXX_FLAGS "${CMAKE_CXX_FLAGS} /clr /EHa -DCSHARP_INTERFACE")
      if(MANAGED_CXX_FLAGS MATCHES "/EHsc")
        string(REPLACE "/EHsc" "" MANAGED_CXX_FLAGS "${MANAGED_CXX_FLAGS}")
      endif()
      set(MANAGED_CXX_FLAGS_DEBUG "${MANAGED_CXX_FLAGS} /MDd /Zi /Ob0 /Od")
      if(CMAKE_CXX_FLAGS_DEBUG MATCHES "/RTC1")
         string(REPLACE "/RTC1" " " MANAGED_CXX_FLAGS_DEBUG "${MANAGED_CXX_FLAGS_DEBUG}")
      endif() 
      
      set(MANAGED_CXX_FLAGS "${MANAGED_CXX_FLAGS} /AI${PROJECT_BINARY_DIR}/nupic/csharp/bindings/htm_core_cs.dir/Release")
      message(STATUS "MANAGED_CXX_FLAGS: ${MANAGED_CXX_FLAGS}")
      message(STATUS "MANAGED_CXX_FLAGS_DEBUG: ${MANAGED_CXX_FLAGS_DEBUG}")
      
      add_library(${LIB_TARGET} SHARED  ${src_cs_wrapper_files})
      set_target_properties(${LIB_TARGET} PROPERTIES COMPILE_FLAGS "${MANAGED_CXX_FLAGS}")
#set_target_properties(${LIB_TARGET} PROPERTIES LINK_FLAGS "${INTERNAL_LINKER_FLAGS_OPTIMIZED}")

      target_link_libraries(${LIB_TARGET}
                            ${tmp_lib}
                            ${src_lib_static_nupiccore_solo}
                            ${src_external_static_libs}
                            ${src_common_os_libs})

      add_dependencies(${LIB_TARGET}
                            ${src_external_static_libs}
                            ${src_lib_static_nupiccore_solo}
                            ${tmp_lib})

###      STRING (REGEX REPLACE "/RTC(su|[1su])" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")   
      
      