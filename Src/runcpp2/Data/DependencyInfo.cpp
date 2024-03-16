#include "runcpp2/Data/DependencyInfo.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "runcpp2/Data/ParseCommon.hpp"
#include "ssLogger/ssLog.hpp"

bool runcpp2::Data::DependencyInfo::ParseYAML_Node(YAML::Node& node)
{
    INTERNAL_RUNCPP2_SAFE_START();
    
    std::vector<NodeRequirement> requirements =
    {
        NodeRequirement("Name", YAML::NodeType::Scalar, true, false),
        NodeRequirement("Platforms", YAML::NodeType::Sequence, true, false),
        NodeRequirement("Source", YAML::NodeType::Map, true, false),
        NodeRequirement("LibraryType", YAML::NodeType::Scalar, true, false),
        NodeRequirement("IncludePaths", YAML::NodeType::Sequence, false, true),
        NodeRequirement("LinkProperties", YAML::NodeType::Map, false, false),
        NodeRequirement("Setup", YAML::NodeType::Map, false, true)
    };
    
    if(!CheckNodeRequirements(node, requirements))
    {
        ssLOG_ERROR("DependencyInfo: Failed to meet requirements");
        return false;
    }
    
    Name = node["Name"].as<std::string>();
    
    for(int i = 0; i < node["Platforms"].size(); ++i)
        Platforms.insert(node["Platforms"][i].as<std::string>());
    
    YAML::Node sourceNode = node["Source"];
    if(!Source.ParseYAML_Node(sourceNode))
    {
        ssLOG_ERROR("DependencyInfo: Failed to parse Source");
        return false;
    }
    
    static_assert((int)DependencyLibraryType::COUNT == 4, "");
    
    if(node["LibraryType"].as<std::string>() == "Static")
        LibraryType = DependencyLibraryType::STATIC;
    else if(node["LibraryType"].as<std::string>() == "Shared")
        LibraryType = DependencyLibraryType::SHARED;
    else if(node["LibraryType"].as<std::string>() == "Object")
        LibraryType = DependencyLibraryType::OBJECT;
    else if(node["LibraryType"].as<std::string>() == "Header")
        LibraryType = DependencyLibraryType::HEADER;
    else
    {
        ssLOG_ERROR("DependencyInfo: LibraryType is invalid");
        return false;
    }
    
    if(node["IncludePaths"])
    {
        YAML::Node includePathsNode = node["IncludePaths"];
        
        for(int i = 0; i < includePathsNode.size(); i++)
            IncludePaths.push_back(includePathsNode[i].as<std::string>());
    }
    
    if(node["LinkProperties"])
    {
        YAML::Node linkPropertiesNode = node["LinkProperties"];
        
        for(auto it = linkPropertiesNode.begin(); it != linkPropertiesNode.end(); ++it)
        {
            ProfileName profile = it->first.as<ProfileName>();
            DependencyLinkProperty property;
            if(!property.ParseYAML_Node(it->second))
            {
                ssLOG_ERROR("DependencyInfo: Failed to parse SearchProperties");
                return false;
            }
            
            LinkProperties[profile] = property;
        }
    }
    
    if(node["Setup"])
    {
        for(auto it = node["Setup"].begin(); it != node["Setup"].end(); ++it)
        {
            DependencySetup currentSetup;
            
            if(!currentSetup.ParseYAML_Node(it->second))
            {
                ssLOG_ERROR("DependencyInfo: Failed to parse Setup");
                return false;
            }
            
            Setup[it->first.as<std::string>()] = currentSetup;
        }
    }
    
    return true;
    
    INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
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