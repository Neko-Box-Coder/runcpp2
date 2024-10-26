#include "runcpp2/Data/ScriptInfo.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "runcpp2/Data/ParseCommon.hpp"
#include "ssLogger/ssLog.hpp"

bool runcpp2::Data::ScriptInfo::ParseYAML_Node(ryml::ConstNodeRef& node)
{
    ssLOG_FUNC_DEBUG();
    
    INTERNAL_RUNCPP2_SAFE_START();
    
    std::vector<NodeRequirement> requirements =
    {
        NodeRequirement("PassScriptPath", ryml::NodeType_e::KEYVAL, false, true),
        NodeRequirement("Language", ryml::NodeType_e::KEYVAL, false, true),
        NodeRequirement("RequiredProfiles", ryml::NodeType_e::MAP, false, true),
        NodeRequirement("OverrideCompileFlags", ryml::NodeType_e::MAP, false, true),
        NodeRequirement("OverrideLinkFlags", ryml::NodeType_e::MAP, false, true),
        NodeRequirement("OtherFilesToBeCompiled", ryml::NodeType_e::MAP, false, true),
        NodeRequirement("Dependencies", ryml::NodeType_e::SEQ, false, true),
        NodeRequirement("Defines", ryml::NodeType_e::MAP, false, true)
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
            ProfilesCompilesFiles compilesFiles;
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
    
    return true;
    
    INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
}

std::string runcpp2::Data::ScriptInfo::ToString(std::string indentation) const
{
    std::string out = indentation + "ScriptInfo:\n";
    
    out += indentation + "    PassScriptPath: " + (PassScriptPath ? "true" : "false") + "\n";
    
    if(!Language.empty())
        out += indentation + "    Language: " + Language + "\n";
    
    out += indentation + "    RequiredProfiles:\n";
    
    for(auto it = RequiredProfiles.begin(); it != RequiredProfiles.end(); ++it)
    {
        out += indentation + "        " + it->first + ":\n";
        for(int i = 0; i < it->second.size(); ++i)
            out += indentation + "        -   " + it->second[i] + "\n";
    }
    
    out += indentation + "    OverrideCompileFlags:\n";
    for(auto it = OverrideCompileFlags.begin(); it != OverrideCompileFlags.end(); ++it)
    {
        out += indentation + "        " + it->first + ":\n";
        out += it->second.ToString(indentation + "            ");
    }
    
    out += indentation + "    OverrideLinkFlags:\n";
    for(auto it = OverrideLinkFlags.begin(); it != OverrideLinkFlags.end(); ++it)
    {
        out += indentation + "        " + it->first + ":\n";
        out += it->second.ToString(indentation + "            ");
    }
    
    out += indentation + "    OtherFilesToBeCompiled:\n";
    for(auto it = OtherFilesToBeCompiled.begin(); it != OtherFilesToBeCompiled.end(); ++it)
    {
        out += indentation + "        " + it->first + ":\n";
        out += it->second.ToString(indentation + "            ");
    }
    
    out += indentation + "    Dependencies:\n";
    for(int i = 0; i < Dependencies.size(); ++i)
        out += Dependencies[i].ToString(indentation + "    ");
    
    out += indentation + "    Defines:\n";
    for(auto it = Defines.begin(); it != Defines.end(); ++it)
    {
        out += indentation + "        " + it->first + ":\n";
        out += it->second.ToString(indentation + "            ");
    }
    
    return out;
}
