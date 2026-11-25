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
