/* runcpp2

Dependencies:
-   Name: SDL2
    Platforms: [DefaultPlatform]
    Source:
        Git:
            URL: "https://github.com/libsdl-org/SDL.git"
            Branch: "release-2.32.6"
    LibraryType: Shared
    IncludePaths:
    -   "./include"
    LinkProperties:
        SearchLibraryNames: ["SDL2"]
        ExcludeLibraryNames: ["SDL2-static", "SDL2_test"] # On Windows
        SearchDirectories: ["./build", "./build/debug"]
    Setup: 
        DefaultPlatform:
            "g++": 
            -   "mkdir build"
            -   "cd build && cmake .. -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ && \
                cmake --build . -j 8"
            "msvc": 
            -   "mkdir build"
            -   "cd build && cmake .. -G \"Visual Studio 17 2022\" && cmake --build . -j 8"

# Or use system installed SDL on Unix...
# OverrideLinkFlags:
#     Unix:
#         "g++":
#             Remove: ""
#             Append: "-lSDL2"

*/


#include "SDL.h"

#include <iostream>

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

//Credits:  https://github.com/AlmasB/SDL2-Demo/blob/master/src/Main.cpp
//          https://github.com/aminosbh/basic-cpp-sdl-project/blob/master/src/main.cc
int main(int arc, char ** argv) 
{

    SDL_Window* window = nullptr;
    
    if (SDL_Init( SDL_INIT_VIDEO ) < 0)
        std::cout << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
    else
    {
        window = SDL_CreateWindow(  "SDL2 Demo",
                                    SDL_WINDOWPOS_CENTERED, 
                                    SDL_WINDOWPOS_CENTERED,
                                    SCREEN_WIDTH, 
                                    SCREEN_HEIGHT,
                                    SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN);
    }
    
    if(!window)
        return 1;
    
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if(!renderer)
        return 1;
    
    SDL_Rect squareRect;
    
    //Square dimensions: Half of the min(SCREEN_WIDTH, SCREEN_HEIGHT)
    squareRect.w = std::min(SCREEN_WIDTH, SCREEN_HEIGHT) / 2;
    squareRect.h = std::min(SCREEN_WIDTH, SCREEN_HEIGHT) / 2;
    
    //Square position: In the middle of the screen
    squareRect.x = SCREEN_WIDTH / 2 - squareRect.w / 2;
    squareRect.y = SCREEN_HEIGHT / 2 - squareRect.h / 2;
    
    bool quit = false;
    SDL_Event e;
    
    while (!quit) 
    {
        while (SDL_PollEvent(&e) != 0) 
        {
            if (e.type == SDL_QUIT)
                quit = true;
            
            // Initialize renderer color white for the background
            SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
            SDL_RenderClear(renderer);
            // Set renderer color red to draw the square
            SDL_SetRenderDrawColor(renderer, 0xFF, 0x00, 0x00, 0xFF);
            SDL_RenderFillRect(renderer, &squareRect);
            SDL_RenderPresent(renderer);
        }
    }
    
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
