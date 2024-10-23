#ifndef RUNCPP2_PLATFORM_UTIL_HPP
#define RUNCPP2_PLATFORM_UTIL_HPP

#include "runcpp2/Data/ParseCommon.hpp"
#include "System2.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace ghc
{
    namespace filesystem
    {
        class path;
    }
}

namespace runcpp2
{
    std::string ProcessPath(const std::string& path);
    
    std::vector<std::string> GetPlatformNames();
    
    bool RunCommandAndGetOutput(const std::string& command, 
                                std::string& outOutput,
                                std::string runDirectory = "");
    
    bool RunCommandAndGetOutput(const std::string& command, 
                                std::string& outOutput, 
                                int& outReturnCode,
                                std::string runDirectory = "");
    
    #if defined(_WIN32)
        std::string GetWindowsError();
    #endif
    
    template <typename T>
    inline bool HasValueFromPlatformMap(const std::unordered_map<PlatformName, T>& map)
    {
        std::vector<std::string> platformNames = runcpp2::GetPlatformNames();
        
        for(int i = 0; i < platformNames.size(); ++i)
        {
            if(map.find(platformNames.at(i)) != map.end())
                return true;
        }
        
        return false;
    }
    
    template <typename T>
    inline const T* GetValueFromPlatformMap(const std::unordered_map<PlatformName, T>& map)
    {
        std::vector<std::string> platformNames = runcpp2::GetPlatformNames();
        
        for(int i = 0; i < platformNames.size(); ++i)
        {
            if(map.find(platformNames.at(i)) != map.end())
                return &map.at(platformNames[i]);
        }
        
        return nullptr;
    }
    
    std::string GetFileExtensionWithoutVersion(const ghc::filesystem::path& path);
}

#endif
