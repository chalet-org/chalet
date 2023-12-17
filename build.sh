#!/usr/bin/env bash

BUILD_CONFIGURATION=$1

CWD="$PWD"

export GCC_COLORS="error=01;31:warning=01;33:note=01;36:caret=01;32:locus=00;34:quote=01"

if [[ $BUILD_CONFIGURATION == '' ]]; then
	echo 'Error: please provide a build configuration (Debug / Release)'
	exit 1
fi

if [[ $OSTYPE == 'linux-gnueabihf' ]]; then
	PLATFORM=pi
elif [[ $OSTYPE == 'linux-gnu'* || $OSTYPE == 'cygwin'* ]]; then
	PLATFORM=linux
elif [[ $OSTYPE == 'darwin'* ]]; then
	PLATFORM=macos
elif [[ $OSTYPE == 'msys' || $OSTYPE == 'win32' ]]; then
	PLATFORM=windows
fi

BUILD_FOLDER="build/$BUILD_CONFIGURATION"
mkdir -p "$BUILD_FOLDER"
cd "$CWD/$BUILD_FOLDER"

if [[ $CI == "1" || $PLATFORM == "pi" ]]; then
	cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=$BUILD_CONFIGURATION $CWD
	# make -j8
	if [[ $PLATFORM == "pi" ]]; then
		cmake --build . -j 1 -- --no-print-directory
	else
		cmake --build . -j $(getconf _NPROCESSORS_ONLN) -- --no-print-directory
	fi
elif [[ $PLATFORM == "windows" ]]; then
	PATH="/c/msys64/mingw64/bin:$PATH"
	cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=$BUILD_CONFIGURATION $CWD
	mingw32-make -j$(getconf _NPROCESSORS_ONLN)
	# cmake --build . -j $(getconf _NPROCESSORS_ONLN) -- --no-print-directory
else
	cmake -G "Ninja" -DCMAKE_BUILD_TYPE=$BUILD_CONFIGURATION $CWD
	# make -j8
	cmake --build . -j $(getconf _NPROCESSORS_ONLN) -- -d keepdepfile
fi

EXIT_CODE=$?

echo
echo "==============================================================================="
echo

exit $EXIT_CODE
