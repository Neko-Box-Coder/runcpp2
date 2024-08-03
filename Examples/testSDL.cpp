/* runcpp2

OverrideLinkFlags:
    Unix:
        "g++":
            Remove: ""
            Append: "-lSDL2"

*/


#include <SDL2/SDL.h>

#include <iostream>

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

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
                                    SDL_WINDOW_SHOWN);
    }
    
    bool quit = false;
    SDL_Event e;
    
    while (!quit) 
    {
        while (SDL_PollEvent(&e) != 0) 
        {
            if (e.type == SDL_QUIT)
                quit = true;
        }
    }
    
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
