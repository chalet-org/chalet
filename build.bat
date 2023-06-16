@echo off

set CWD=%CD%

set BUILD_CONFIGURATION=%1
set BUILD_FOLDER="build/msvc_%BUILD_CONFIGURATION%"

IF [%1] == [] (
	echo "Error: please provide a build configuration (Debug / Release)"
	exit 1
)

cmake -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=%BUILD_CONFIGURATION% -S %CWD% -B %BUILD_FOLDER%
cmake --build %BUILD_FOLDER%

set ERROR=%errorlevel%

IF [%ERROR%] == [1] (
	exit 1
)

echo:
echo ===============================================================================
echo:
