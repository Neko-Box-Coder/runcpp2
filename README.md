## 🏃‍♂️ runcpp2
A cross-platform tool that can let you run any c++ file as a script, just like python!

Run c++ files anytime, anywhere.

### Prerequisites
- A C++ compiler, currently only tested with g++

TODO: Support for other compilers

### Installation
1. Clone the repository with `git clone --recursive https://github.com/Neko-Box-Coder/runcpp2.git`
2. Run `Build.sh` or `Build.bat` to build

TODO: Ability to use pre-built binary

### How to use

#### Single C/C++ File
Just run `runcpp2 <filename>.cpp <arguments...>` and it will execute the file just like any other script.

#### C/C++ File with External Libraries

TODO: Detail documentations

<details>
<summary>Sample C++ file</summary>

```c++

/* runcpp2

OverrideCompileFlags:
    # Profile with the respective flags to override
    "g++":
        # Flags to be removed from the default compile flags, separated by space
        # Remove: "-Werror"

Dependencies:
-   Name: ssLogger
    Platforms: [Windows, Linux, MacOS]
    Source:
        Type: Git
        Value: "https://github.com/Neko-Box-Coder/ssLogger.git"
    LibraryType: Static
    # LibraryType: Header
    IncludePaths:
        - "Include"
    
    # (Optional if LibraryType is Header) Link properties of the dependency
    LinkProperties:
        # Properties for searching the library binary for the profile
        "g++":
            # The library names to be searched for when linking against the script
            SearchLibraryNames: ["ssLogger"]
            
            # (Optional) The library names to be excluded from being searched
            ExcludeLibraryNames: ["ssLogger_SRC"]
            
            # The path (relative to the dependency folder) to be searched for the dependency binaries
            SearchDirectories: ["./build"]
            
            # (Optional) Additional link options for this dependency
            AdditionalLinkOptions: 
                All: []

    
    # (Optional) List of setup commands for the supported platforms
    Setup:
        # Setup commands for the specified platform
        All:
            # Setup commands for the specified profile. 
            # All commands are run in the dependency folder
            "g++":
            -   "git submodule update --init --recursive"
            -   "mkdir build"
            -   "cd build && cmake .."
            -   "cd build && cmake --build . -j 16"
*/



#include "ssLogger/ssLogSwitches.hpp"

#define ssLOG_USE_SOURCE 1
#if !ssLOG_USE_SOURCE
    #include "ssLogger/ssLogInit.hpp"
#endif
 
#include "ssLogger/ssLog.hpp"

#include <iostream>

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
    
    return 0;
}

```


</details>

