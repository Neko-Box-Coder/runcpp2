/* runcpp2

RequiredProfiles:
    Windows: ["msvc"]
    Unix: ["g++"]
Dependencies:
-   Name: ssLogger
    Platforms: [Windows, Linux, MacOS]
    Source:
        Type: Git
        Value: "https://github.com/Neko-Box-Coder/ssLogger.git"
    LibraryType: Shared
    IncludePaths: ["Include"]
    LinkProperties:
        "All":
            SearchLibraryNames: ["ssLogger"]
            ExcludeLibraryNames: ["ssLogger_SRC"]
            SearchDirectories: ["./build", "./build/Debug", "./build/Release"]
    Setup:
        All:
            "All":
            -   "mkdir build"
    Build:
        All:
            "All":
            -   "cd build && cmake .. -DssLOG_BUILD_TYPE=SHARED"
            -   "cd build && cmake --build . --config Release -j 16"
*/


//#include "ssLogger/ssLogInit.hpp"

#define ssLOG_DLL 1
#include "ssLogger/ssLog.hpp"

#include <iostream>
#include <chrono>

int main(int argc, char* argv[])
{
    std::cout << "Hello World" << std::endl;
    
    for(int i = 0; i < argc; ++i)
        std::cout << "Arg" << i << ": " << argv[i] << std::endl;
    
    #if ssLOG_USE_SOURCE
        ssLOG_LINE("Source dependency is working!!!");
    #else
        ssLOG_LINE("Header only dependency is working!!!");
    #endif

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
