#ifndef RUNCPP2_DATA_SCRIPT_INFO_HPP
#define RUNCPP2_DATA_SCRIPT_INFO_HPP

#include "runcpp2/Data/DependencyInfo.hpp"
#include "runcpp2/Data/ProfilesFlagsOverride.hpp"
#include "runcpp2/Data/ProfilesProcessPaths.hpp"
#include "runcpp2/Data/ProfilesDefines.hpp"
#include "runcpp2/Data/ProfilesCommands.hpp"
#include "runcpp2/Data/BuildType.hpp"
#include "runcpp2/Data/ParameterValue.hpp"
#include "runcpp2/ParameterUtil.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "runcpp2/Data/ParseCommon.hpp"
#include "runcpp2/LibYAML_Wrapper.hpp"
#include "runcpp2/DeferUtil.hpp"

#if !defined(NOMINMAX)
    #define NOMINMAX 1
#endif

#include "mpark/variant.hpp"
#include "DSResult/DSResult.hpp"
#include "ghc/filesystem.hpp"
#include "ssLogger/ssLog.hpp"

#include <string>
#include <vector>
#include <unordered_map>
#include <stddef.h>
#include <cctype>
#include <chrono>
#include <utility>
#include <deque>

namespace runcpp2
{
namespace Data
{
    struct ScriptInfo
    {
        std::string Language;
        bool PassScriptPath = false;
        BuildType CurrentBuildType = BuildType::EXECUTABLE;
        std::unordered_map<PlatformName, std::vector<ProfileName>> RequiredProfiles;
        std::unordered_map<std::string, ParameterValue> Parameters;
        std::unordered_map<std::string, std::string> Variables;
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
        
        //Internal tracking
        bool Populated = false;
        ghc::filesystem::file_time_type LastWriteTime = ghc::filesystem::file_time_type::min();
        
