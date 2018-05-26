@echo off
rem Runs CMake to configure nupic.core for Visual Studio 2017.
rem taken mostly from https://github.com/nanodbc/nanodbc/blob/master/utility/build.bat
rem !! Please make sure to use unix '/' folder seperator 
rem Usage:
rem 1. Boost root folder, e.g. c:/boost
rem 2. Boost libs folder, e.g. c:/boost/stage/libs
rem 3. VCPKG tool chain file, e.g. c:/vcpkg/scripts/buildsystems/vcpkg.cmake
rem 4. VCPKG target triplet, x64-windows-static

if not defined VS150COMNTOOLS goto :NoVS

if [%1]==[] goto :Usage
if [%2]==[] goto :Usage
if [%3]==[] goto :Usage
if [%4]==[] goto :Usage


:Build
set BUILDDIR=build
set BOOST_ROOT_FOLDER=%1
set BOOST_LIBS_FOLDER=%2
set VCPKG_TOOLCHAIN_CMAKE_FILE=%3
set VCPKG_TARGET_TRIPLET=%4

pushd ..\..

rem remove build folder
if exist ".\build\" (
    rd /s /q "build\"
)

mkdir %BUILDDIR%
pushd %BUILDDIR%

"C:/Program Files/CMake/bin/cmake.exe" ^
    -A x64 ^
    -DBOOST_ROOT:PATH=%BOOST_ROOT_FOLDER% ^
    -DBOOST_LIBRARYDIR:PATH=%BOOST_LIBS_FOLDER% ^
    -DCMAKE_TOOLCHAIN_FILE=%VCPKG_TOOLCHAIN_CMAKE_FILE% ^
    -DVCPKG_TARGET_TRIPLET=%VCPKG_TARGET_TRIPLET% ^
    ..

rem Building
rem msbuild.exe nupic.base.sln /p:Configuration=Release /p:Platform=x64

popd
popd
goto :EOF

:NoVS
@echo build.bat
@echo  Visual Studio 2017 not found
@echo  "%%VS150COMNTOOLS%%" environment variable not defined
exit /B 1

:Usage
@echo build.bat
@echo Usage: build.bat BOOST_ROOT_Folder
exit /B 1