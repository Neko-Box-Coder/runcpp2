@REM @echo off

setlocal

mkdir Build

pushd Build || goto :error

cmake .. || goto :error
cmake --build . --target Embed2C || goto :error
cmake .. -DssLOG_LEVEL=DEBUG "%*" || goto :error
cmake --build . -j 16 --config Debug || goto :error

popd

goto :eof

:error
echo Error occurred during the build process.

goto :eof