        inline DS::Result<void> 
        ParseYAML_Node( YAML::ConstNodePtr node, 
                        const std::unordered_map<std::string, std::string>& inputParameters)
        {
            ssLOG_FUNC_DEBUG();
            
            ParseParametersAndVariables(*this, node).DS_TRY();
            
            //Clone and modify the yaml node
            YAML::ResourceHandle resourceHandle;
            YAML::NodePtr clonedNode = node->Clone(false, resourceHandle).DS_TRY();
            DEFER { YAML::FreeYAMLResource(resourceHandle); };
            
            std::vector<YAML::ConstNodePtr> exclusions;
            if(ExistAndHasChild(clonedNode, "Dependencies"))
            {
                YAML::ConstNodePtr keyNode = clonedNode->GetMapKeyNode("Dependencies");
                YAML::ConstNodePtr valueNode = clonedNode->GetMapValueNode("Dependencies");
                DS_ASSERT_TRUE(keyNode != nullptr);
                DS_ASSERT_TRUE(valueNode != nullptr);
                exclusions.push_back(keyNode);
                exclusions.push_back(valueNode);
            }
            
            ApplyParametersAndVariables(*this, 
                                        clonedNode, 
                                        resourceHandle, 
                                        inputParameters,
                                        exclusions).DS_TRY();
            
            std::vector<NodeRequirement> requirements =
            {
                NodeRequirement("PassScriptPath", YAML::NodeType::Scalar, false, true),
                NodeRequirement("Language", YAML::NodeType::Scalar, false, true),
                NodeRequirement("BuildType", YAML::NodeType::Scalar, false, true),
                NodeRequirement("RequiredProfiles", YAML::NodeType::Map, false, true),
                
                //Expecting either platform profile map or remove append map
                NodeRequirement("OverrideCompileFlags", YAML::NodeType::Map, false, true),
                
                //Expecting either platform profile map or remove append map
                NodeRequirement("OverrideLinkFlags", YAML::NodeType::Map, false, true),
                
                //OtherFilesToBeCompiled can be platform profile map or sequence of paths, handle later
                //IncludePaths can be platform profile map or sequence of paths, handle later
                
                NodeRequirement("Dependencies", YAML::NodeType::Sequence, false, true)
                
                //Defines can be platform profile map or sequence of defines, handle later
                //Setup can be platform profile map or sequence of commands, handle later
                //PreBuild can be platform profile map or sequence of commands, handle later
                //PostBuild can be platform profile map or sequence of commands, handle later
                //Cleanup can be platform profile map or sequence of commands, handle later
            };
            
            if(!CheckNodeRequirements(clonedNode, requirements))
                return DS_ERROR_MSG("ScriptInfo: Failed to meet requirements");
            
            if(ExistAndHasChild(clonedNode, "PassScriptPath"))
            {
                std::string passScriptPathStr = 
                    clonedNode->GetMapValueScalar<std::string>("PassScriptPath").DS_TRY();
                for(size_t i = 0; i < passScriptPathStr.length(); ++i)
                    passScriptPathStr[i] = std::tolower(passScriptPathStr[i]);
                
                if(passScriptPathStr == "true" || passScriptPathStr == "1")
                    PassScriptPath = true;
                else if(passScriptPathStr == "false" || passScriptPathStr == "0")
                    PassScriptPath = false;
                else
                {
                    return DS_ERROR_MSG("ScriptInfo: Invalid value for PassScriptPath: " + 
                                        passScriptPathStr + "\n" +
                                        "Expected true/false or 1/0");
                }
            }
            
            if(ExistAndHasChild(clonedNode, "Language"))
            {
                Language = clonedNode->GetMapValueScalar<std::string>("Language").DS_TRY();
            }
            
            if(ExistAndHasChild(clonedNode, "BuildType"))
            {
                std::string typeStr = clonedNode->GetMapValueScalar<std::string>("BuildType").DS_TRY();
                BuildType buildType = StringToBuildType(typeStr);
                if(buildType == BuildType::COUNT)
                    return DS_ERROR_MSG("ScriptInfo: Invalid build type: " + typeStr);
                CurrentBuildType = buildType;
            }
            
            if(ExistAndHasChild(clonedNode, "RequiredProfiles"))
            {
                YAML::ConstNodePtr requiredProfilesNode = 
                    clonedNode->GetMapValueNode("RequiredProfiles");
                for(int i = 0; i < requiredProfilesNode->GetChildrenCount(); ++i)
                {
                    PlatformName platform = requiredProfilesNode->GetMapKeyScalarAt<std::string>(i)
                                                                .DS_TRY();
                    std::vector<ProfileName> profiles;
                    YAML::ConstNodePtr platformNode = requiredProfilesNode->GetMapValueNodeAt(i);
                    for(int j = 0; j < platformNode->GetChildrenCount(); ++j)
                    {
                        std::string profile = platformNode  ->GetSequenceChildScalar<std::string>(j)
                                                            .DS_TRY();
                        profiles.push_back(profile);
                    }
                    
                    RequiredProfiles[platform] = profiles;
                }
            }
            
            //TODO: Use DS::Result for ParsePlatformProfileMap
            DS_ASSERT_TRUE(ParsePlatformProfileMap<ProfilesFlagsOverride>(  clonedNode, 
                                                                            "OverrideCompileFlags", 
                                                                            OverrideCompileFlags, 
                                                                            "OverrideCompileFlags"));
            DS_ASSERT_TRUE(ParsePlatformProfileMap<ProfilesFlagsOverride>(  clonedNode, 
                                                                            "OverrideLinkFlags", 
                                                                            OverrideLinkFlags, 
                                                                            "OverrideLinkFlags"));
            DS_ASSERT_TRUE(ParsePlatformProfileMap<ProfilesProcessPaths>(   clonedNode, 
                                                                            "OtherFilesToBeCompiled", 
                                                                            OtherFilesToBeCompiled, 
                                                                            "OtherFilesToBeCompiled"));
            DS_ASSERT_TRUE(ParsePlatformProfileMap<ProfilesProcessPaths>(   clonedNode, 
                                                                            "SourceFiles", 
                                                                            OtherFilesToBeCompiled, 
                                                                            "SourceFiles"));
            DS_ASSERT_TRUE(ParsePlatformProfileMap<ProfilesProcessPaths>(   clonedNode, 
                                                                            "IncludePaths", 
                                                                            IncludePaths, 
                                                                            "IncludePaths"));
            if(ExistAndHasChild(clonedNode, "Dependencies"))
            {
                YAML::ConstNodePtr dependenciesNode = clonedNode->GetMapValueNode("Dependencies");
                for(int i = 0; i < dependenciesNode->GetChildrenCount(); ++i)
                {
                    DependencyInfo info;
                    YAML::ConstNodePtr dependencyNode = dependenciesNode->GetSequenceChildNode(i);
                    info.ParseYAML_Node(dependencyNode, inputParameters).DS_TRY_ACT
                    (
                        DS_TMP_ERROR.Message += "\nScriptInfo: Failed to parse DependencyInfo at "
                                                "index " + DS_STR(i);
                        DS_APPEND_TRACE(DS_TMP_ERROR);
                        return DS::Error(DS_TMP_ERROR)
                    );
                    Dependencies.push_back(info);
                }
            }
            
            //TODO: We should check if there's any unsubstituted 
            
            DS_ASSERT_TRUE(ParsePlatformProfileMap<ProfilesDefines>(clonedNode, 
                                                                    "Defines", 
                                                                    Defines, 
                                                                    "Defines"));
            DS_ASSERT_TRUE(ParsePlatformProfileMap<ProfilesCommands>(   clonedNode, 
                                                                        "Setup", 
                                                                        Setup, 
                                                                        "Setup"));
            DS_ASSERT_TRUE(ParsePlatformProfileMap<ProfilesCommands>(   clonedNode, 
                                                                        "PreBuild", 
                                                                        PreBuild, 
                                                                        "PreBuild"));
            DS_ASSERT_TRUE(ParsePlatformProfileMap<ProfilesCommands>(   clonedNode, 
                                                                        "PostBuild", 
                                                                        PostBuild, 
                                                                        "PostBuild"));
            DS_ASSERT_TRUE(ParsePlatformProfileMap<ProfilesCommands>(   clonedNode, 
                                                                        "Cleanup", 
                                                                        Cleanup, 
                                                                        "Cleanup"));
            return {};
        }

