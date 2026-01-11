#!/bin/bash

set -e

function runTest()
{
    if [ -f $1 ]; then
        chmod +x $1
        $1
    else
        echo "[Auto Test Warning] $1 doesn't exist, skipping"
        exit 1
    fi
}

runTest ./BuildTypeTest
runTest ./DependencyInfoTest
runTest ./DependencySourceTest
runTest ./ProfileTest
runTest ./ScriptInfoTest
runTest ./BuildsManagerTest
runTest ./ConfigParsingTest
runTest ./IncludeManagerTest
