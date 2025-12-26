#ifndef RUNCPP2_DATA_SCRIPT_INFO_HPP
#define RUNCPP2_DATA_SCRIPT_INFO_HPP

#include "runcpp2/Data/DependencyInfo.hpp"
#include "runcpp2/Data/ProfilesFlagsOverride.hpp"
#include "runcpp2/Data/ProfilesProcessPaths.hpp"
#include "runcpp2/Data/ProfilesDefines.hpp"
#include "runcpp2/Data/ProfilesCommands.hpp"
#include "runcpp2/Data/BuildType.hpp"

#define RUNCPP2_CURRENT_CLASS_NAME ScriptInfo
#include "runcpp2/MacroUtil.hpp"

#if !defined(NOMINMAX)
    #define NOMINMAX 1
#endif

#include "ghc/filesystem.hpp"

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
                RUNCPP2_FIELD_BEGIN();
                
                RUNCPP2_FIELD std::string Language;
                RUNCPP2_FIELD bool PassScriptPath = false;
                RUNCPP2_FIELD BuildType CurrentBuildType = BuildType::EXECUTABLE;
                RUNCPP2_FIELD std::unordered_map<   PlatformName, 
                                                    std::vector<ProfileName>> RequiredProfiles;
                RUNCPP2_FIELD std::unordered_map<   PlatformName, 
                                                    ProfilesFlagsOverride> OverrideCompileFlags;
                RUNCPP2_FIELD std::unordered_map<   PlatformName, 
                                                    ProfilesFlagsOverride> OverrideLinkFlags;
                RUNCPP2_FIELD std::unordered_map<   PlatformName, 
                                                    ProfilesProcessPaths> OtherFilesToBeCompiled;
                RUNCPP2_FIELD std::unordered_map<   PlatformName, 
                                                    ProfilesProcessPaths> IncludePaths;
                RUNCPP2_FIELD std::vector<DependencyInfo> Dependencies;
                RUNCPP2_FIELD std::unordered_map<PlatformName, ProfilesDefines> Defines;
                RUNCPP2_FIELD std::unordered_map<PlatformName, ProfilesCommands> Setup;
                RUNCPP2_FIELD std::unordered_map<PlatformName, ProfilesCommands> PreBuild;
                RUNCPP2_FIELD std::unordered_map<PlatformName, ProfilesCommands> PostBuild;
                RUNCPP2_FIELD std::unordered_map<PlatformName, ProfilesCommands> Cleanup;
                
                static constexpr int FieldsCount = RUNCPP2_FIELD_COUNT;
                
                //Internal tracking
                bool Populated = false;
                
                ghc::filesystem::file_time_type LastWriteTime = 
                    ghc::filesystem::file_time_type::min();
                
                bool ParseYAML_Node(ryml::ConstNodeRef node);
                bool ParseYAML_Node(YAML::ConstNodePtr node);
                
                std::string ToString(std::string indentation) const;
                bool IsAllCompiledCacheInvalidated(const ScriptInfo& other) const;
                bool Equals(const ScriptInfo& other) const;
        };
    }
}

#endif
