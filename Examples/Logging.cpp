/* runcpp2
Dependencies:
-   Name: ssLogger
    Platforms: [DefaultPlatform]
    Source:
        Git:
            URL: "https://github.com/Neko-Box-Coder/ssLogger.git"
    LibraryType: Header
    IncludePaths: ["Include"]
    Defines:
    -   "_CRT_SECURE_NO_WARNINGS=1"
    # Uncomment this to log things below warning
    # -   "ssLOG_LEVEL=5"
*/


#include "ssLogger/ssLogInit.hpp"
#include "ssLogger/ssLog.hpp"

int main(int, char**)
{
    ssLOG_LINE("Let's log something with different log levels!");
    ssLOG_FATAL("Hello World");
    ssLOG_ERROR("Hello World");
    ssLOG_WARNING("Hello World");
    
    //You won't see the following logs since anything below warnings are evaluated to nothing
    ssLOG_INFO("Hello World");
    ssLOG_DEBUG("Hello World");
    
    return 0;
}
