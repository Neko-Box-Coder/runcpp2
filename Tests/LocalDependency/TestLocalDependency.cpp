/* runcpp2

Dependencies:
-   Name: LocalTest
    Platforms: [Windows, Linux, MacOS]
    Source:
        Local:
            Path: ./LocalDependency
    LibraryType: Header
    IncludePaths: []
    Build:
        Windows:
            DefaultProfile: ["dir", "dir .\\NestedTestDirectory"]
        Unix:
            DefaultProfile: ["ls -lah", "cd ./NestedTestDirectory && ls -lah"]
*/


#include <iostream>

int main(int argc, char* argv[])
{
    return 0;
}
