@echo off
setlocal enableextensions enabledelayedexpansion
rem |     startupVCpkg.bat
rem | 
rem | Runs CMake to configure nupic.base for Visual Studio 2017.
rem | When complete, double click build\nupic.base.sln to start Visual Studio 2017
rem | and perform a full build.
rem |
rem | Usage:
rem |    cd external\Windows
rem |    startupVCpkg [- | path]  [triplet]
rem |        path      Path to vcpkg installation ROOT
rem |        triplet  Triplet to use, default is  x64-windows-static
rem |
rem | The vcpkg path can be specified in the following ways:
rem |   1) using the first runtime option to specify the path to the ROOT folder of the vcpkg installation
rem |   2) using " - " as first argument
rem |         Looks for environment variable VCPKG_ROOT  expects the path to the vcpkg installation ROOT folder.
rem |         else Looks for the path to vcpkg.exe in the PATH environment variable.
rem |
rem | The triplet is a specification for the target environment.
rem | This can be specified in the following ways:
rem |  1) using second runtime option -t e.g  setup.bat path... x64-windows-static
rem |  2) by setting environment varable VCPKG_TRIPLET
rem |  3) the default is "x64-windows-static"   (normally what you want)
rem | The acceptable values for the  triplet are listed using:
rem |       C:\> vcpkg>vcpkg help triplet
rem |   A triplet of "x64-windows" means to build dynamic libraries for 64 bit Windows.
rem |   A triplet of "x64-windows-static" means build static libraries for 64 bit Windows.

rem |
  
rem  Evaluate the argument 1
	if "%1"=="" goto checkDefaultRoot
	if "%1"=="-" goto checkDefaultRoot
	set "VCPKG_ROOT=%1"
	goto get_Arg2
	
:checkDefaultRoot
if not "%VCPKG_ROOT%"=="" goto get_Arg2
set "x=vcpkg"
@echo %x~$PATH%
where vcpkg > xtmp.txt 2> NUL
if %errorlevel% neq 0 (
	@echo The path to the VCPKG Root folder cannot be determined.
	goto Usage
)
set /p VCPKG_ROOT=<xtmp.txt
del xtmp.txt
set VCPKG_ROOT=!VCPKG_ROOT:~0,-10!
if "%VCPKG_ROOT%"=="" (
	@echo The path to the VCPKG Root folder cannot be determined.
	goto Usage
)

rem  Evaluate the argument 2
:get_Arg2
if not "%2"=="" (
	set "VCPKG_TRIPLET=%2"
)
if "%VCPKG_TRIPLET%"=="" (
	set "VCPKG_TRIPLET=x64-windows-static"
)


rem  make sure CMake is installed.
set CMAKE=cmake
%CMAKE% -version > NUL 2> NUL 
if !errorlevel! neq 0 (
	set "CMAKE=C:/Program Files/CMake/bin/cmake.exe"
	%CMAKE% -version > NUL 2> NUL 
	if !errorlevel! neq 0 (
		@echo %~nx0%;  CMake was not found. 
		@echo Make sure its path is in the system PATH environment variable.
		goto Usage
	)
)

rem  make sure VCpkg is installed.
%VCPKG_ROOT%\vcpkg version > NUL 2> NUL 
if !errorlevel! neq 0 (
  @echo The vcpkg Root directory was not found. 
  @echo Make sure its path is in the first argument, 
  @echo defined in the VCPKG_ROOT environment variable, 
  @echo or its executable is the in PATH environment variable.
  goto Usage
)


:Build the dependancies
set "VCPKG_DEFAULT_TRIPLET=!VCPKG_TRIPLET!"
set "VCPKG=!VCPKG_ROOT!\vcpkg"
%VCPKG% install boost-system
%VCPKG% install boost-filesystem
%VCPKG% install boost-dll
%VCPKG% install yaml-cpp
%VCPKG% install gtest

rem |   6/21/2018  There is an issue regarding the build of boost as a static library. 
rem |          https://github.com/Microsoft/vcpkg/issues/1338
rem |  So, the Boost configuration must be manually set
set "BOOST_ROOT_FOLDER=!VCPKG_ROOT!/installed/!VCPKG_TRIPLET!
set "BOOST_LIBS_FOLDER=!VCPKG_ROOT!/installed/!VCPKG_TRIPLET!/lib
set "CMAKE_TOOLCHAIN_FILE=!VCPKG_ROOT!/scripts/buildsystems/vcpkg.cmake"


rem remove build folder if it exists
pushd ..\..
if exist ".\build\" (
    rd /s /q "build\"
)

mkdir build
pushd build

!CMAKE! -DBOOST_ROOT:PATH=%BOOST_ROOT_FOLDER% ^
		-DBOOST_LIBRARYDIR:PATH=%BOOST_LIBS_FOLDER% ^
		-DCMAKE_TOOLCHAIN_FILE=!CMAKE_TOOLCHAIN_FILE! ^
		-DVCPKG_TARGET_TRIPLET:STRING=!VCPKG_TRIPLET! ^
		-G "Visual Studio 15 2017 Win64" ^
		..


rem Building
popd
popd
@echo If this ran successfully, you can now start Visual Studio using solution file %BUILDDIR%/nupic.base.sln
goto :EOF


:Usage
@echo Usage: 
@echo   startupVCpkg [- ^| path]  [triplet]
@echo      path      Path to vcpkg installation ROOT. 
@echo                Default is root path in envirement VCPKG_ROOT
@echo                 or vcpkg.exe path in PATH envirement variable.
@echo      triplet   Triplet to use, default is  x64-windows-static
exit /B 1