#include "runcpp2/Data/DependencyLinkProperty.hpp"
#include "runcpp2/Data/ParseCommon.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "ssLogger/ssLog.hpp"

bool runcpp2::Data::DependencyLinkProperty::ParseYAML_Node(ryml::ConstNodeRef& node)
{
    INTERNAL_RUNCPP2_SAFE_START();
    
    if(!node.is_map())
    {
        ssLOG_ERROR("DependencySearchProperty: Node is not a Map");
        return false;
    }
    
    std::vector<NodeRequirement> requirements =
    {
        NodeRequirement("SearchLibraryNames", ryml::NodeType_e::SEQ, true, false),
        NodeRequirement("SearchDirectories", ryml::NodeType_e::SEQ, true, false),
        NodeRequirement("ExcludeLibraryNames", ryml::NodeType_e::SEQ, false, true),
        NodeRequirement("AdditionalLinkOptions", ryml::NodeType_e::MAP, false, true)
    };
    
    if(!CheckNodeRequirements(node, requirements))
    {
        ssLOG_ERROR("DependencySource: Failed to meet requirements");
        return false;
    }
    
    for(int i = 0; i < node["SearchLibraryNames"].num_children(); ++i)
        SearchLibraryNames.push_back(GetValue(node["SearchLibraryNames"][i]));
    
    for(int i = 0; i < node["SearchDirectories"].num_children(); ++i)
        SearchDirectories.push_back(GetValue(node["SearchDirectories"][i]));

    for(int i = 0; i < node["ExcludeLibraryNames"].num_children(); ++i)
        ExcludeLibraryNames.push_back(GetValue(node["ExcludeLibraryNames"][i]));

    if(ExistAndHasChild(node, "AdditionalLinkOptions"))
    {
        ryml::ConstNodeRef additionalLinkNode = node["AdditionalLinkOptions"];

        for(int i = 0; i < additionalLinkNode.num_children(); ++i)
        {
            ryml::ConstNodeRef currentLinkNode = additionalLinkNode[i];
            
            if(!currentLinkNode.is_seq())
            {
                ssLOG_ERROR("Sequence is expected for AdditionalLinkOptions at " << 
                            GetKey(currentLinkNode));
                
                return false;
            }
            
            const std::string currentPlatform = GetKey(currentLinkNode);
            for(int j = 0; j < currentLinkNode.num_children(); ++j)
                AdditionalLinkOptions[currentPlatform].push_back(GetValue(currentLinkNode[j]));
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