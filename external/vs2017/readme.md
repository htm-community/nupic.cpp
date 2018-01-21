# Visual Studio 2017 solution

ONLY 64BIT is supported!

This solution contains all c++ related projects.

## nupic.core

Dependencies (if not header only, then linked statically):
* Boost (1.66)
* zlib
* yaml-cpp


Compile Settings:
* Preprocessor: 
    Debug: _MBCS;NTA_OS_WINDOWS;NTA_COMPILER_MSVC;_CRT_SECURE_NO_WARNINGS;_SCL_SECURE_NO_WARNINGS;BOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE;BOOST_ALL_NO_LIB;
    Release: _MBCS;NTA_OS_WINDOWS;NTA_COMPILER_MSVC;_CRT_SECURE_NO_WARNINGS;_SCL_SECURE_NO_WARNINGS;BOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE;BOOST_ALL_NO_LIB;
* Debug Information:
    Debug: Program Database (/Zi)
    Release: None
* Warning Level 3 (/W3)
* No SDL checks (/sdl-)

Linker Settings:
    


Removals:

* capn proto
    - capnp::AnyPointer
    - base class stuff:  public Serializable<XXX>
    - virtual void write(capnp::AnyPointer::Builder& anyProto) const override;
    - virtual void read(capnp::AnyPointer::Reader& anyProto) override;

* os related ifdef's (will be switching to boost)

* no py_support or for that matter no python C API calls.


Output: Static Lib: nupic.core.lib

All the base classes but NO other languages related bindings. For such please have a look at nupic.bindingss.


## nupic.core_test

Dependencies (if not header only, then linked statically):
* gtest

Debug Compile Settings:
* Preprocessor: _DEBUG;_CONSOLE;_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING;BOOST_ALL_NO_LIB;NTA_OS_WINDOWS;
* Program Database (/Zi)
* Warning Level 3 (/W3)
* No SDL checks (/sdl-)


## nupic.python.algorithms
## nupic.python.engine
## nupic.python.math

Python plugins which expose nupic.core functionality. All dependencies are either header only or linked statically.

Output: Dynamic Lib: xxx.pyd

* Character Set: Multi-Byte Character Set

* Include Directories: ..\..\..\..\include;..\..\..\..\src\cpp;..\..\..\..\include\python36

* Preprocessor Definitions: _WINDOWS;_USRDLL;NTA_OS_WINDOWS;BOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE;_SCL_SECURE_NO_WARNINGS;

* Conformance Mode: No  (pybind11 doesn't work otherwise)

* Additional Library Directories: ..\..\lib\debug

* Additional Dependencies: Psapi.lib;zlibd.lib;libboost_system-vc141-mt-gd-x64-1_66.lib;libboost_filesystem-vc141-mt-gd-x64-1_66.lib;python36.lib;

* Post-Build Event: copy /Y "$(OutDir)$(TargetName)$(TargetExt)" "..\..\..\..\src\python\python3\nupic\$(TargetName)$(TargetExt)"

Interface code to be accessed by python 3. For python 2.7 have a look a nupic.python27.xxx


## nupic.python27.algorithms
## nupic.python27.engine
## nupic.python27.math

Same as nupic.python but compiled with python 2.7 includes and libs.


## yaml-cpp