#!/usr/bin/env bash


if [[ $OSTYPE == 'linux-gnueabihf' ]]; then
	PLATFORM=pi
else
	echo 'Error: Raspberry Pi only'
	exit 1
fi

/opt/chalet/bin/chalet bundle
cp dist/chalet /opt/chalet/bin
