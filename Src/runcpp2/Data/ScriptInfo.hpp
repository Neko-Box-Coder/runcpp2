#ifndef RUNCPP2_DATA_SCRIPT_INFO_HPP
#define RUNCPP2_DATA_SCRIPT_INFO_HPP

#include "runcpp2/Data/DependencyInfo.hpp"
#include "runcpp2/Data/Commands.hpp"
#include "runcpp2/Data/BuildType.hpp"
#include "runcpp2/Data/ParameterValue.hpp"
#include "runcpp2/Data/ProcessPaths.hpp"
#include "runcpp2/Data/Defines.hpp"
#include "runcpp2/Data/FlagsOverrideInfo.hpp"
#include "runcpp2/ParameterUtil.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "runcpp2/Data/ParseCommon.hpp"
#include "runcpp2/Data/PlatformProfileContainer.hpp"
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
        std::unordered_map<std::string, ParameterValue> Parameters; //TODO(NOW): Make this Platform profile map
        std::unordered_map<std::string, std::string> Variables;     //TODO(NOW): Make this Platform profile map
        
        PlatformProfileContainer<FlagsOverrideInfo> OverrideCompileFlags;
        PlatformProfileContainer<FlagsOverrideInfo> OverrideLinkFlags;
        PlatformProfileContainer<ProcessPaths> OtherFilesToBeCompiled;
        PlatformProfileContainer<ProcessPaths> IncludePaths;
        std::vector<DependencyInfo> Dependencies;
        
        PlatformProfileContainer<Defines> Defines;
        PlatformProfileContainer<Commands> Setup;
        PlatformProfileContainer<Commands> PreBuild;
        PlatformProfileContainer<Commands> PostBuild;
        PlatformProfileContainer<Commands> Cleanup;
        
        //Internal tracking
        bool Populated = false;
        ghc::filesystem::file_time_type LastWriteTime = ghc::filesystem::file_time_type::min();
        std::unordered_map<std::string, std::vector<std::string>> SubstitutionMap;
        
        inline DS::Result<void> 
        ParseYAML_Node( YAML::ConstNodePtr node, 
                        bool parseParameters,
                        const std::unordered_map<std::string, std::string>& inputParameters)
        {
            ssLOG_FUNC_DEBUG();
            
            if(parseParameters)
            {
                ParseParametersAndVariables(*this, node).DS_TRY();
            }
            
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
            
            if(parseParameters)
            {
                SubstitutionMap = {};
                ApplyParametersAndVariables(*this, 
                                            clonedNode, 
                                            resourceHandle, 
                                            SubstitutionMap, 
                                            inputParameters,
                                            exclusions).DS_TRY();
            }
            
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
            
            YAML::ConstNodePtr overrideCompileFlagsNode = 
                clonedNode->GetMapValueNode("OverrideCompileFlags");
            if(overrideCompileFlagsNode)
                DS_ASSERT_TRUE(OverrideCompileFlags.ParseYAML_Node(overrideCompileFlagsNode));
            
            YAML::ConstNodePtr overrideLinkFlagsNode = 
                clonedNode->GetMapValueNode("OverrideLinkFlags");
            if(overrideLinkFlagsNode)
                DS_ASSERT_TRUE(OverrideLinkFlags.ParseYAML_Node(overrideLinkFlagsNode));
            
            YAML::ConstNodePtr otherFilesToBeCompiledNode = 
                clonedNode->GetMapValueNode("OtherFilesToBeCompiled");
            if(!otherFilesToBeCompiledNode)
                otherFilesToBeCompiledNode = clonedNode->GetMapValueNode("SourceFiles");
            if(!otherFilesToBeCompiledNode)
                DS_ASSERT_TRUE(OtherFilesToBeCompiled.ParseYAML_Node(otherFilesToBeCompiledNode));
            
            YAML::ConstNodePtr includePathsNode = clonedNode->GetMapValueNode("IncludePaths");
            if(includePathsNode)
                DS_ASSERT_TRUE(IncludePaths.ParseYAML_Node(includePathsNode));
            
            if(ExistAndHasChild(clonedNode, "Dependencies"))
            {
                YAML::ConstNodePtr dependenciesNode = clonedNode->GetMapValueNode("Dependencies");
                for(int i = 0; i < dependenciesNode->GetChildrenCount(); ++i)
                {
                    DependencyInfo info;
                    YAML::ConstNodePtr dependencyNode = dependenciesNode->GetSequenceChildNode(i);
                    info.ParseYAML_Node(dependencyNode, SubstitutionMap, inputParameters).DS_TRY_ACT
                    (
                        DS_TMP_ERROR.Message += "\nScriptInfo: Failed to parse DependencyInfo at "
                                                "index " + DS_STR(i);
                        DS_APPEND_TRACE(DS_TMP_ERROR);
                        return DS::Error(DS_TMP_ERROR)
                    );
                    Dependencies.push_back(info);
                }
            }
            
            YAML::ConstNodePtr definesNode = clonedNode->GetMapValueNode("Defines");
            if(definesNode)
                DS_ASSERT_TRUE(Defines.ParseYAML_Node(definesNode));
            
            YAML::ConstNodePtr setupNode = clonedNode->GetMapValueNode("Setup");
            if(setupNode)
                DS_ASSERT_TRUE(Setup.ParseYAML_Node(setupNode));
            
            YAML::ConstNodePtr preBuildNode = clonedNode->GetMapValueNode("PreBuild");
            if(preBuildNode)
                DS_ASSERT_TRUE(PreBuild.ParseYAML_Node(preBuildNode));
            
            YAML::ConstNodePtr postBuildNode = clonedNode->GetMapValueNode("PostBuild");
            if(postBuildNode)
                DS_ASSERT_TRUE(PostBuild.ParseYAML_Node(postBuildNode));
            
            YAML::ConstNodePtr cleanupNode = clonedNode->GetMapValueNode("Cleanup");
            if(cleanupNode)
                DS_ASSERT_TRUE(Cleanup.ParseYAML_Node(cleanupNode));
            
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
            
            if(!OverrideCompileFlags.Data.empty())
            {
                out += indentation + "OverrideCompileFlags:\n";
                out += OverrideCompileFlags.ToString(indentation + "    ");
            }
            
            if(!OverrideLinkFlags.Data.empty())
            {
                out += indentation + "OverrideLinkFlags:\n";
                out += OverrideLinkFlags.ToString(indentation + "    ");
            }
            
            if(!OtherFilesToBeCompiled.Data.empty())
            {
                out += indentation + "SourceFiles:\n";
                out += OtherFilesToBeCompiled.ToString(indentation + "    ");
            }

            if(!IncludePaths.Data.empty())
            {
                out += indentation + "IncludePaths:\n";
                out += IncludePaths.ToString(indentation + "    ");
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
            
            if(!Defines.Data.empty())
            {
                out += indentation + "Defines:\n";
                out += Defines.ToString(indentation + "    ");
            }
            
            if(!Setup.Data.empty())
            {
                out += indentation + "Setup:\n";
                out += Setup.ToString(indentation + "    ");
            }
            
            if(!PreBuild.Data.empty())
            {
                out += indentation + "PreBuild:\n";
                out += PreBuild.ToString(indentation + "    ");
            }
            
            if(!PostBuild.Data.empty())
            {
                out += indentation + "PostBuild:\n";
                out += PostBuild.ToString(indentation + "    ");
            }
            
            if(!Cleanup.Data.empty())
            {
                out += indentation + "Cleanup:\n";
                out += Cleanup.ToString(indentation + "    ");
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
                Dependencies.size() != other.Dependencies.size() ||
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

            if(!OverrideCompileFlags.Equals(other.OverrideCompileFlags))
                return false;
            
            if(!OtherFilesToBeCompiled.Equals(other.OtherFilesToBeCompiled))
                return false;

            if(!IncludePaths.Equals(other.IncludePaths))
                return false;

            for(size_t i = 0; i < Dependencies.size(); ++i)
            {
                if(!Dependencies[i].Equals(other.Dependencies[i]))
                    return true;
            }

            if(!Defines.Equals(other.Defines))
                return false;

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
                Dependencies.size() != other.Dependencies.size() ||
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

            if(!OverrideCompileFlags.Equals(other.OverrideCompileFlags))
                return false;

            if(!OverrideLinkFlags.Equals(other.OverrideLinkFlags))
                return false;
            
            if(!OtherFilesToBeCompiled.Equals(other.OtherFilesToBeCompiled))
                return false;
            
            if(!IncludePaths.Equals(other.IncludePaths))
                return false;

            for(size_t i = 0; i < Dependencies.size(); ++i)
            {
                if(!Dependencies[i].Equals(other.Dependencies[i]))
                    return false;
            }

            if(!Defines.Equals(other.Defines))
                return false;

            if(!Setup.Equals(other.Setup))
                return false;
            
            if(!PreBuild.Equals(other.PreBuild))
                return false;
            
            if(!PostBuild.Equals(other.PostBuild))
                return false;
            
            if(!Cleanup.Equals(other.Cleanup))
                return false;
            
            return true;
        }
    };
}
}

#endif
