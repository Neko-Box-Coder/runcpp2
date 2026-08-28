SETLOCAL ENABLEEXTENSIONS

GOTO :FINAL

:RUN_TEST <testFile>
    @REM Setlocal EnableDelayedExpansion
    IF NOT EXIST "%~1" (
        ECHO "%~1 doesn't exist"
        ECHO ""
        GOTO :FAILED
    )
    PUSHD "%~dp1"
    CALL "%~1"
    IF NOT %errorlevel% == 0 (
        ECHO "Failed: %errorlevel%"
        GOTO :FAILED
    )
    POPD
    EXIT /b


:FINAL
CALL :RUN_TEST "%~dp0\BuildTypeTest.exe"
CALL :RUN_TEST "%~dp0\DependencyInfoTest.exe"
CALL :RUN_TEST "%~dp0\DependencySourceTest.exe"
CALL :RUN_TEST "%~dp0\ProfileTest.exe"
CALL :RUN_TEST "%~dp0\ScriptInfoTest.exe"
CALL :RUN_TEST "%~dp0\BuildsManagerTest.exe"
CALL :RUN_TEST "%~dp0\ConfigParsingTest.exe"
CALL :RUN_TEST "%~dp0\IncludeManagerTest.exe"

EXIT 0

:FAILED
EXIT 1
