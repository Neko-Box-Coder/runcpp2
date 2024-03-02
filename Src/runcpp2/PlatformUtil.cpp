#include "runcpp2/PlatformUtil.hpp"

#include <stdio.h>

namespace runcpp2
{

namespace Internal
{
    char GetAltFileSystemSeparator()
    {
        #ifdef _WIN32
            return '/';
        #elif __unix__
            return '\0';
        #else
            return '\0';
        #endif
    }
    
    char GetFileSystemSeparator()
    {
        #ifdef _WIN32
            return '\\';
        #elif __unix__
            return '/';
        #else
            return '/';
        #endif
    }

    std::string ProcessPath(const std::string& path)
    {
        std::string processedScriptPath;
        
        char altSeparator = GetAltFileSystemSeparator();
        
        //Convert alternative slashes to native slashes
        if(altSeparator != '\0')
        {
            char separator = GetFileSystemSeparator();
            
            processedScriptPath = path;
            for(int i = 0; i < processedScriptPath.size(); ++i)
            {
                if(processedScriptPath[i] == altSeparator)
                    processedScriptPath[i] = separator;
            }
            
            return processedScriptPath;
        }
        else
            return path;
    }

    std::vector<std::string> GetPlatformNames()
    {
        #ifdef _WIN32
            return {"Windows", "All"};
        #elif __linux__
            return {"Linux", "Unix", "All"};
        #elif __APPLE__
            return {"MacOS", "Unix", "All"};
        #elif __unix__
            return {"Unix", "All"};
        #else
            return {"Unknown", "All"};
        #endif
    }
}

}