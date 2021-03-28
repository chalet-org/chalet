#!/bin/bash

bash ./build.sh Debug && build/Debug/chalet-debug --save-schema buildrun Release

echo "exited with code $?"