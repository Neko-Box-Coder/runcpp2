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
runTest ./FilePropertiesTest
runTest ./FlagsOverrideInfoTest
runTest ./DependencySourceTest
runTest ./ProfilesCommandsTest
runTest ./FilesToCopyInfoTest
runTest ./ProfilesProcessPathsTest
runTest ./ProfilesDefinesTest
runTest ./DependencyLinkPropertyTest
runTest ./FilesTypesInfoTest
runTest ./ProfilesFlagsOverrideTest
runTest ./StageInfoTest
runTest ./DependencyInfoTest
runTest ./ScriptInfoTest
runTest ./ProfileTest
