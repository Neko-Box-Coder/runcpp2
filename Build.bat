@REM @echo off

setlocal

mkdir Build || goto :error

pushd Build || goto :error

cmake .. || goto :error
cmake --build . --target Embed2C || goto :error
cmake .. || goto :error
cmake --build . -j 16 || goto :error

popd

goto :eof

:error
echo Error occurred during the build process.

goto :eof
