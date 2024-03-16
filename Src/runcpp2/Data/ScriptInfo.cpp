#include "runcpp2/Data/ScriptInfo.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "runcpp2/Data/ParseCommon.hpp"
#include "ssLogger/ssLog.hpp"

bool runcpp2::Data::ScriptInfo::ParseYAML_Node(YAML::Node& node)
{
    INTERNAL_RUNCPP2_SAFE_START();
    
    std::vector<NodeRequirement> requirements =
    {
        NodeRequirement("Language", YAML::NodeType::Scalar, false, true),
        NodeRequirement("RequiredProfiles", YAML::NodeType::Map, false, true),
        NodeRequirement("OverrideCompileFlags", YAML::NodeType::Map, false, true),
        NodeRequirement("OverrideLinkFlags", YAML::NodeType::Map, false, true),
        NodeRequirement("Dependencies", YAML::NodeType::Sequence, false, true),
    };
    
    if(!CheckNodeRequirements(node, requirements))
    {
        ssLOG_ERROR("ScriptInfo: Failed to meet requirements");
        return false;
    }
    
    if(node["Language"])
        Language = node["Language"].as<std::string>();
    
    if(node["RequiredProfiles"])
    {
        for(auto it = node["RequiredProfiles"].begin(); it != node["RequiredProfiles"].end(); ++it)
        {
            if(it->second.Type() != YAML::NodeType::Sequence)
            {
                ssLOG_ERROR("ScriptInfo: RequiredProfiles requires a sequence");
                return false;
            }
            
            PlatformName platform = it->first.as<PlatformName>();
            std::vector<ProfileName> profiles;
            for(int i = 0; i < it->second.size(); ++i)
                profiles.push_back(it->second[i].as<ProfileName>());
            
            RequiredProfiles[platform] = profiles;
        }
    }
    
    if(node["OverrideCompileFlags"])
    {
        YAML::Node overrideCompileFlagsNode = node["OverrideCompileFlags"];
        
        for(auto it = overrideCompileFlagsNode.begin(); it != overrideCompileFlagsNode.end(); ++it)
        {
            ProfileName profile = it->first.as<ProfileName>();
            FlagsOverrideInfo compileFlags;
            
            if(!compileFlags.ParseYAML_Node(it->second))
            {
                ssLOG_ERROR("ScriptInfo: Failed to parse OverrideCompileFlags.");
                return false;
            }
            OverrideCompileFlags[profile] = compileFlags;
        }
    }
    
    if(node["OverrideLinkFlags"])
    {
        YAML::Node overrideLinkFlagsNode = node["OverrideLinkFlags"];
        
        for(auto it = overrideLinkFlagsNode.begin(); it != overrideLinkFlagsNode.end(); ++it)
        {
            ProfileName profile = it->first.as<ProfileName>();
            FlagsOverrideInfo linkFlags;
            
            if(!linkFlags.ParseYAML_Node(it->second))
            {
                ssLOG_ERROR("ScriptInfo: Failed to parse OverrideLinkFlags.");
                return false;
            }
            OverrideLinkFlags[profile] = linkFlags;
        }
    }
    
    for(int i = 0; i < node["Dependencies"].size(); ++i)
    {
        DependencyInfo info;
        YAML::Node dependencyNode = node["Dependencies"][i];
        
        if(!info.ParseYAML_Node(dependencyNode))
        {
            ssLOG_ERROR("ScriptInfo: Failed to parse DependencyInfo at index " << i);
            return false;
        }
        
        Dependencies.push_back(info);
    }
    
    return true;
    
    INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
}

std::string runcpp2::Data::ScriptInfo::ToString(std::string indentation) const
{
    std::string out = indentation + "ScriptInfo:\n";
    
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
    
    out += indentation + "    Dependencies:\n";
    for(int i = 0; i < Dependencies.size(); ++i)
        out += Dependencies[i].ToString(indentation + "    ");
    
    return out;
}