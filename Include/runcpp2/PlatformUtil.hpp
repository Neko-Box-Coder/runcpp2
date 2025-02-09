#ifndef RUNCPP2_PLATFORM_UTIL_HPP
#define RUNCPP2_PLATFORM_UTIL_HPP

#include "runcpp2/Data/ParseCommon.hpp"
#include "runcpp2/Data/Profile.hpp"

#if !defined(NOMINMAX)
    #define NOMINMAX 1
#endif

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
    
    bool RunCommand(const std::string& command, 
                    const bool& captureOutput,
                    const std::string& runDirectory, 
                    std::string& outOutput, 
                    int& outReturnCode);
    
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
    
    template <typename T>
    inline bool HasValueFromProfileMap( const Data::Profile& profile,
                                        const std::unordered_map<ProfileName, T>& map)
    {
        std::vector<std::string> profileNames;
        profile.GetNames(profileNames);
        
        for(int i = 0; i < profileNames.size(); ++i)
        {
            if(map.find(profileNames.at(i)) != map.end())
                return true;
        }
        return false;
    }
    
    template <typename T>
    inline const T* GetValueFromProfileMap( const Data::Profile& profile,
                                            const std::unordered_map<ProfileName, T>& map)
    {
        std::vector<std::string> profileNames;
        profile.GetNames(profileNames);
        
        for(int i = 0; i < profileNames.size(); ++i)
        {
            auto it = map.find(profileNames.at(i));
            if(it != map.end())
                return &it->second;
        }
        return nullptr;
    }
}

#endif
