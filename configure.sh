#!/usr/bin/env bash

cmake -H. -B/tmp/cmake-build-debug -DCMAKE_BUILD_TYPE=Debug -DHPX_DIR=/Users/parsa/Documents/hpx/code/hpx/cmake-build-debug/lib/cmake/HPX

#cmake --build /tmp/cmake-build-debug --config Debug

read -n1 -r -p 'Paused... Press any key to continue...' key

#cmake --open cmake-build-debug
