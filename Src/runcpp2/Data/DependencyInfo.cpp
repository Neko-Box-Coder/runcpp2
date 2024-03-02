#include "runcpp2/Data/DependencyInfo.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "runcpp2/Data/ParseCommon.hpp"
#include "ssLogger/ssLog.hpp"


namespace runcpp2
{
    bool DependencyInfo::ParseYAML_Node(YAML::Node& node)
    {
        INTERNAL_RUNCPP2_SAFE_START();
        
        std::vector<Internal::NodeRequirement> requirements =
        {
            Internal::NodeRequirement("Name", YAML::NodeType::Scalar, true, false),
            Internal::NodeRequirement("Platforms", YAML::NodeType::Sequence, true, false),
            Internal::NodeRequirement("Source", YAML::NodeType::Map, true, false),
            Internal::NodeRequirement("LibraryType", YAML::NodeType::Scalar, true, false),
            Internal::NodeRequirement("IncludePaths", YAML::NodeType::Sequence, false, true),
            Internal::NodeRequirement("SearchProperties", YAML::NodeType::Map, false, false),
            Internal::NodeRequirement("Setup", YAML::NodeType::Map, false, true)
        };
        
        if(!Internal::CheckNodeRequirements(node, requirements))
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
        
        if(node["SearchProperties"])
        {
            YAML::Node searchPropertiesNode = node["SearchProperties"];
            
            for(auto it = searchPropertiesNode.begin(); it != searchPropertiesNode.end(); ++it)
            {
                ProfileName profile = it->first.as<ProfileName>();
                DependencySearchProperty property;
                if(!property.ParseYAML_Node(it->second))
                {
                    ssLOG_ERROR("DependencyInfo: Failed to parse SearchProperties");
                    return false;
                }
                
                SearchProperties[profile] = property;
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
    
    std::string DependencyInfo::ToString(std::string indentation) const
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
        
        out += indentation + "    SearchProperties:\n";
        for(auto it = SearchProperties.begin(); it != SearchProperties.end(); ++it)
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

}