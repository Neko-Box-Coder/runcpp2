#ifndef RUNCPP2_DATA_PROFILES_COMPILES_FILES_HPP
#define RUNCPP2_DATA_PROFILES_COMPILES_FILES_HPP

#include "runcpp2/Data/ParseCommon.hpp"

#include "runcpp2/YamlLib.hpp"

#define NOMINMAX 1
#include "ghc/filesystem.hpp"

#include <unordered_map>

namespace runcpp2
{
    namespace Data
    {
        class ProfilesCompilesFiles
        {
            public:
                std::unordered_map<ProfileName, std::vector<ghc::filesystem::path>> CompilesFiles;
                
                bool ParseYAML_Node(ryml::ConstNodeRef& node);
                std::string ToString(std::string indentation) const;
                bool Equals(const ProfilesCompilesFiles& other) const;
        };
    }
}

#endif
