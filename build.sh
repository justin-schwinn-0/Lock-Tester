#!/bin/bash
#export TASN_OPTIONS=verbosity=1

cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ 
cmake --build build -j
