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
