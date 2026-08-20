@ECHO OFF

SETLOCAL ENABLEEXTENSIONS

copy .\External\cfgpath\cfgpath.h .\Src\cfgpath.h
if %errorlevel% neq 0 exit /b %errorlevel%

CL.exe /c /nologo /W4 /diagnostics:caret /utf-8 /MDd /EHar /std:c++11 /RTC1 /Zc:inline /Zi ^
    /permissive- /external:W0 /DEBUG ^
    "/DRUNCPP2_VERSION=BOOTSTRAP" ^
    "/DRUNCPP2_CONFIG_VERSION=0" ^
    "/DYAML_DECLARE_STATIC=1" ^
    /external:I".\External\ssLogger\Include" ^
    /external:I".\External\filesystem\include" ^
    /external:I".\External\System2" ^
    /external:I".\External\dylib\include" ^
    /external:I".\External\variant\include" ^
    /external:I".\External\DSResult\Include" ^
    /external:I".\External\libyaml\include" ^
    /external:I".\External\string-view-lite\include" ^
    /external:I".\External\External\CppOverride\Include_SingleHeader" ^
    /external:I".\External\External\MacroPowerToys" ^
    /I".\Src" ^
    /Fo".\bootstrap\runcpp2.obj"
    
if %errorlevel% neq 0 exit /b %errorlevel%
