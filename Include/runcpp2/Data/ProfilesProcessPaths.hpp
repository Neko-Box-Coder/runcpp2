#ifndef RUNCPP2_DATA_PROFILES_PROCESS_PATHS_HPP
#define RUNCPP2_DATA_PROFILES_PROCESS_PATHS_HPP

#include "runcpp2/Data/ParseCommon.hpp"

#include "runcpp2/YamlLib.hpp"

#if !defined(NOMINMAX)
    #define NOMINMAX 1
#endif
#include "ghc/filesystem.hpp"

#include <unordered_map>

namespace runcpp2
{
    namespace Data
    {
        class ProfilesProcessPaths
        {
            public:
                std::unordered_map<ProfileName, std::vector<ghc::filesystem::path>> Paths;
                
                bool ParseYAML_Node(ryml::ConstNodeRef& node);
                std::string ToString(std::string indentation) const;
                bool Equals(const ProfilesProcessPaths& other) const;
        };
    }
}

#endif
