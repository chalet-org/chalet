#!/usr/bin/env bash

BUILD_CONFIGURATION=Debug

CWD="$PWD"

export GCC_COLORS="error=01;31:warning=01;33:note=01;36:caret=01;32:locus=00;34:quote=01"

if [[ $BUILD_CONFIGURATION == '' ]]; then
	echo 'Error: please provide a build configuration (Debug / Release)'
	exit 1
fi

if [[ $OSTYPE == 'darwin'* ]]; then
	PLATFORM=macos
else
	echo 'Error: macOS only'
	exit 1
fi

BUILD_FOLDER="build/xcode_$BUILD_CONFIGURATION"
mkdir -p "$BUILD_FOLDER"
cd "$CWD/$BUILD_FOLDER"

cmake -G "Xcode" -DCMAKE_BUILD_TYPE=$BUILD_CONFIGURATION $CWD
cmake --build . -j $(getconf _NPROCESSORS_ONLN)

EXIT_CODE=$?

echo
echo "==============================================================================="
echo

exit $EXIT_CODE
