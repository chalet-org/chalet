#!/usr/bin/env bash

if [[ $OSTYPE == 'linux-gnu'* || $OSTYPE == 'cygwin'* ]]; then
	PLATFORM=linux
elif [[ $OSTYPE == 'darwin'* ]]; then
	PLATFORM=macos
elif [[ $OSTYPE == 'msys' || $OSTYPE == 'win32' ]]; then
	PLATFORM=windows
fi

bash ./fetch_externals.sh

if [[ $PLATFORM == "windows" ]]; then
	# MinGW (Hasn't been tested in a long time)
	# PATH="/c/msys64/mingw64/bin:$PATH"
	# bash ./build.sh Debug && build/Debug/chalet-debug -c Release buildrun

	# Visual Studio
	./build.bat Debug && build/msvc_Debug/chalet-debug -c Release buildrun
elif [[ $PLATFORM == "macos" ]]; then
	# Expects apple clang
	bash ./build.sh Debug && build/Debug/chalet-debug --os-target-name macosx --os-target-version 11.0 -c Release buildrun
else
	# Expects GCC
	bash ./build.sh Debug && build/Debug/chalet-debug -c Release buildrun
fi

echo "exited with code $?"
