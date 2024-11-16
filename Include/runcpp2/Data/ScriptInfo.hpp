#ifndef RUNCPP2_DATA_SCRIPT_INFO_HPP
#define RUNCPP2_DATA_SCRIPT_INFO_HPP

#include "runcpp2/Data/DependencyInfo.hpp"
#include "runcpp2/Data/ProfilesFlagsOverride.hpp"
#include "runcpp2/Data/ParseCommon.hpp"
#include "runcpp2/Data/ProfilesProcessPaths.hpp"
#include "runcpp2/Data/ProfilesDefines.hpp"
#include "runcpp2/Data/ProfilesCommands.hpp"

#include <string>
#include <vector>
#include <unordered_map>

namespace runcpp2
{
    namespace Data
    {
        class ScriptInfo
        {
            public:
                std::string Language;
                bool PassScriptPath = false;
                std::unordered_map<PlatformName, std::vector<ProfileName>> RequiredProfiles;
                
                std::unordered_map<PlatformName, ProfilesFlagsOverride> OverrideCompileFlags;
                std::unordered_map<PlatformName, ProfilesFlagsOverride> OverrideLinkFlags;
                
                std::unordered_map<PlatformName, ProfilesProcessPaths> OtherFilesToBeCompiled;
                std::unordered_map<PlatformName, ProfilesProcessPaths> IncludePaths;
                
                std::vector<DependencyInfo> Dependencies;
                
                std::unordered_map<PlatformName, ProfilesDefines> Defines;
                
                std::unordered_map<PlatformName, ProfilesCommands> Setup;
                std::unordered_map<PlatformName, ProfilesCommands> PreBuild;
                std::unordered_map<PlatformName, ProfilesCommands> PostBuild;
                std::unordered_map<PlatformName, ProfilesCommands> Cleanup;
                
                bool Populated = false;
                
                bool ParseYAML_Node(ryml::ConstNodeRef& node);
                std::string ToString(std::string indentation) const;
                bool Equals(const ScriptInfo& other) const;
        };
    }
}

#endif
