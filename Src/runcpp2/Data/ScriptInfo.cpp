#include "runcpp2/Data/ScriptInfo.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "runcpp2/Data/ParseCommon.hpp"
#include "ssLogger/ssLog.hpp"

bool runcpp2::Data::ScriptInfo::ParseYAML_Node(ryml::ConstNodeRef node)
{
    ssLOG_FUNC_DEBUG();
    
    INTERNAL_RUNCPP2_SAFE_START();
    
    std::vector<NodeRequirement> requirements =
    {
        NodeRequirement("PassScriptPath", ryml::NodeType_e::KEYVAL, false, true),
        NodeRequirement("Language", ryml::NodeType_e::KEYVAL, false, true),
        NodeRequirement("BuildType", ryml::NodeType_e::KEYVAL, false, true),
        NodeRequirement("RequiredProfiles", ryml::NodeType_e::MAP, false, true),
        NodeRequirement("OverrideCompileFlags", ryml::NodeType_e::MAP, false, true),
        NodeRequirement("OverrideLinkFlags", ryml::NodeType_e::MAP, false, true),
        NodeRequirement("OtherFilesToBeCompiled", ryml::NodeType_e::MAP, false, true),
        NodeRequirement("IncludePaths", ryml::NodeType_e::MAP, false, true),
        NodeRequirement("Dependencies", ryml::NodeType_e::SEQ, false, true),
        NodeRequirement("Defines", ryml::NodeType_e::MAP, false, true),
        NodeRequirement("Setup", ryml::NodeType_e::MAP, false, true),
        NodeRequirement("PreBuild", ryml::NodeType_e::MAP, false, true),
        NodeRequirement("PostBuild", ryml::NodeType_e::MAP, false, true),
        NodeRequirement("Cleanup", ryml::NodeType_e::MAP, false, true)
    };
    
    if(!CheckNodeRequirements(node, requirements))
    {
        ssLOG_ERROR("ScriptInfo: Failed to meet requirements");
        return false;
    }
    
    if(ExistAndHasChild(node, "PassScriptPath"))
    {
        std::string passScriptPathStr = GetValue(node["PassScriptPath"]);
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
        node["Language"] >> Language;
    
    if(ExistAndHasChild(node, "BuildType"))
    {
        std::string typeStr = GetValue(node["BuildType"]);
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
        for(int i = 0; i < node["RequiredProfiles"].num_children(); ++i)
        {
            if(!INTERNAL_RUNCPP2_BIT_CONTANTS(  node["RequiredProfiles"][i].type().type,
                                                ryml::NodeType_e::SEQ))
            {
                ssLOG_ERROR("ScriptInfo: RequiredProfiles requires a sequence");
                return false;
            }
            
            PlatformName platform = GetKey(node["RequiredProfiles"][i]);
            std::vector<ProfileName> profiles;
            for(int j = 0; j < node["RequiredProfiles"][i].num_children(); ++j)
                profiles.push_back(GetValue(node["RequiredProfiles"][i][j]));
            
            RequiredProfiles[platform] = profiles;
        }
    }
    
    if(ExistAndHasChild(node, "OverrideCompileFlags"))
    {
        ryml::ConstNodeRef overrideCompileFlagsNode = node["OverrideCompileFlags"];
        
        for(int i = 0; i < overrideCompileFlagsNode.num_children(); ++i)
        {
            PlatformName platform = GetKey(overrideCompileFlagsNode[i]);
            ProfilesFlagsOverride compileFlags;
            ryml::ConstNodeRef currentCompileFlagsNode = overrideCompileFlagsNode[i];
            
            if(!compileFlags.ParseYAML_Node(currentCompileFlagsNode))
            {
                ssLOG_ERROR("ScriptInfo: Failed to parse OverrideCompileFlags.");
                return false;
            }
            OverrideCompileFlags[platform] = compileFlags;
        }
    }
    
    if(ExistAndHasChild(node, "OverrideLinkFlags"))
    {
        ryml::ConstNodeRef overrideLinkFlagsNode = node["OverrideLinkFlags"];
        
        for(int i = 0; i < overrideLinkFlagsNode.num_children(); ++i)
        {
            PlatformName platform = GetKey(overrideLinkFlagsNode[i]);
            ProfilesFlagsOverride linkFlags;
            ryml::ConstNodeRef currentLinkFlagsNode = overrideLinkFlagsNode[i];
            
            if(!linkFlags.ParseYAML_Node(currentLinkFlagsNode))
            {
                ssLOG_ERROR("ScriptInfo: Failed to parse OverrideLinkFlags.");
                return false;
            }
            OverrideLinkFlags[platform] = linkFlags;
        }
    }
    
    if(ExistAndHasChild(node, "OtherFilesToBeCompiled"))
    {
        for(int i = 0; i < node["OtherFilesToBeCompiled"].num_children(); ++i)
        {
            ProfilesProcessPaths compilesFiles;
            ryml::ConstNodeRef currentProfileMapNode = node["OtherFilesToBeCompiled"][i];
            PlatformName platform = GetKey(currentProfileMapNode);
            
            if(!compilesFiles.ParseYAML_Node(currentProfileMapNode))
            {
                ssLOG_ERROR("ScriptInfo: Failed to parse OtherFilesToBeCompiled.");
                return false;
            }
            OtherFilesToBeCompiled[platform] = compilesFiles;
        }
    }

    if(ExistAndHasChild(node, "IncludePaths"))
    {
        for(int i = 0; i < node["IncludePaths"].num_children(); ++i)
        {
            ProfilesProcessPaths includePaths;
            ryml::ConstNodeRef currentProfileMapNode = node["IncludePaths"][i];
            PlatformName platform = GetKey(currentProfileMapNode);
            
            if(!includePaths.ParseYAML_Node(currentProfileMapNode))
            {
                ssLOG_ERROR("ScriptInfo: Failed to parse IncludePaths.");
                return false;
            }
            IncludePaths[platform] = includePaths;
        }
    }
    
    if(ExistAndHasChild(node, "Dependencies"))
    {
        for(int i = 0; i < node["Dependencies"].num_children(); ++i)
        {
            DependencyInfo info;
            ryml::ConstNodeRef dependencyNode = node["Dependencies"][i];
            
            if(!info.ParseYAML_Node(dependencyNode))
            {
                ssLOG_ERROR("ScriptInfo: Failed to parse DependencyInfo at index " << i);
                return false;
            }
            
            Dependencies.push_back(info);
        }
    }
    
    if(ExistAndHasChild(node, "Defines"))
    {
        ryml::ConstNodeRef definesNode = node["Defines"];
        
        for(int i = 0; i < definesNode.num_children(); ++i)
        {
            PlatformName platform = GetKey(definesNode[i]);
            ProfilesDefines profilesDefines;
            ryml::ConstNodeRef currentDefinesNode = definesNode[i];
            
            if(!profilesDefines.ParseYAML_Node(currentDefinesNode))
            {
                ssLOG_ERROR("ScriptInfo: Failed to parse Defines.");
                return false;
            }
            Defines[platform] = profilesDefines;
        }
    }
    
    if(ExistAndHasChild(node, "Setup"))
    {
        for(int i = 0; i < node["Setup"].num_children(); ++i)
        {
            PlatformName platform = GetKey(node["Setup"][i]);
            ProfilesCommands commands;
            ryml::ConstNodeRef currentCommandsNode = node["Setup"][i];
            
            if(!commands.ParseYAML_Node(currentCommandsNode))
            {
                ssLOG_ERROR("ScriptInfo: Failed to parse Setup.");
                return false;
            }
            Setup[platform] = commands;
        }
    }
    
    if(ExistAndHasChild(node, "PreBuild"))
    {
        for(int i = 0; i < node["PreBuild"].num_children(); ++i)
        {
            PlatformName platform = GetKey(node["PreBuild"][i]);
            ProfilesCommands commands;
            ryml::ConstNodeRef currentCommandsNode = node["PreBuild"][i];
            
            if(!commands.ParseYAML_Node(currentCommandsNode))
            {
                ssLOG_ERROR("ScriptInfo: Failed to parse PreBuild.");
                return false;
            }
            PreBuild[platform] = commands;
        }
    }
    
    if(ExistAndHasChild(node, "PostBuild"))
    {
        for(int i = 0; i < node["PostBuild"].num_children(); ++i)
        {
            PlatformName platform = GetKey(node["PostBuild"][i]);
            ProfilesCommands commands;
            ryml::ConstNodeRef currentCommandsNode = node["PostBuild"][i];
            
            if(!commands.ParseYAML_Node(currentCommandsNode))
            {
                ssLOG_ERROR("ScriptInfo: Failed to parse PostBuild.");
                return false;
            }
            PostBuild[platform] = commands;
        }
    }
    
    if(ExistAndHasChild(node, "Cleanup"))
    {
        for(int i = 0; i < node["Cleanup"].num_children(); ++i)
        {
            PlatformName platform = GetKey(node["Cleanup"][i]);
            ProfilesCommands commands;
            ryml::ConstNodeRef currentCommandsNode = node["Cleanup"][i];
            
            if(!commands.ParseYAML_Node(currentCommandsNode))
            {
                ssLOG_ERROR("ScriptInfo: Failed to parse Cleanup.");
                return false;
            }
            Cleanup[platform] = commands;
        }
    }
    
    return true;
    
    INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
}

std::string runcpp2::Data::ScriptInfo::ToString(std::string indentation) const
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
        out += indentation + "OtherFilesToBeCompiled:\n";
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

bool runcpp2::Data::ScriptInfo::Equals(const ScriptInfo& other) const
{
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
