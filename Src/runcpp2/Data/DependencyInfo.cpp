#include "runcpp2/Data/DependencyInfo.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "runcpp2/Data/ParseCommon.hpp"
#include "ssLogger/ssLog.hpp"

bool runcpp2::Data::DependencyInfo::ParseYAML_Node(ryml::ConstNodeRef& node)
{
    std::vector<NodeRequirement> requirements =
    {
        NodeRequirement("Name", ryml::NodeType_e::KEYVAL, true, false),
        NodeRequirement("Platforms", ryml::NodeType_e::SEQ, true, false),
        NodeRequirement("Source", ryml::NodeType_e::MAP, true, false),
        NodeRequirement("LibraryType", ryml::NodeType_e::KEYVAL, true, false),
        NodeRequirement("IncludePaths", ryml::NodeType_e::SEQ, false, true),
        NodeRequirement("LinkProperties", ryml::NodeType_e::MAP, false, false),
        NodeRequirement("Setup", ryml::NodeType_e::MAP, false, true)
    };
    
    if(!CheckNodeRequirements(node, requirements))
    {
        ssLOG_ERROR("DependencyInfo: Failed to meet requirements");
        return false;
    }
    
    node["Name"] >> Name;
    
    for(int i = 0; i < node["Platforms"].num_children(); ++i)
        Platforms.insert(GetValue(node["Platforms"][i]));
    
    ryml::ConstNodeRef sourceNode = node["Source"];
    if(!Source.ParseYAML_Node(sourceNode))
    {
        ssLOG_ERROR("DependencyInfo: Failed to parse Source");
        return false;
    }
    
    static_assert((int)DependencyLibraryType::COUNT == 4, "");
    
    if(node["LibraryType"].val() == "Static")
        LibraryType = DependencyLibraryType::STATIC;
    else if(node["LibraryType"].val() == "Shared")
        LibraryType = DependencyLibraryType::SHARED;
    else if(node["LibraryType"].val() == "Object")
        LibraryType = DependencyLibraryType::OBJECT;
    else if(node["LibraryType"].val() == "Header")
        LibraryType = DependencyLibraryType::HEADER;
    else
    {
        ssLOG_ERROR("DependencyInfo: LibraryType is invalid");
        return false;
    }
    
    if(ExistAndHasChild(node, "IncludePaths"))
    {
        ryml::ConstNodeRef includePathsNode = node["IncludePaths"];
        
        for(int i = 0; i < includePathsNode.num_children(); i++)
            IncludePaths.push_back(GetValue(includePathsNode[i]));
    }
    
    if(ExistAndHasChild(node, "LinkProperties"))
    {
        ryml::ConstNodeRef linkPropertiesNode = node["LinkProperties"];
        
        for(auto it = linkPropertiesNode.begin(); it != linkPropertiesNode.end(); ++it)
        
        for(int i = 0; i < linkPropertiesNode.num_children(); ++i)
        {
            ProfileName profile = GetKey(linkPropertiesNode[i]);
            ryml::ConstNodeRef currentPropertyNode = linkPropertiesNode[i];
            
            DependencyLinkProperty property;
            if(!property.ParseYAML_Node(currentPropertyNode))
            {
                ssLOG_ERROR("DependencyInfo: Failed to parse SearchProperties");
                return false;
            }
            
            LinkProperties[profile] = property;
        }
    }
    
    if(ExistAndHasChild(node, "Setup"))
    {
        for(int i = 0; i < node["Setup"].num_children(); ++i)
        {
            DependencySetup currentSetup;
            ryml::ConstNodeRef currentSetupNode = node["Setup"][i];
            
            if(!currentSetup.ParseYAML_Node(currentSetupNode))
            {
                ssLOG_ERROR("DependencyInfo: Failed to parse Setup");
                return false;
            }
            
            Setup[GetKey(node["Setup"][i])] = currentSetup;
        }
    }
    
    return true;
}

std::string runcpp2::Data::DependencyInfo::ToString(std::string indentation) const
{
    std::string out;
    out += indentation + "-   Name: " + Name + "\n";
    
    out += indentation + "    Platforms:\n";
    for(auto it = Platforms.begin(); it != Platforms.end(); ++it)
        out += indentation + "    -   " + *it + "\n";
    
    out += Source.ToString(indentation + "    ");
    
    static_assert((int)DependencyLibraryType::COUNT == 4, "");
    
    if(LibraryType == DependencyLibraryType::STATIC)
        out += indentation + "    LibraryType: Static\n";
    else if(LibraryType == DependencyLibraryType::SHARED)
        out += indentation + "    LibraryType: Shared\n";
    else if(LibraryType == DependencyLibraryType::OBJECT)
        out += indentation + "    LibraryType: Object\n";
    else if(LibraryType == DependencyLibraryType::HEADER)
        out += indentation + "    LibraryType: Header\n";
    
    out += indentation + "    IncludePaths:\n";
    for(auto it = IncludePaths.begin(); it != IncludePaths.end(); ++it)
        out += indentation + "    -   " + *it + "\n";
    
    out += indentation + "    LinkProperties:\n";
    for(auto it = LinkProperties.begin(); it != LinkProperties.end(); ++it)
    {
        out += indentation + "        " + it->first + ":\n";
        out += it->second.ToString(indentation + "            ");
    }
    
    out += indentation + "    Setup:\n";
    for(auto it = Setup.begin(); it != Setup.end(); ++it)
    {
        out += indentation + "        " + it->first + ":\n";
        out += it->second.ToString(indentation + "            ");
    }
    
    return out;
}