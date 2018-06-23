Nupic.Base is a very slim cpp library containing mostly the htm algorithms. It's cpp only with very few dependencies:


# Building under windows using Visual Studio 2017
##Prerequisites are 
- Visual Studio 2017, or newer (free Community version is ok)
	- on X64 OS (Windows 7, 8, or 10) 64 bit
	- with Desktop development with C++  
	   (be sure to check sub component "Visual C++ tools for CMake")
	- configured to build with C++17 compiler
	- Also note that Visual Studio 2015 will also work.
- CMake 3.11 or newer
     Add path to cmake.exe (i.e. C:\Program Files\CMake\bin) to your path environment variable.
     
- vcpkg package manager (optional)
	Clone vcpkg repository from https://github.com/Microsoft/vcpkg
	This will be used by the startupVCpkg.bat script be used to download, build, and install the prerequisite packages.
- hunter package manager (alternative)
	This does not require prior installation.  It will download and install itself if using startupHunter.bat
	
- The dependancy packages installed by the package managers are:
	- Boost (1.67 or newer)
	- yaml-cpp (0.5 or newer)
	- gtest

##Procedures
1 Clone the community version of ['nupic.cpp' repository](https://github.com/htm-community/nupic.cpp)
  You may want to install git to perform this job.

2 Create the Visual Studio solution and project files (.sln and .vcxproj). The dependancies
are downloaded and installed at the same time.
The scripts to do this are found in the "external\Windows" folder of the repository.
- startupVCpkg.bat  -- create nupic.base.sln using the vcpkg package manager
- startupHunter.bat -- create nupic.base.sln using the Hunter package manager

You should only need to run one of these, once to create the Visual Studio solution file
then you can use that to start up Visual Studio and build everything. If using the vcpkg package manager
be sure to add its path to the system PATH environment variable.

Run as administrator 'Visual Studio 2017 Developer version' of Command Prompt (a shortcut is in your start menu). 
CD to the folder "external/Windows/" under the repository. Execute startupVCpkg.bat to use the vcpkg manager
or startupHunter.bat to use the Hunter package manager. 

**Note: This can take a while the first time you run this because it needs to download, build and install all of the prerequiete packages including boost. If there is a major change to CMake structure you may need to start over with this step, delete the "build/" folder before running startupVCpkg.bat or startupHunter.bat command again.

3) Now you can start Visual Studio by double clicking the solution file at build/nupic.base.sln.  It will setup its configuration based on CMake.  
4) Build the "ALL_BUILD" project.  This will build all of the libraries.
5) Build the "INSTALL" project.  This will put the products in build/bin (unless you specified elsewhere on the command line in step 2.)