        //TODO: Text escaping?
        inline DS::Result<std::string> ToString(std::string indentation) const
        {
            std::string out;
            
            out += indentation + "PassScriptPath: " + (PassScriptPath ? "true" : "false") + "\n";
            
            if(!Language.empty())
                out += indentation + "Language: " + GetEscapedYAMLString(Language) + "\n";
            
            out += indentation + "BuildType: " + BuildTypeToString(CurrentBuildType) + "\n";
            
            if(!RequiredProfiles.empty())
            {
                out += indentation + "RequiredProfiles:\n";
                for(auto it = RequiredProfiles.begin(); it != RequiredProfiles.end(); ++it)
                {
                    if(it->second.empty())
                        out += indentation + "    " + it->first + ": []\n";
                    else
                    {
                        out += indentation + "    " + it->first + ":\n";
                        for(int i = 0; i < it->second.size(); ++i)
                        {
                            out +=  indentation + "    -   " + 
                                    GetEscapedYAMLString(it->second[i]) + "\n";
                        }
                    }
                }
            }
            
            if(!Parameters.empty())
            {
                out += indentation + "Parameters:\n";
                for(auto it = Parameters.begin(); it != Parameters.end(); ++it)
                {
                    out += indentation + "    " + it->first + ":\n";
                    std::string paramerterStr = it->second.ToString(indentation + "        ").DS_TRY();
                    out += paramerterStr;
                }
            }
            
            if(!Variables.empty())
            {
                out += indentation + "Variables:\n";
                for(auto it = Variables.begin(); it != Variables.end(); ++it)
                {
                    out += indentation + "    " + it->first + ": \"";
                    out += it->second + "\"\n";
                }
            }
            
            if(!OverrideCompileFlags.empty())
            {
                out += indentation + "OverrideCompileFlags:\n";
                for(auto it = OverrideCompileFlags.begin(); it != OverrideCompileFlags.end(); ++it)
                {
                    out += indentation + "    " + it->first + ":\n";
                    out += it->second.ToString(indentation + "        ");
                }
            }
            
            if(!OverrideLinkFlags.empty())
            {
                out += indentation + "OverrideLinkFlags:\n";
                for(auto it = OverrideLinkFlags.begin(); it != OverrideLinkFlags.end(); ++it)
                {
                    out += indentation + "    " + it->first + ":\n";
                    out += it->second.ToString(indentation + "        ");
                }
            }
            
            if(!OtherFilesToBeCompiled.empty())
            {
                out += indentation + "SourceFiles:\n";
                for(auto it = OtherFilesToBeCompiled.begin(); 
                    it != OtherFilesToBeCompiled.end(); 
                    ++it)
                {
                    out += indentation + "    " + it->first + ":\n";
                    out += it->second.ToString(indentation + "        ");
                }
            }

            if(!IncludePaths.empty())
            {
                out += indentation + "IncludePaths:\n";
                for(auto it = IncludePaths.begin(); it != IncludePaths.end(); ++it)
                {
                    out += indentation + "    " + it->first + ":\n";
                    out += it->second.ToString(indentation + "        ");
                }
            }
            
            if(!Dependencies.empty())
            {
                out += indentation + "Dependencies:\n";
                for(int i = 0; i < Dependencies.size(); ++i)
                {
                    int currentOutSize = out.size();
                    out += Dependencies[i].ToString(indentation + "    ");
                    
                    //Change character to yaml list
                    out.at(currentOutSize + indentation.size()) = '-';
                }
            }
            
            if(!Defines.empty())
            {
                out += indentation + "Defines:\n";
                for(auto it = Defines.begin(); it != Defines.end(); ++it)
                {
                    out += indentation + "    " + it->first + ":\n";
                    out += it->second.ToString(indentation + "        ");
                }
            }
            
            if(!Setup.empty())
            {
                out += indentation + "Setup:\n";
                for(auto it = Setup.begin(); it != Setup.end(); ++it)
                {
                    out += indentation + "    " + it->first + ":\n";
                    out += it->second.ToString(indentation + "        ");
                }
            }
            
            if(!PreBuild.empty())
            {
                out += indentation + "PreBuild:\n";
                for(auto it = PreBuild.begin(); it != PreBuild.end(); ++it)
                {
                    out += indentation + "    " + it->first + ":\n";
                    out += it->second.ToString(indentation + "        ");
                }
            }
            
            if(!PostBuild.empty())
            {
                out += indentation + "PostBuild:\n";
                for(auto it = PostBuild.begin(); it != PostBuild.end(); ++it)
                {
                    out += indentation + "    " + it->first + ":\n";
                    out += it->second.ToString(indentation + "        ");
                }
            }
            
            if(!Cleanup.empty())
            {
                out += indentation + "Cleanup:\n";
                for(auto it = Cleanup.begin(); it != Cleanup.end(); ++it)
                {
                    out += indentation + "    " + it->first + ":\n";
                    out += it->second.ToString(indentation + "        ");
                }
            }
            
            return out;
        }

