#!/bin/sh
set -e

mkdir -p ./Build
pushd ./Build
cmake ..
cmake --build . --target Embed2C
cmake ..
cmake --build . -j 16

popd

