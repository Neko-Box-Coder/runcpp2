//bin/true;runcpp2 "$0" "$@"; exit;

/* runcpp2

PassScriptPath: true
BuildType: Static
RequiredProfiles:
    Windows: ["msvc"]
    Unix: ["g++"]

OtherFilesToBeCompiled:
    # Target Platform
    DefaultPlatform:
        # Target Profile
        "g++":
        -   "./OtherSources/AnotherSourceFileGcc.cpp"
        "msvc":
        -   "./OtherSources/AnotherSourceFileMSVC.cpp"

IncludePaths:
    DefaultPlatform:
        DefaultProfile:
        -   "./OtherSources"

Defines:
    DefaultPlatform:
        # Turns into `TEST_DEF=\"Test Define Working\"` in shell
        DefaultProfile: ["TEST_DEF=\\\"Test Define Working\\\""]

Setup:
    Unix:
        DefaultProfile:
        -   "echo Setting up script... in $PWD"
    Windows:
        DefaultProfile:
        -   "echo Setting up script... in %cd%"

PreBuild:
    Unix:
        DefaultProfile:
        -   "echo Starting build... in $PWD"
    Windows:
        DefaultProfile:
        -   "echo Starting build... in %cd%"

PostBuild:
    Unix:
        DefaultProfile:
        -   "echo Build completed... in $PWD"
    Windows:
        DefaultProfile:
        -   "echo Build completed... in %cd%"

Cleanup:
    Unix:
        DefaultProfile:
        -   "echo Cleaning up script... in $PWD"
    Windows:
        DefaultProfile:
        -   "echo Cleaning up script... in %cd%"

Dependencies:
-   Name: ssLogger
    Platforms: [Windows, Linux, MacOS]
    Source:
        Git:
            URL: "https://github.com/Neko-Box-Coder/ssLogger.git"
    LibraryType: Shared
    IncludePaths: ["Include"]
    LinkProperties:
        DefaultPlatform:
            DefaultProfile:
                SearchLibraryNames: ["ssLogger"]
                ExcludeLibraryNames: ["ssLogger_SRC"]
                SearchDirectories: ["./build", "./build/Debug", "./build/Release"]
    Setup:
        DefaultPlatform:
            DefaultProfile:
            -   "mkdir build"
    Build:
        DefaultPlatform:
            DefaultProfile:
            -   "cd build && cmake .. -DssLOG_BUILD_TYPE=SHARED"
            -   "cd build && cmake --build . -j 16"
    FilesToCopy:
        # Target Platform (Default, Windows, Linux, MacOS, or Unix)
        DefaultPlatform:
            DefaultProfile:
            -  "./Include/ssLogger/ssLog.hpp"

-   Name: System2.cpp
    Source:
        ImportPath: "./TestImport.yaml"

# -   Name: System2.cpp
#     Platforms: [DefaultPlatform]
#     Source:
#         Git:
#             URL: "https://github.com/Neko-Box-Coder/System2.cpp.git"
#     LibraryType: Header
#     IncludePaths: [".", "./External/System2"]
#     Setup:
#         DefaultPlatform:
#             DefaultProfile:
#             -   "git submodule update --init --recursive"
*/


//#include "ssLogger/ssLogInit.hpp"

#define ssLOG_DLL 1
#include "ssLogger/ssLog.hpp"
#include "System2.hpp"

#if defined(__GNUC__)
    #include "AnotherSourceFileGcc.hpp"
#endif

#if defined(_MSC_VER)
    #include "AnotherSourceFileMSVC.hpp"
#endif

#include <iostream>
#include <chrono>

int main(int argc, char* argv[])
{
    std::cout << "Hello World" << std::endl;
    std::cout << TEST_DEF << std::endl;
    
    System2CommandInfo commandInfo = {};
    int returnCode = 0;
    
    #if defined(_WIN32)
        System2CppRun("dir", commandInfo);
    #else
        auto re = System2CppRun("ls -lah", commandInfo);
    #endif
    System2CppGetCommandReturnValueSync(commandInfo, returnCode, false);
    
    for(int i = 0; i < argc; ++i)
        std::cout << "Arg" << i << ": " << argv[i] << std::endl;
    
    #if ssLOG_USE_SOURCE
        ssLOG_LINE("Source dependency is working!!!");
    #else
        ssLOG_LINE("Header only dependency is working!!!");
    #endif

    ssLOG_LINE("Multi source file is working: " << TestFunc());

    int CheckCounter = 5;
    ssLOG_FATAL("Test fatal: " << CheckCounter);
    ssLOG_ERROR("Test error: " << CheckCounter);
    ssLOG_WARNING("Test warning: " << CheckCounter);
    ssLOG_INFO("Test info: " << CheckCounter);
    ssLOG_DEBUG("Test debug: " << CheckCounter);
 
    for(int i = 0; i < 100; ++i)
    {
        ssLOG_LINE("Logging test: " << i);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    return 0;
}