        inline bool IsAllCompiledCacheInvalidated(const ScriptInfo& other) const
        {
            if( Language != other.Language || 
                CurrentBuildType != other.CurrentBuildType ||
                RequiredProfiles.size() != other.RequiredProfiles.size() ||
                Parameters.size() != other.Parameters.size() ||
                Variables.size() != other.Variables.size() ||
                OverrideCompileFlags.size() != other.OverrideCompileFlags.size() ||
                OtherFilesToBeCompiled.size() != other.OtherFilesToBeCompiled.size() ||
                IncludePaths.size() != other.IncludePaths.size() ||
                Dependencies.size() != other.Dependencies.size() ||
                Defines.size() != other.Defines.size() ||
                Populated != other.Populated)
            {
                return true;
            }

            for(const auto& it : RequiredProfiles)
            {
                if( other.RequiredProfiles.count(it.first) == 0 || 
                    other.RequiredProfiles.at(it.first) != it.second)
                {
                    return true;
                }
            }
            
            for(const auto& it : Parameters)
            {
                if( other.Parameters.count(it.first) == 0 || 
                    !other.Parameters.at(it.first).Equals(it.second))
                {
                    return true;
                }
            }
            
            for(const auto& it : Variables)
            {
                if( other.Variables.count(it.first) == 0 || 
                    other.Variables.at(it.first) != it.second)
                {
                    return true;
                }
            }

            for(const auto& it : OverrideCompileFlags)
            {
                if( other.OverrideCompileFlags.count(it.first) == 0 || 
                    !other.OverrideCompileFlags.at(it.first).Equals(it.second))
                {
                    return true;
                }
            }

            for(const auto& it : OtherFilesToBeCompiled)
            {
                if( other.OtherFilesToBeCompiled.count(it.first) == 0 || 
                    !other.OtherFilesToBeCompiled.at(it.first).Equals(it.second))
                {
                    return true;
                }
            }

            for(const auto& it : IncludePaths)
            {
                if( other.IncludePaths.count(it.first) == 0 || 
                    !other.IncludePaths.at(it.first).Equals(it.second))
                {
                    return true;
                }
            }

            for(size_t i = 0; i < Dependencies.size(); ++i)
            {
                if(!Dependencies[i].Equals(other.Dependencies[i]))
                    return true;
            }

            for(const auto& it : Defines)
            {
                if(other.Defines.count(it.first) == 0 || !other.Defines.at(it.first).Equals(it.second))
                    return true;
            }

            return false;
        }

