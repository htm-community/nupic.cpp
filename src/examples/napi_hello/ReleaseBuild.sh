#! /bin/sh
# An example script to build for Release using GCC on linux ...
# First build htm.core as Release from sources.
#      cd <path-to-repo>
#      mkdir -p build/scripts
#      cd build/scripts
#      cmake ../..
#      make -j4 install
#
# Now build napi_hello
# We use -std=c++17 to get <filesystem> so you can avoid using the boost library.
# The -I gives the path to the includes needed to use the htm.core library.
# The -L gives the path to the shared htm.core library location at build time.
g++ -o napi_hello -std=c++17  -I ../../../build/Release/include myapp.cpp -L ../../../build/Release/lib -lhtm_core -lpthread -ldl

# Run napi_hello 
# The LD_LIBRARY_PATH envirment variable points to the htm.core library location at runtime.
export LD_LIBRARY_PATH=../../../build/Release/lib:$LD_LIBRARY_PATH
./napi_hello