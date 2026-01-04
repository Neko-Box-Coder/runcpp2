#include "runcpp2/Data/ScriptInfo.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "runcpp2/Data/ParseCommon.hpp"
#include "ssLogger/ssLog.hpp"

bool runcpp2::Data::ScriptInfo::ParseYAML_Node(YAML::ConstNodePtr node)
{
    ssLOG_FUNC_DEBUG();
    
    static_assert(FieldsCount == 14, "Update this function when adding new fields");
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
    
    if(!CheckNodeRequirements(node, requirements))
    {
        ssLOG_ERROR("ScriptInfo: Failed to meet requirements");
        return false;
    }
    
    if(ExistAndHasChild(node, "PassScriptPath"))
    {
        std::string passScriptPathStr = node->GetMapValueScalar<std::string>("PassScriptPath")
                                            .DS_TRY_ACT(return false);
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
    
    if(ExistAndHasChild(node, "Language"))
    {
        Language = node->GetMapValueScalar<std::string>("Language").DS_TRY_ACT(return false);
    }
    
    if(ExistAndHasChild(node, "BuildType"))
    {
        std::string typeStr = node  ->GetMapValueScalar<std::string>("BuildType")
                                    .DS_TRY_ACT(return false);
        BuildType buildType = StringToBuildType(typeStr);
        if(buildType == BuildType::COUNT)
        {
            ssLOG_ERROR("ScriptInfo: Invalid build type: " << typeStr);
            return false;
        }
        CurrentBuildType = buildType;
    }
    
    if(ExistAndHasChild(node, "RequiredProfiles"))
    {
        YAML::ConstNodePtr requiredProfilesNode = node->GetMapValueNode("RequiredProfiles");
        for(int i = 0; i < requiredProfilesNode->GetChildrenCount(); ++i)
        {
            PlatformName platform = requiredProfilesNode->GetMapKeyScalarAt<std::string>(i)
                                                        .DS_TRY_ACT(return false);
            std::vector<ProfileName> profiles;
            YAML::ConstNodePtr platformNode = requiredProfilesNode->GetMapValueNodeAt(i);
            for(int j = 0; j < platformNode->GetChildrenCount(); ++j)
            {
                std::string profile = platformNode  ->GetSequenceChildScalar<std::string>(j)
                                                    .DS_TRY_ACT(return false);
                profiles.push_back(profile);
            }
            
            RequiredProfiles[platform] = profiles;
        }
    }
     
    if(!ParsePlatformProfileMap<ProfilesFlagsOverride>( node, 
                                                        "OverrideCompileFlags", 
                                                        OverrideCompileFlags, 
                                                        "OverrideCompileFlags"))
    {
        return false;
    }
    
    if(!ParsePlatformProfileMap<ProfilesFlagsOverride>( node, 
                                                        "OverrideLinkFlags", 
                                                        OverrideLinkFlags, 
                                                        "OverrideLinkFlags"))
    {
        return false;
    }
    
    if(!ParsePlatformProfileMap<ProfilesProcessPaths>(  node, 
                                                        "OtherFilesToBeCompiled", 
                                                        OtherFilesToBeCompiled, 
                                                        "OtherFilesToBeCompiled"))
    {
        return false;
    }
    
    if(!ParsePlatformProfileMap<ProfilesProcessPaths>(  node, 
                                                        "SourceFiles", 
                                                        OtherFilesToBeCompiled, 
                                                        "SourceFiles"))
    {
        return false;
    }
    
    if(!ParsePlatformProfileMap<ProfilesProcessPaths>(  node, 
                                                        "IncludePaths", 
                                                        IncludePaths, 
                                                        "IncludePaths"))
    {
        return false;
    }
    
    if(ExistAndHasChild(node, "Dependencies"))
    {
        YAML::ConstNodePtr dependenciesNode = node->GetMapValueNode("Dependencies");
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
    
    if(!ParsePlatformProfileMap<ProfilesDefines>(node, "Defines", Defines, "Defines"))
        return false;
    
    if(!ParsePlatformProfileMap<ProfilesCommands>(node, "Setup", Setup, "Setup"))
        return false;
    
    if(!ParsePlatformProfileMap<ProfilesCommands>(node, "PreBuild", PreBuild, "PreBuild"))
        return false;
    
    if(!ParsePlatformProfileMap<ProfilesCommands>(node, "PostBuild", PostBuild, "PostBuild"))
        return false;
    
    if(!ParsePlatformProfileMap<ProfilesCommands>(node, "Cleanup", Cleanup, "Cleanup"))
        return false;
    
    return true;
}

std::string runcpp2::Data::ScriptInfo::ToString(std::string indentation) const
{
    static_assert(FieldsCount == 14, "Update this function when adding new fields");
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
                    out += indentation + "    -   " + GetEscapedYAMLString(it->second[i]) + "\n";
            }
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
        for(auto it = OtherFilesToBeCompiled.begin(); it != OtherFilesToBeCompiled.end(); ++it)
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

bool runcpp2::Data::ScriptInfo::IsAllCompiledCacheInvalidated(const ScriptInfo& other) const
{
    static_assert(FieldsCount == 14, "Update this function when adding new fields");
    if( Language != other.Language || 
        CurrentBuildType != other.CurrentBuildType ||
        RequiredProfiles.size() != other.RequiredProfiles.size() ||
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

bool runcpp2::Data::ScriptInfo::Equals(const ScriptInfo& other) const
{
    static_assert(FieldsCount == 14, "Update this function when adding new fields");
    if( Language != other.Language || 
        PassScriptPath != other.PassScriptPath ||
        CurrentBuildType != other.CurrentBuildType ||
        RequiredProfiles.size() != other.RequiredProfiles.size() ||
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
        if(other.PreBuild.count(it.first) == 0 || !other.PreBuild.at(it.first).Equals(it.second))
            return false;
    }

    for(const auto& it : PostBuild)
    {
        if(other.PostBuild.count(it.first) == 0 || !other.PostBuild.at(it.first).Equals(it.second))
            return false;
    }

    for(const auto& it : Cleanup)
    {
        if(other.Cleanup.count(it.first) == 0 || !other.Cleanup.at(it.first).Equals(it.second))
            return false;
    }
    
    return true;
}
