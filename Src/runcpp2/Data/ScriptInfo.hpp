#ifndef RUNCPP2_DATA_SCRIPT_INFO_HPP
#define RUNCPP2_DATA_SCRIPT_INFO_HPP

#include "runcpp2/Data/DependencyInfo.hpp"
#include "runcpp2/Data/ProfilesFlagsOverride.hpp"
#include "runcpp2/Data/ProfilesProcessPaths.hpp"
#include "runcpp2/Data/ProfilesDefines.hpp"
#include "runcpp2/Data/ProfilesCommands.hpp"
#include "runcpp2/Data/BuildType.hpp"
#include "runcpp2/Data/ParameterValue.hpp"
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
        
        inline DS::Result<void> ParseParametersAndVariables(YAML::ConstNodePtr node)
        {
            std::vector<NodeRequirement> requirements =
            {
                NodeRequirement("Parameters", YAML::NodeType::Map, false, true),
                NodeRequirement("Variables", YAML::NodeType::Map, false, true),
            };
            
            DS_ASSERT_TRUE(CheckNodeRequirements(node, requirements));
            
            //NOTE: Evaluate Parameters first, since we need to perform substitutions
            if(ExistAndHasChild(node, "Parameters"))
            {
               YAML::ConstNodePtr parametersNode = node->GetMapValueNode("Parameters");
               for(int i = 0; i < parametersNode->GetChildrenCount(); ++i)
               {
                    std::string parameterName = parametersNode  ->GetMapKeyScalarAt<std::string>(i)
                                                                .DS_TRY();
                    YAML::ConstNodePtr valueNode = parametersNode->GetMapValueNodeAt(i);
                    ParameterValue paramValue;
                    paramValue.ParseYAML_Node(valueNode).DS_TRY();
                    if(Parameters.count(parameterName) != 0)
                    {
                        return DS_ERROR_MSG("ScriptInfo: Same parameter (" + parameterName + 
                                            ") is added more than once");
                    }
                    Parameters[parameterName] = paramValue;
               }
            }
            
            //NOTE: Evaluate Parameters first, since we need to perform substitutions
            if(ExistAndHasChild(node, "Variables"))
            {
                YAML::ConstNodePtr variablesNode = node->GetMapValueNode("Variables");
                for(int i = 0; i < variablesNode->GetChildrenCount(); ++i)
                {
                    std::string variableName = variablesNode->GetMapKeyScalarAt<std::string>(i)
                                                            .DS_TRY();
                    std::string variableValue = variablesNode   ->GetMapValueScalarAt<std::string>(i)
                                                                .DS_TRY();
                    if(Variables.count(variableName) != 0)
                    {
                        return DS_ERROR_MSG("ScriptInfo: Same variable (" + variableName + 
                                            ") is added more than once");
                    }
                    Variables[variableName] = variableValue;
                }
            }
            
            return {};
        }
        
        //TODO: Use DS::Result instead
        inline bool ParseYAML_Node( YAML::ConstNodePtr node, 
                                    const std::unordered_map<   std::string, 
                                                                std::string>& inputParameters)
        {
            ssLOG_FUNC_DEBUG();
            
            #define TRY_RET() DS_TRY_ACT(ssLOG_ERROR(DS_TMP_ERROR.ToString()); return false)
            
            ParseParametersAndVariables(node).TRY_RET();
            
            //Remove parameters and variables in the cloned node.
            YAML::ResourceHandle resourceHandle;
            YAML::NodePtr clonedNode = node->Clone(false, resourceHandle).TRY_RET();
            DEFER { YAML::FreeYAMLResource(resourceHandle); };
            
            if(clonedNode->HasMapKey("Parameters"))
            {
                clonedNode->RemoveMapChild("Parameters").TRY_RET();
            }
            if(clonedNode->HasMapKey("Variables"))
            {
                clonedNode->RemoveMapChild("Variables").TRY_RET();
            }
            
            std::unordered_map<std::string, std::vector<std::string>> substitutionMap;
            
            //Populate parameters values first
            for(auto it = Parameters.begin(); it != Parameters.end(); ++it)
            {
                bool isDefault = true;
                std::string valueToParse;
                if(inputParameters.count(it->first) == 0)
                    valueToParse = it->second.Default;
                else
                {
                    valueToParse = inputParameters.at(it->first);
                    isDefault = false;
                }
                
                //Parse the value
                std::string subKey = "{" + it->first + "}";
                {
                    if(it->second.Array)
                    {
                        std::vector<std::string> inputValues;
                        SplitString(valueToParse, ",", inputValues);
                        for(int i = 0; i < inputValues.size(); ++i)
                        {
                            bool inConstraint = it  ->second
                                                    .IsInputInConstraint(inputValues[i])
                                                    .TRY_RET();
                            if(!inConstraint)
                            {
                                ssLOG_ERROR("Input parameter[" << i << "]: " << inputValues[i] << 
                                            (isDefault ? "(Default)" : "") << " is not in constraint");
                                return false;
                            }
                        }
                        substitutionMap[subKey] = inputValues;
                    }
                    else
                    {
                        bool inConstraint = it  ->second
                                                .IsInputInConstraint(valueToParse)
                                                .TRY_RET();
                        if(!inConstraint)
                        {
                            ssLOG_ERROR("Input parameter: " << it->first << 
                                        (isDefault ? "(Default)" : "") << " is not in constraint");
                            return false;
                        }
                        substitutionMap[subKey] = {valueToParse};
                    }
                }
            } //for(auto it = Parameters.begin(); it != Parameters.end(); ++it)
            
            std::unordered_map<std::string, std::vector<std::string>> variablesMap;
            
            //Populate variables next
            for(auto it = Variables.begin(); it != Variables.end(); ++it)
            {
                //Check if the same key already exists in the parameter
                if(Parameters.count(it->first) != 0)
                {
                    ssLOG_ERROR("Variable name " << it->first << " already exists in Parameters");
                    return false;
                }
                
                std::string subKey = "{" + it->first + "}";
                variablesMap[it->first] = {};
                PerformMultiSubstitutions(  substitutionMap, 
                                            {}, 
                                            it->second, 
                                            variablesMap[it->first]).TRY_RET();
            }
            substitutionMap.insert(variablesMap.begin(), variablesMap.end());
            
            //TODO: Probably need to move this to a helper function for dependencies import
            //Perform substitution recursively over the whole YAML object
            std::deque<YAML::NodePtr> nodesToVisit;
            nodesToVisit.push_back(clonedNode);
            while(!nodesToVisit.empty())
            {
                YAML::NodePtr currentNode = nodesToVisit.front();
                nodesToVisit.pop_front();
                static_assert((int)YAML::NodeType::Count == 4, "");
                switch(currentNode->GetType())
                {
                    case YAML::NodeType::Scalar:
                    {
                        YAML::Node* parent = currentNode->GetParent();
                        std::string scalarValue = currentNode->GetScalar<std::string>().TRY_RET();
                        if(parent && parent->GetType() == YAML::NodeType::Sequence)
                        {
                            std::vector<std::string> newValues;
                            PerformMultiSubstitutions(  substitutionMap, 
                                                        {}, 
                                                        scalarValue, 
                                                        newValues).TRY_RET();
                            if(newValues.empty())
                            {
                                ssLOG_ERROR("Substitution array returned empty");
                                return false;
                            }
                            
                            //Update the current value
                            currentNode->InitScalar(newValues[0], resourceHandle).TRY_RET();
                            
                            //Find the index of the current node
                            int currentIndex = -1;
                            for(int i = 0; i < parent->GetChildrenCount(); ++i)
                            {
                                if(parent->GetSequenceChildNode(i) == currentNode)
                                {
                                    currentIndex = i;
                                    break;
                                }
                            }
                            
                            if(currentIndex == -1)
                            {
                                ssLOG_ERROR("Cannot find current node from parent?");
                                return false;
                            }
                            
                            //Then insert the rest of the substituted values after the current one
                            if(newValues.size() > 1)
                            {
                                for(int i = 1; i < newValues.size(); ++i)
                                {
                                    YAML::NodePtr newChild = 
                                        parent->CreateSequenceChildAt(currentIndex + i).TRY_RET();
                                    newChild->InitScalar(newValues[i], resourceHandle).TRY_RET();
                                }
                            }
                        }
                        else
                        {
                            PerformSubstitutions(substitutionMap, {}, scalarValue).TRY_RET();
                            currentNode->InitScalar(scalarValue, resourceHandle).TRY_RET();
                        }
                        break;
                    } //case YAML::NodeType::Scalar:
                    case YAML::NodeType::Alias:
                        ssLOG_ERROR("Anchors should be resolved. This should not be reached");
                        return false;
                    case YAML::NodeType::Sequence:
                        for(int i = currentNode->GetChildrenCount() - 1; i >= 0; --i)
                            nodesToVisit.push_back(currentNode->GetSequenceChildNode(i));
                        break;
                    case YAML::NodeType::Map:
                        for(int i = currentNode->GetChildrenCount() - 1; i >= 0; --i)
                            nodesToVisit.push_back(currentNode->GetMapValueNodeAt(i));
                        break;
                } //switch(currentNode->GetType())
            } //while(!nodesToVisit.empty())
            
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
            {
                ssLOG_ERROR("ScriptInfo: Failed to meet requirements");
                return false;
            }
            
            if(ExistAndHasChild(clonedNode, "PassScriptPath"))
            {
                std::string passScriptPathStr = 
                    clonedNode->GetMapValueScalar<std::string>("PassScriptPath").TRY_RET();
                for(size_t i = 0; i < passScriptPathStr.length(); ++i)
                    passScriptPathStr[i] = std::tolower(passScriptPathStr[i]);
                
                if(passScriptPathStr == "true" || passScriptPathStr == "1")
                    PassScriptPath = true;
                else if(passScriptPathStr == "false" || passScriptPathStr == "0")
                    PassScriptPath = false;
                else
                {
                    ssLOG_ERROR("ScriptInfo: Invalid value for PassScriptPath: " << passScriptPathStr);
                    ssLOG_ERROR("Expected true/false or 1/0");
                    return false;
                }
            }
            
            if(ExistAndHasChild(clonedNode, "Language"))
            {
                Language = clonedNode->GetMapValueScalar<std::string>("Language").TRY_RET();
            }
            
            if(ExistAndHasChild(clonedNode, "BuildType"))
            {
                std::string typeStr = clonedNode->GetMapValueScalar<std::string>("BuildType").TRY_RET();
                BuildType buildType = StringToBuildType(typeStr);
                if(buildType == BuildType::COUNT)
                {
                    ssLOG_ERROR("ScriptInfo: Invalid build type: " << typeStr);
                    return false;
                }
                CurrentBuildType = buildType;
            }
            
            if(ExistAndHasChild(clonedNode, "RequiredProfiles"))
            {
                YAML::ConstNodePtr requiredProfilesNode = 
                    clonedNode->GetMapValueNode("RequiredProfiles");
                for(int i = 0; i < requiredProfilesNode->GetChildrenCount(); ++i)
                {
                    PlatformName platform = requiredProfilesNode->GetMapKeyScalarAt<std::string>(i)
                                                                .TRY_RET();
                    std::vector<ProfileName> profiles;
                    YAML::ConstNodePtr platformNode = requiredProfilesNode->GetMapValueNodeAt(i);
                    for(int j = 0; j < platformNode->GetChildrenCount(); ++j)
                    {
                        std::string profile = platformNode  ->GetSequenceChildScalar<std::string>(j)
                                                            .TRY_RET();
                        profiles.push_back(profile);
                    }
                    
                    RequiredProfiles[platform] = profiles;
                }
            }
            
            if(!ParsePlatformProfileMap<ProfilesFlagsOverride>( clonedNode, 
                                                                "OverrideCompileFlags", 
                                                                OverrideCompileFlags, 
                                                                "OverrideCompileFlags"))
            {
                return false;
            }
            
            if(!ParsePlatformProfileMap<ProfilesFlagsOverride>( clonedNode, 
                                                                "OverrideLinkFlags", 
                                                                OverrideLinkFlags, 
                                                                "OverrideLinkFlags"))
            {
                return false;
            }
            
            if(!ParsePlatformProfileMap<ProfilesProcessPaths>(  clonedNode, 
                                                                "OtherFilesToBeCompiled", 
                                                                OtherFilesToBeCompiled, 
                                                                "OtherFilesToBeCompiled"))
            {
                return false;
            }
            
            if(!ParsePlatformProfileMap<ProfilesProcessPaths>(  clonedNode, 
                                                                "SourceFiles", 
                                                                OtherFilesToBeCompiled, 
                                                                "SourceFiles"))
            {
                return false;
            }
            
            if(!ParsePlatformProfileMap<ProfilesProcessPaths>(  clonedNode, 
                                                                "IncludePaths", 
                                                                IncludePaths, 
                                                                "IncludePaths"))
            {
                return false;
            }
            
            if(ExistAndHasChild(clonedNode, "Dependencies"))
            {
                YAML::ConstNodePtr dependenciesNode = clonedNode->GetMapValueNode("Dependencies");
                for(int i = 0; i < dependenciesNode->GetChildrenCount(); ++i)
                {
                    DependencyInfo info;
                    YAML::ConstNodePtr dependencyNode = dependenciesNode->GetSequenceChildNode(i);
                    if(!info.ParseYAML_Node(dependencyNode))
                    {
                        ssLOG_ERROR("ScriptInfo: Failed to parse DependencyInfo at index " << i);
                        return false;
                    }
                    
                    Dependencies.push_back(info);
                }
            }
            
            if(!ParsePlatformProfileMap<ProfilesDefines>(clonedNode, "Defines", Defines, "Defines"))
                return false;
            
            if(!ParsePlatformProfileMap<ProfilesCommands>(clonedNode, "Setup", Setup, "Setup"))
                return false;
            
            if(!ParsePlatformProfileMap<ProfilesCommands>(clonedNode, "PreBuild", PreBuild, "PreBuild"))
                return false;
            
            if(!ParsePlatformProfileMap<ProfilesCommands>(  clonedNode, 
                                                            "PostBuild", 
                                                            PostBuild, 
                                                            "PostBuild"))
            {
                return false;
            }
            
            if(!ParsePlatformProfileMap<ProfilesCommands>(clonedNode, "Cleanup", Cleanup, "Cleanup"))
                return false;
            
            return true;
            
            #undef TRY_RET
        }

        //TODO: Use DS::Result
        //TODO: Text escaping?
        inline std::string ToString(std::string indentation) const
        {
            #define TRY_RET() DS_TRY_ACT(ssLOG_ERROR(DS_TMP_ERROR.ToString()); return "")
            
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
                    std::string paramerterStr = it->second.ToString(indentation + "        ").TRY_RET();
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
            #undef TRY_RET
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
