/* runcpp2

Dependencies:
-   Name: cpp-httplib
    Platforms: [DefaultPlatform]
    Source:
        Git:
            URL: "https://github.com/yhirose/cpp-httplib.git"
            Branch: "v0.20.0"
    LibraryType: Header
    IncludePaths:
    -   "./"
*/

#include "httplib.h"
#include <iostream>

int main()
{
    httplib::Server svr;
    if(!svr.set_mount_point("/", "./"))
        return 1;
    
    svr.set_file_extension_and_mimetype_mapping("cpp", "text/x-c");
    std::cout << "Running on localhost:8080" << std::endl;
    svr.listen("0.0.0.0", 8080);
    
    return 0;
}

