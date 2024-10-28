#ifndef RUNCPP2_DATA_PROFILE_HPP
#define RUNCPP2_DATA_PROFILE_HPP

#include "runcpp2/Data/ParseCommon.hpp"
#include "runcpp2/Data/FilesTypesInfo.hpp"
#include "runcpp2/Data/StageInfo.hpp"

#include "ryml.hpp"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace runcpp2
{
    namespace Data
    {
        class Profile
        {
            public:
                std::string Name;
                
                std::unordered_set<std::string> NameAliases;
                std::unordered_set<std::string> FileExtensions;
                std::unordered_set<std::string> Languages;
                std::unordered_map<PlatformName, std::vector<std::string>> Setup;
                std::unordered_map<PlatformName, std::vector<std::string>> Cleanup;
                FilesTypesInfo FilesTypes;
                
                StageInfo Compiler;
                StageInfo Linker;
                
                void GetNames(std::vector<std::string>& outNames) const;
                bool ParseYAML_Node(ryml::ConstNodeRef& profileNode);
                std::string ToString(std::string indentation) const;
                bool Equals(const Profile& other) const;
        };
    }
}

#endif
