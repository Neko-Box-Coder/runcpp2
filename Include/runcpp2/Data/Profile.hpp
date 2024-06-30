#ifndef RUNCPP2_DATA_PROFILE_HPP
#define RUNCPP2_DATA_PROFILE_HPP

#include "runcpp2/Data/CompilerInfo.hpp"
#include "runcpp2/Data/LinkerInfo.hpp"
#include "runcpp2/Data/ParseCommon.hpp"
#include "runcpp2/Data/FileProperties.hpp"

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
                std::unordered_map<PlatformName, std::vector<std::string>> SetupSteps;
                FileProperties ObjectLinkFile;
                FileProperties SharedLinkFile;
                FileProperties SharedLibraryFile;
                FileProperties StaticLinkFile;
                FileProperties DebugSymbolFile;
                
                CompilerInfo Compiler;
                LinkerInfo Linker;
                
                bool ParseYAML_Node(ryml::ConstNodeRef& profileNode);
                std::string ToString(std::string indentation) const;
        };
    }
}

#endif