        inline bool Equals(const ScriptInfo& other) const
        {
            if( Language != other.Language || 
                PassScriptPath != other.PassScriptPath ||
                CurrentBuildType != other.CurrentBuildType ||
                RequiredProfiles.size() != other.RequiredProfiles.size() ||
                Parameters.size() != other.Parameters.size() ||
                Variables.size() != other.Variables.size() ||
                OverrideCompileFlags.size() != other.OverrideCompileFlags.size() ||
                OverrideLinkFlags.size() != other.OverrideLinkFlags.size() ||
                OtherFilesToBeCompiled.size() != other.OtherFilesToBeCompiled.size() ||
                IncludePaths.size() != other.IncludePaths.size() ||
                Dependencies.size() != other.Dependencies.size() ||
                Defines.size() != other.Defines.size() ||
                Setup.size() != other.Setup.size() ||
                PreBuild.size() != other.PreBuild.size() ||
                PostBuild.size() != other.PostBuild.size() ||
                Cleanup.size() != other.Cleanup.size() ||
                Populated != other.Populated)
            {
                return false;
            }

            for(const auto& it : RequiredProfiles)
            {
                if( other.RequiredProfiles.count(it.first) == 0 || 
                    other.RequiredProfiles.at(it.first) != it.second)
                {
                    return false;
                }
            }
            
            for(const auto& it : Parameters)
            {
                if( other.Parameters.count(it.first) == 0 || 
                    !other.Parameters.at(it.first).Equals(it.second))
                {
                    return true;
                }
            }
            
            for(const auto& it : Variables)
            {
                if( other.Variables.count(it.first) == 0 || 
                    other.Variables.at(it.first) != it.second)
                {
                    return true;
                }
            }

            for(const auto& it : OverrideCompileFlags)
            {
                if( other.OverrideCompileFlags.count(it.first) == 0 || 
                    !other.OverrideCompileFlags.at(it.first).Equals(it.second))
                {
                    return false;
                }
            }

            for(const auto& it : OverrideLinkFlags)
            {
                if( other.OverrideLinkFlags.count(it.first) == 0 || 
                    !other.OverrideLinkFlags.at(it.first).Equals(it.second))
                {
                    return false;
                }
            }

            for(const auto& it : OtherFilesToBeCompiled)
            {
                if( other.OtherFilesToBeCompiled.count(it.first) == 0 || 
                    !other.OtherFilesToBeCompiled.at(it.first).Equals(it.second))
                {
                    return false;
                }
            }

            for(const auto& it : IncludePaths)
            {
                if( other.IncludePaths.count(it.first) == 0 || 
                    !other.IncludePaths.at(it.first).Equals(it.second))
                {
                    return false;
                }
            }

            for(size_t i = 0; i < Dependencies.size(); ++i)
            {
                if(!Dependencies[i].Equals(other.Dependencies[i]))
                    return false;
            }

            for(const auto& it : Defines)
            {
                if(other.Defines.count(it.first) == 0 || !other.Defines.at(it.first).Equals(it.second))
                    return false;
            }

            for(const auto& it : Setup)
            {
                if(other.Setup.count(it.first) == 0 || !other.Setup.at(it.first).Equals(it.second))
                    return false;
            }

            for(const auto& it : PreBuild)
            {
                if( other.PreBuild.count(it.first) == 0 || 
                    !other.PreBuild.at(it.first).Equals(it.second))
                {
                    return false;
                }
            }

            for(const auto& it : PostBuild)
            {
                if( other.PostBuild.count(it.first) == 0 || 
                    !other.PostBuild.at(it.first).Equals(it.second))
                {
                    return false;
                }
            }

            for(const auto& it : Cleanup)
            {
                if(other.Cleanup.count(it.first) == 0 || !other.Cleanup.at(it.first).Equals(it.second))
                    return false;
            }
            
            return true;
        }
    };
}
}

#endif
