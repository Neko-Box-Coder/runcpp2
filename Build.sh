#!/bin/bash
set -e

mkdir -p ./Build
pushd ./Build

cmake ..
cmake --build . --target Embed2C
cmake .. -DssLOG_LEVEL=DEBUG -DCMAKE_BUILD_TYPE=Debug "$@"
cmake --build . -j 16

popd

