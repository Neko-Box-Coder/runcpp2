/* runcpp2

# (Optional) The list of dependencies needed by the script
Dependencies:
    # Dependency name
-   Name: SDL2
    
    # Supported platforms of the dependency
    Platforms: [Unix]
    
    # The source of getting the dependency
    Source:
        Type: Local
        Value: "./dummy"
    
    # Library Type (Static, Object, Shared, Header)
    LibraryType: Shared
    
    # (Optional if LibraryType is Header) Link properties of the dependency
    LinkProperties:
        # Properties for searching the library binary for the profile
        "g++":
            # The library names to be searched for when linking against the script
            SearchLibraryNames: []
            
            # (Optional) The library names to be excluded from being searched
            ExcludeLibraryNames: []
            
            # The path (relative to the dependency folder) to be searched for the dependency binaries
            SearchDirectories: []
            
            # (Optional) Additional link options for this dependency
            AdditionalLinkOptions: 
                # Applies to all platforms (Can be changed to Linux, MacOS, Windows, Unix, etc.)
                Unix: ["-lSDL2"]
*/


#include <SDL2/SDL.h>

#include <iostream>

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

int main(int arc, char ** argv) 
{

    SDL_Window* window = nullptr;
    
    if (SDL_Init( SDL_INIT_VIDEO ) < 0) {
        std::cout << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
    } else {
        
        window = SDL_CreateWindow(
            "SDL2 Demo",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            SCREEN_WIDTH, SCREEN_HEIGHT,
            SDL_WINDOW_SHOWN
        );
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
