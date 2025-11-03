#!/bin/bash
#export TASN_OPTIONS=verbosity=1

cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
cmake --build build
