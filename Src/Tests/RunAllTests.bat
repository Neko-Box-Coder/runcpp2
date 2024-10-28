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
        ECHO "[Auto Test Warning] %~1 doesn't exist, skipping"
        ECHO ""
        EXIT /b
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
CALL :RUN_TEST "%~dp0\%MODE%BuildsManagerTest.exe"
CALL :RUN_TEST "%~dp0\%MODE%FilePropertiesTest.exe"
CALL :RUN_TEST "%~dp0\%MODE%FlagsOverrideInfoTest.exe"
CALL :RUN_TEST "%~dp0\%MODE%DependencySourceTest.exe"
CALL :RUN_TEST "%~dp0\%MODE%DependencyCommandsTest.exe"
CALL :RUN_TEST "%~dp0\%MODE%FilesToCopyInfoTest.exe"
CALL :RUN_TEST "%~dp0\%MODE%ProfilesCompilesFilesTest.exe"
CALL :RUN_TEST "%~dp0\%MODE%ProfilesDefinesTest.exe"
CALL :RUN_TEST "%~dp0\%MODE%DependencyLinkPropertyTest.exe"
CALL :RUN_TEST "%~dp0\%MODE%FilesTypesInfoTest.exe"
CALL :RUN_TEST "%~dp0\%MODE%ProfilesFlagsOverrideTest.exe"
CALL :RUN_TEST "%~dp0\%MODE%StageInfoTest.exe"
CALL :RUN_TEST "%~dp0\%MODE%DependencyInfoTest.exe"
CALL :RUN_TEST "%~dp0\%MODE%ScriptInfoTest.exe"
CALL :RUN_TEST "%~dp0\%MODE%ProfileTest.exe"

EXIT 0

:FAILED
EXIT 1
