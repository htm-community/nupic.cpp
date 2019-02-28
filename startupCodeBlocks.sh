

# for starting up CodeBlocks
#  To install codeblocks on Umbuntu
#    sudo add-apt-repository ppa:damien-moore/codeblocks-stable
#    sudo apt update
#    sudo apt install codeblocks codeblocks-contrib
#    cd to repository 
#    ./startupCodeBlocks.sh
#
mkdir -p build/scripts
cd build/scripts
cmake -G "CodeBlocks - Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug ../..
codeblocks nupic_code.cbp
