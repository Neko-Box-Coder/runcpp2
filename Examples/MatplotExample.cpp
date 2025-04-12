/* runcpp2

Dependencies:
-   Name: matplotplusplus
    Platforms: [DefaultPlatform]
    Source:
        Git:
            URL: "https://github.com/alandefreitas/matplotplusplus.git"
            Branch: "v1.2.2"
    LibraryType: Shared
    IncludePaths: ["./source", "./build/source/matplot"]
    LinkProperties:
        Unix:
            DefaultProfile:
                SearchLibraryNames: ["libmatplot.so.1"]
                ExcludeLibraryNames: ["libmatplot.so.1.2.0"]
                SearchDirectories: ["./build/source/matplot"]
        Windows:
            DefaultProfile:
                SearchLibraryNames: ["libmatplot"]
                SearchDirectories: ["./build/source/matplot/debug"]
    Setup:
    -   "mkdir build"
    -   "cd build && cmake .. -DMATPLOTPP_BUILD_WITH_SANITIZERS=OFF -DMATPLOTPP_BUILD_EXAMPLES=OFF -DMATPLOTPP_BUILD_INSTALLER=OFF -DMATPLOTPP_BUILD_PACKAGE=OFF -DBUILD_SHARED_LIBS=ON"
    -   "cd build && cmake --build . -j 16"*/

#include "matplot/matplot.h"

#include <cstdlib>
#include <iostream>

int main()
{
    //NOTE: Alternatively, you can put this under "Setup" section in runcpp2 build settings
#ifdef _WIN32
    if(system("which gnuplot") != 0)
#else
    if(system("where gnuplot") != 0)
#endif
    {
        std::cout << "Failed to find gnuplot, please install it on the system" << std::endl;
        return 1;
    }

    auto f1 = matplot::figure();
    matplot::plot(matplot::vector_1d{1., 2., 3.}, matplot::vector_1d{2., 4., 6.});


    auto f2 = matplot::figure();
    matplot::scatter(matplot::iota(1, 20), matplot::rand(20, 0, 1));

    matplot::show();
    return 0;
}
