@ECHO OFF

SETLOCAL ENABLEEXTENSIONS

SET MODE=Release\
IF "%1"=="" (GOTO :DEFAULT) ELSE (GOTO :PAREM)


:DEFAULT
GOTO :FINAL

:PAREM
IF "%1"=="-r" (
    SET MODE=Release\
)
IF "%1"=="-d" (
    SET MODE=Debug\
)
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
CALL :RUN_TEST "%~dp0\%MODE%BuildTypeTest.exe"
CALL :RUN_TEST "%~dp0\%MODE%DependencyInfoTest.exe"
CALL :RUN_TEST "%~dp0\%MODE%DependencySourceTest.exe"
CALL :RUN_TEST "%~dp0\%MODE%ProfileTest.exe"
CALL :RUN_TEST "%~dp0\%MODE%ScriptInfoTest.exe"
CALL :RUN_TEST "%~dp0\%MODE%BuildsManagerTest.exe"
CALL :RUN_TEST "%~dp0\%MODE%ConfigParsingTest.exe"
CALL :RUN_TEST "%~dp0\%MODE%IncludeManagerTest.exe"

EXIT 0

:FAILED
EXIT 1
