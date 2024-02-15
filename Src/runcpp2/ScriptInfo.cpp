#include "runcpp2/ScriptInfo.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "ssLogger/ssLog.hpp"

namespace runcpp2
{
    bool ScriptInfo::ParseYAML_Node(YAML::Node& node)
    {
        std::vector<Internal::NodeRequirement> requirements =
        {
            Internal::NodeRequirement("Language", YAML::NodeType::Scalar, false, true),
            Internal::NodeRequirement("PreferredProfiles", YAML::NodeType::Sequence, false, true),
            Internal::NodeRequirement("Dependencies", YAML::NodeType::Sequence, false, true),
            Internal::NodeRequirement("OverrideCompileFlags", YAML::NodeType::Map, false, true),
            Internal::NodeRequirement("OverrideLinkFlags", YAML::NodeType::Map, false, true),
        };
        
        if(!Internal::CheckNodeRequirements(node, requirements))
        {
            ssLOG_ERROR("ScriptInfo: Failed to meet requirements");
            return false;
        }
        
        if(node["Language"])
            Language = node["Language"].as<std::string>();
        
        for(int i = 0; i < node["PreferredProfiles"].size(); ++i)
            PreferredProfiles.insert(node["PreferredProfiles"][i].as<std::string>());
        
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
        
        if(node["OverrideLinkFlags"])
        {
            YAML::Node overrideCompileFlagsNode = node["OverrideCompileFlags"];
            if(!OverrideLinkFlags.ParseYAML_Node(overrideCompileFlagsNode))
            {
                ssLOG_ERROR("ScriptInfo: Failed to parse OverrideCompileFlags");
                return false;
            }
        }
        
        if(node["OverrideLinkFlags"])
        {
            YAML::Node overrideLinkFlagsNode = node["OverrideLinkFlags"];
            if(!OverrideLinkFlags.ParseYAML_Node(overrideLinkFlagsNode))
            {
                ssLOG_ERROR("ScriptInfo: Failed to parse OverrideLinkFlags");
                return false;
            }
        }
        
        return true;
    }
    
    std::string ScriptInfo::ToString(std::string indentation) const
    {
        std::string out = indentation + "ScriptInfo:\n";
        
        if(!Language.empty())
            out += indentation + "    Language: " + Language + "\n";
        
        out += indentation + "    PreferredProfiles:\n";
        
        for(auto it = PreferredProfiles.begin(); it != PreferredProfiles.end(); ++it)
            out += indentation + "    -   " + *it + "\n";
        
        out += indentation + "    Dependencies:\n";
        for(int i = 0; i < Dependencies.size(); ++i)
            out += Dependencies[i].ToString(indentation + "        ");
        
        out += OverrideCompileFlags.ToString(indentation + "    ");
        out += OverrideLinkFlags.ToString(indentation + "    ");
        
        return out;
    }

}