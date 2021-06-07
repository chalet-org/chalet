#!/bin/bash

BUILD_CONFIGURATION=$1

CWD="$PWD"

export GCC_COLORS="error=01;31:warning=01;33:note=01;36:caret=01;32:locus=00;34:quote=01"

if [[ $BUILD_CONFIGURATION == '' ]]; then
	echo 'Error: please provide a build configuration (Debug / Release)'
	exit 1
fi

if [[ $OSTYPE == 'linux-gnu'* || $OSTYPE == 'cygwin'* ]]; then
	PLATFORM=linux
elif [[ $OSTYPE == 'darwin'* ]]; then
	PLATFORM=macos
elif [[ $OSTYPE == 'msys' || $OSTYPE == 'win32' ]]; then
	PLATFORM=windows
fi

BUILD_FOLDER="build/$BUILD_CONFIGURATION"
mkdir -p "$BUILD_FOLDER"
cd "$CWD/$BUILD_FOLDER"

if [[ $PLATFORM == "windows" ]]; then
	PATH="/c/msys64/mingw64/bin:$PATH"
	cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=$BUILD_CONFIGURATION $CWD
	# cmake -E time mingw32-make -j8
	mingw32-make -j8
	# cmake --build . -j 8 -- --no-print-directory
else
	cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=$BUILD_CONFIGURATION $CWD
	# cmake -E time make -j8
	# make -j8
	cmake --build . -j 16 -- --no-print-directory
fi

EXIT_CODE=$?

echo
echo "==============================================================================="
echo

exit $EXIT_CODE