/* runcpp2

OverrideCompileFlags:
    Windows:
        msvc:
            Append: "/wd4251"
Defines: ["MATPLOT_BUILD_HIGH_RESOLUTION_WORLD_MAP"]
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
        SearchLibraryNames: ["matplot"]
        ExcludeLibraryNames: ["libmatplot.so.1.2.0"]
        SearchDirectories: ["./build/source/matplot", "./build/source/matplot/debug"]
    Setup:
        DefaultPlatform:
            "g++":
            -   "mkdir build"
            -   "cd build && cmake .. -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ \
                -DMATPLOTPP_BUILD_WITH_SANITIZERS=OFF -DMATPLOTPP_BUILD_EXAMPLES=OFF \
                -DMATPLOTPP_BUILD_INSTALLER=OFF -DMATPLOTPP_BUILD_PACKAGE=OFF -DBUILD_SHARED_LIBS=ON"
            -   "cd build && cmake --build . -j 8"
            "msvc":
            -   "mkdir build"
            -   "cd build && cmake .. -G \"Visual Studio 17 2022\" \
                -DMATPLOTPP_BUILD_WITH_SANITIZERS=OFF -DMATPLOTPP_BUILD_EXAMPLES=OFF \
                -DMATPLOTPP_BUILD_INSTALLER=OFF -DMATPLOTPP_BUILD_PACKAGE=OFF -DBUILD_SHARED_LIBS=ON"
            -   "cd build && cmake --build . -j 8"
*/

#include "matplot/matplot.h"

#include <cstdlib>
#include <iostream>

int main()
{
    //NOTE: Alternatively, you can put this under "Setup" section in runcpp2 build settings
#ifdef _WIN32
    if(system("where gnuplot") != 0)
#else
    if(system("which gnuplot") != 0)
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
