#!/bin/bash

set -e
set -x

rm -rf build
mkdir build
pushd build

export CONAN_SYSREQUIRES_MODE=enabled

# conan install ..
conan install .. -s build_type=Debug #--install-folder=build --build missing

cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build .
