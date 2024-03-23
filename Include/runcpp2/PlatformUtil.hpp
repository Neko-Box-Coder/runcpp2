#ifndef RUNCPP2_PLATFORM_UTIL_HPP
#define RUNCPP2_PLATFORM_UTIL_HPP

#include "runcpp2/Data/ParseCommon.hpp"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace runcpp2
{
    std::string ProcessPath(const std::string& path);
    
    std::vector<std::string> GetPlatformNames();
    
    template <typename T>
    inline bool HasValueFromPlatformMap(const std::unordered_map<PlatformName, T>& map)
    {
        std::vector<std::string> platformNames = runcpp2::GetPlatformNames();
        
        for(int i = platformNames.size() - 1; i >= 0; --i)
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
        
        for(int i = platformNames.size() - 1; i >= 0; --i)
        {
            if(map.find(platformNames.at(i)) != map.end())
                return &map.at(platformNames[i]);
        }
        
        return nullptr;
    }
}

#endif