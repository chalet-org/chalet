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
	# PATH="/c/msys64/mingw64/bin:$PATH"
	# bash ./build.sh Debug && build/Debug/chalet-debug -c Release buildrun
	./build.bat Debug && build/msvc_Debug/chalet-debug -c Release buildrun
else
	bash ./build.sh Debug && build/Debug/chalet-debug -c Release buildrun
fi

echo "exited with code $?"
