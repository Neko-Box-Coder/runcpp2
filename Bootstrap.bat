SETLOCAL ENABLEEXTENSIONS

if not exist ".\BootstrapBuild\" mkdir .\BootstrapBuild

copy /y .\External\cfgpath\cfgpath.h .\Src\cfgpath.h
if %errorlevel% neq 0 exit /b %errorlevel%

for /f "delims=" %%i in ('CALL "C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe" ^
    -version "[17.0,18.0)" -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 ^
    -property installationPath') do (
    
    set prerun=%%i\VC\Auxiliary\Build\vcvarsall.bat
)

CALL "%prerun%" x64 && ^
CL.exe /c /nologo /W0 /diagnostics:caret /utf-8 /MDd /EHar /RTC1 /Zc:inline /Zi ^
    /permissive- /external:W0 /std:c++14 /bigobj /DEBUG ^
    "/DRUNCPP2_VERSION=\"BOOTSTRAP_VERSON\"" ^
    "/DRUNCPP2_CONFIG_VERSION=0" ^
    "/DRUNCPP2_BOOTSTRAP=1" ^
    "/DssLOG_ASCII=0" "/DssLOG_CALL_STACK=1" "/DssLOG_CALL_STACK_ONLY=0" "/DssLOG_IMMEDIATE_FLUSH=0" ^
    "/DssLOG_LEVEL=5" "/DssLOG_LOG_TO_FILE=0" "/DssLOG_SHOW_DATE=0" "/DssLOG_SHOW_FILE_NAME=1" ^
    "/DssLOG_SHOW_FUNC_NAME=1" "/DssLOG_SHOW_LINE_NUM=1" "/DssLOG_SHOW_TIME=1" ^
    "/DssLOG_THREAD_SAFE_OUTPUT=1" "/DssLOG_THREAD_VSPACE=4" "/DssLOG_USE_ESCAPE_SEQUENCES=0" ^
    "/DssLOG_USE_WINDOWS_COLOR=0" "/DGHC_WIN_DISABLE_WSTRING_STORAGE_TYPE=1" ^
    "/DDS_USE_DEBUG_BREAK=1" "/DYAML_DECLARE_STATIC=1" ^
    /external:I".\External\ssLogger\Include" ^
    /external:I".\External\filesystem\include" ^
    /external:I".\External\System2" ^
    /external:I".\External\dylib\include" ^
    /external:I".\External\variant\include" ^
    /external:I".\External\DSResult\Include" ^
    /external:I".\External\DSResult\External\expected\include" ^
    /external:I".\External\libyaml\include" ^
    /external:I".\External" ^
    /external:I".\External\string-view-lite\include" ^
    /external:I".\External\CppOverride\Include_SingleHeader" ^
    /external:I".\External\External\MacroPowerToys" ^
    /I".\Src\runcpp2" ^
    /I".\Src" ^
    /Fo".\BootstrapBuild\runcpp2.obj" ^
    /Fd".\BootstrapBuild\runcpp2.pdb" ^
    ".\Src\runcpp2\runcpp2.cpp"
    
if %errorlevel% neq 0 exit /b %errorlevel%

CALL "%prerun%" x64 && ^
link.exe /NOLOGO kernel32.lib user32.lib gdi32.lib winspool.lib shell32.lib ole32.lib oleaut32.lib ^
    uuid.lib comdlg32.lib advapi32.lib /manifest:embed /SUBSYSTEM:CONSOLE ^
    /MANIFESTUAC:"level='asInvoker'" /DEBUG /OUT:".\BootstrapBuild\runcpp2.exe" ^
    ".\BootstrapBuild\runcpp2.obj"

if %errorlevel% neq 0 exit /b %errorlevel%

ECHO Bootstrap Done

exit /b 0
