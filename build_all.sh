#!/bin/bash

if [[ $OSTYPE == 'linux-gnu'* || $OSTYPE == 'cygwin'* ]]; then
	PLATFORM=linux
elif [[ $OSTYPE == 'darwin'* ]]; then
	PLATFORM=macos
elif [[ $OSTYPE == 'msys' || $OSTYPE == 'win32' ]]; then
	PLATFORM=windows
fi

if [[ $PLATFORM == "windows" ]]; then
	PATH="/c/msys64/mingw64/bin:$PATH"
fi

bash ./fetch_vendors.sh

bash ./build.sh Debug && build/Debug/chalet-debug -c Release buildrun

echo "exited with code $?"