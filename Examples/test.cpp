/* runcpp2

RequiredProfiles:
    Windows: ["msvc"]
    Unix: ["g++"]

OtherFilesToBeCompiled:
    # Target Platform
    Default:
        # Target Profile
        "g++":
        -   "./OtherSources/AnotherSourceFileGcc.cpp"
        Default:
        -   "./AnotherSourceFile.cpp"
        -   "./AnotherSourceFile2.cpp"

Defines:
    Default:
        # Turns into `TEST_DEF=\"Test Define Working\"` in shell
        "Default": ["TEST_DEF=\\\"Test Define Working\\\""]

Dependencies:
-   Name: ssLogger
    Platforms: [Windows, Linux, MacOS]
    Source:
        Type: Git
        Value: "https://github.com/Neko-Box-Coder/ssLogger.git"
    LibraryType: Shared
    IncludePaths: ["Include"]
    LinkProperties:
        "Default":
            SearchLibraryNames: ["ssLogger"]
            ExcludeLibraryNames: ["ssLogger_SRC"]
            SearchDirectories: ["./build", "./build/Debug", "./build/Release"]
    Setup:
        Default:
            "Default":
            -   "mkdir build"
    Build:
        Default:
            "Default":
            -   "cd build && cmake .. -DssLOG_BUILD_TYPE=SHARED"
            -   "cd build && cmake --build . --config Release -j 16"
    FilesToCopy:
        # Target Platform (Default, Windows, Linux, MacOS, or Unix)
        Default:
            "Default":
            -  "./Include/ssLogger/ssLog.hpp"
*/


//#include "ssLogger/ssLogInit.hpp"

#define ssLOG_DLL 1
#include "ssLogger/ssLog.hpp"

#include "./OtherSources/AnotherSourceFileGcc.hpp"

#include <iostream>
#include <chrono>

int main(int argc, char* argv[])
{
    std::cout << "Hello World" << std::endl;
    std::cout << TEST_DEF << std::endl;
    
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
