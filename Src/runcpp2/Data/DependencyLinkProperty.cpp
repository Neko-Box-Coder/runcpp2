#include "runcpp2/Data/DependencyLinkProperty.hpp"
#include "runcpp2/Data/ParseCommon.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "ssLogger/ssLog.hpp"

bool runcpp2::DependencyLinkProperty::ParseYAML_Node(YAML::Node& node)
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
        NodeRequirement("SearchDirectories", YAML::NodeType::Sequence, true, false)
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
    
    return true;
    
    INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
}

std::string runcpp2::DependencyLinkProperty::ToString(std::string indentation) const
{
    std::string out;
    out += indentation + "SearchLibraryName: \n";
    
    for(int i = 0; i < SearchLibraryNames.size(); ++i)
        out += indentation + "-   " + SearchLibraryNames[i] + "\n";
    
    out += indentation + "SearchDirectories: \n";
    
    for(int i = 0; i < SearchDirectories.size(); ++i)
        out += indentation + "-   " + SearchDirectories[i] + "\n";
    
    return out;
}