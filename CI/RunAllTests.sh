#!/bin/bash

set -e

function runTest()
{
    if [ -f $1 ]; then
        chmod +x $1
        $1
    else
        echo "[Auto Test Warning] $1 doesn't exist, skipping"
        echo ""
    fi
}

runTest ./BuildsManagerTest

