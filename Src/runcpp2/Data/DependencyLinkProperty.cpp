#include "runcpp2/Data/DependencyLinkProperty.hpp"
#include "runcpp2/Data/ParseCommon.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "ssLogger/ssLog.hpp"

bool runcpp2::Data::DependencyLinkProperty::ParseYAML_Node(YAML::Node& node)
{
    INTERNAL_RUNCPP2_SAFE_START();
    
    if(!node.IsMap())
    {
        ssLOG_ERROR("DependencySearchProperty: Node is not a Map");
        return false;
    }
    
    std::vector<NodeRequirement> requirements =
    {
        NodeRequirement("SearchLibraryNames", YAML::NodeType::Sequence, true, false),
        NodeRequirement("SearchDirectories", YAML::NodeType::Sequence, true, false),
        NodeRequirement("ExcludeLibraryNames", YAML::NodeType::Sequence, false, true),
        NodeRequirement("AdditionalLinkOptions", YAML::NodeType::Map, false, true)
    };
    
    if(!CheckNodeRequirements(node, requirements))
    {
        ssLOG_ERROR("DependencySource: Failed to meet requirements");
        return false;
    }
    
    for(int i = 0; i < node["SearchLibraryNames"].size(); ++i)
        SearchLibraryNames.push_back(node["SearchLibraryNames"][i].as<std::string>());
    
    for(int i = 0; i < node["SearchDirectories"].size(); ++i)
        SearchDirectories.push_back(node["SearchDirectories"][i].as<std::string>());

    for(int i = 0; i < node["ExcludeLibraryNames"].size(); ++i)
        ExcludeLibraryNames.push_back(node["ExcludeLibraryNames"][i].as<std::string>());

    if(node["AdditionalLinkOptions"])
    {
        YAML::Node additionalLinkNode = node["AdditionalLinkOptions"];

        for(auto it = additionalLinkNode.begin(); it != additionalLinkNode.end(); ++it)
        {
            if(!it->second.IsSequence())
            {
                ssLOG_ERROR("Sequence is expected for AdditionalLinkOptions at " << 
                            it->first.as<std::string>());
                
                return false;
            }
            
            const std::string currentPlatform = it->first.as<std::string>();
            for(int i = 0; i < it->second.size(); ++i)
            {
                std::string currentOption = it->second[i].as<std::string>();
                AdditionalLinkOptions[currentPlatform].push_back(currentOption);
            }
        }
    }

    return true;
    
    INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
}

std::string runcpp2::Data::DependencyLinkProperty::ToString(std::string indentation) const
{
    std::string out;
    out += indentation + "SearchLibraryName: \n";
    
    for(int i = 0; i < SearchLibraryNames.size(); ++i)
        out += indentation + "-   " + SearchLibraryNames[i] + "\n";
    
    out += indentation + "SearchDirectories: \n";
    
    for(int i = 0; i < SearchDirectories.size(); ++i)
        out += indentation + "-   " + SearchDirectories[i] + "\n";
    
    out += indentation + "AdditionalLinkOptions: \n";
    
    for(auto it = AdditionalLinkOptions.begin(); it != AdditionalLinkOptions.end(); ++it)
    {
        out += indentation + "-   " + it->first + ":\n";
        for(int i = 0; i < it->second.size(); ++i)
            out += indentation + "    -   " + it->second[i] + "\n";
    }
    
    return out;
}