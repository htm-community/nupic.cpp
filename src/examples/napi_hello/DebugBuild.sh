#! /bin/sh
# An example script to build for debug using GCC on linux ...
# First build htm.core as debug from sources.
#      cd <path-to-repo>
#      mkdir -p build/scripts
#      cd build/scripts
#      cmake ../.. -DCMAKE_BUILD_TYPE=Debug
#      make -j4 install
#
# Now build napi_hello
# The -g -Og tells the compiler to build debug mode with no optimize.
# We use -std=c++17 to get <filesystem> so we can avoid using the boost library.
# The -D_GLIBCXX_DEBUG setting says compile std:: with debug
# The -I gives the path to the includes needed to use the htm.core library.
# The -L gives the path to the shared htm.core library location at build time.
# -lhtm_core says link with libhtm_core.so
g++ -g -Og -o napi_hello -std=c++17 -D_GLIBCXX_DEBUG -I ../../../build/Debug/include myapp.cpp -L ../../../build/Debug/lib -lhtm_core -lpthread -ldl

# Run napi_hello in the debugger
# The LD_LIBRARY_PATH envirment variable points to the htm.core library location at runtime.
export LD_LIBRARY_PATH=../../../build/Debug/lib:$LD_LIBRARY_PATH
gdb napi_hello