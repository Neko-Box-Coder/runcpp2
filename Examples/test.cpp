/* runcpp2

Dependencies:
-   Name: ssLogger
    Platforms: [Windows, Linux, MacOS]
    Source:
        Type: Git
        Value: "https://github.com/Neko-Box-Coder/ssLogger.git"
    LibraryType: Header
    IncludePaths:
        - "Include"
    
    Setup:
        All:
            "g++":
            -   "git submodule update --init --recursive"
            -   "mkdir build"
            -   "cd build && cmake .."
            -   "cd build && cmake --build . -j 16"
*/


#include "ssLogger/ssLogInit.hpp"
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
