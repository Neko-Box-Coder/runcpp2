#ifndef RUNCPP2_DATA_PROFILE_HPP
#define RUNCPP2_DATA_PROFILE_HPP

#include "runcpp2/Data/CompilerInfo.hpp"
#include "runcpp2/Data/LinkerInfo.hpp"
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
                
                //TODO: Add a function for getting value from map that uses profile name as key
                //      Or maybe just a function that return all the names
                
                std::unordered_set<std::string> NameAliases;
                std::unordered_set<std::string> FileExtensions;
                std::unordered_set<std::string> Languages;
                std::unordered_map<PlatformName, std::vector<std::string>> Setup;
                std::unordered_map<PlatformName, std::vector<std::string>> Cleanup;
                FilesTypesInfo FilesTypes;
                
                StageInfo Compiler;
                StageInfo Linker;
                
                bool ParseYAML_Node(ryml::ConstNodeRef& profileNode);
                std::string ToString(std::string indentation) const;
        };
    }
}

#endif
