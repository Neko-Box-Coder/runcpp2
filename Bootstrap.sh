#!/bin/bash

set -ex

mkdir -p BootstrapBuild

cp ./External/cfgpath/cfgpath.h ./Src/cfgpath.h

g++ -c -w -g -std=c++11 \
    "-DRUNCPP2_VERSION=\"BOOTSTRAP_VERSON\"" \
    "-DRUNCPP2_CONFIG_VERSION=0" \
    "-DRUNCPP2_BOOTSTRAP=1" \
    "-DssLOG_ASCII=0" "-DssLOG_CALL_STACK=1" "-DssLOG_CALL_STACK_ONLY=0" "-DssLOG_IMMEDIATE_FLUSH=0" \
    "-DssLOG_LEVEL=4" "-DssLOG_LOG_TO_FILE=0" "-DssLOG_SHOW_DATE=0" "-DssLOG_SHOW_FILE_NAME=1" \
    "-DssLOG_SHOW_FUNC_NAME=1" "-DssLOG_SHOW_LINE_NUM=1" "-DssLOG_SHOW_TIME=1" \
    "-DssLOG_THREAD_SAFE_OUTPUT=1" "-DssLOG_THREAD_VSPACE=4" "-DssLOG_USE_ESCAPE_SEQUENCES=0" \
    "-DssLOG_USE_WINDOWS_COLOR=0" "-DGHC_WIN_DISABLE_WSTRING_STORAGE_TYPE=1" "-DDS_USE_DEBUG_BREAK=1" \
    "-DYAML_DECLARE_STATIC=1" \
    -isystem "./External/ssLogger/Include" \
    -isystem "./External/filesystem/include" \
    -isystem "./External/System2/." \
    -isystem "./External/dylib/include" \
    -isystem "./External/variant/include" \
    -isystem "./External/DSResult/Include" \
    -isystem "./External/DSResult/External/expected/include" \
    -isystem "./External/libyaml/include" \
    -isystem "./External" \
    -isystem "./External/string-view-lite/include" \
    -isystem "./External/CppOverride/Include_SingleHeader" \
    -isystem "./External/MacroPowerToys/." \
    -I"./Src/runcpp2" \
    -I"./Src" \
    "./Src/runcpp2/runcpp2.cpp" \
    -o "./BootstrapBuild/runcpp2.o"

g++ -Wl,-rpath,\$ORIGIN -o "./BootstrapBuild/runcpp2" "./BootstrapBuild/runcpp2.o"
