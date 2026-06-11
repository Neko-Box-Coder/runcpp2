/* runcpp2
Parameters:
    LogText:
        Optional: false

Variables:
    LogSentence: "LogText is {LogText}"

Defines:
-   "_CRT_SECURE_NO_WARNINGS=1"
-   "LOG_SEN=\\\"{LogSentence}\\\""
# Uncomment this to log things below warning
# -   "ssLOG_LEVEL=5"

Dependencies:
-   Name: ssLogger
    Platforms: [DefaultPlatform]
    Source:
        Git:
            URL: "https://github.com/Neko-Box-Coder/ssLogger.git"
    LibraryType: Header
    IncludePaths: ["Include"]
*/


#include "ssLogger/ssLogInit.hpp"
#include "ssLogger/ssLog.hpp"

int main(int, char**)
{
    ssLOG_LINE("Let's log something with different log levels!");
    ssLOG_FATAL("Hello World");
    ssLOG_ERROR("Hello World");
    ssLOG_WARNING("Hello World");
    
    ssLOG_LINE(LOG_SEN);
    
    //You won't see the following logs since anything below warnings are evaluated to nothing
    ssLOG_INFO("Hello World");
    ssLOG_DEBUG("Hello World");
    
    return 0;
}
