#include "runcpp2/DependencyInfo.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "ssLogger/ssLog.hpp"


namespace runcpp2
{
    bool DependencyInfo::ParseYAML_Node(YAML::Node& node)
    {
        std::vector<Internal::NodeRequirement> requirements =
        {
            Internal::NodeRequirement("Name", YAML::NodeType::Scalar, true, false),
            Internal::NodeRequirement("Platforms", YAML::NodeType::Sequence, true, false),
            Internal::NodeRequirement("LibraryType", YAML::NodeType::Scalar, true, false),
            Internal::NodeRequirement("SearchLibraryName", YAML::NodeType::Scalar, true, false),
            Internal::NodeRequirement("Source", YAML::NodeType::Map, true, false),
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
        
        SearchLibraryName = node["SearchLibraryName"].as<std::string>();
        
        YAML::Node sourceNode = node["Source"];
        if(!Source.ParseYAML_Node(sourceNode))
        {
            ssLOG_ERROR("DependencyInfo: Failed to parse Source");
            return false;
        }
        
        for(auto it = node["Setup"].begin(); it != node["Setup"].end(); ++it)
        {
            std::vector<std::string> currentSetup;
            for(int i = 0; i < it->second.size(); ++i)
                currentSetup.push_back(it->second[i].as<std::string>());
            
            Setup[it->first.as<std::string>()] = currentSetup;
        }
        
        return true;
    }
    
    std::string DependencyInfo::ToString(std::string indentation) const
    {
        std::string out = indentation + "DependencyInfo:\n";
        
        out += indentation + "    Name: " + Name + "\n";
        
        out += indentation + "    Platforms:\n";
        for(auto it = Platforms.begin(); it != Platforms.end(); ++it)
            out += indentation + "    -   " + *it + "\n";
        
        static_assert((int)DependencyLibraryType::COUNT == 4, "");
        
        if(LibraryType == DependencyLibraryType::STATIC)
            out += indentation + "    LibraryType: Static\n";
        else if(LibraryType == DependencyLibraryType::SHARED)
            out += indentation + "    LibraryType: Shared\n";
        else if(LibraryType == DependencyLibraryType::OBJECT)
            out += indentation + "    LibraryType: Object\n";
        else if(LibraryType == DependencyLibraryType::HEADER)
            out += indentation + "    LibraryType: Header\n";
        
        out += indentation + "    SearchLibraryName: " + SearchLibraryName + "\n";
        out += Source.ToString(indentation + "    ");
        
        out += indentation + "    Setup:\n";
        for(auto it = Setup.begin(); it != Setup.end(); ++it)
        {
            out += indentation + "    " + it->first + ":\n";
            for(int i = 0; i < it->second.size(); ++i)
                out += indentation + "    -   " + it->second[i] + "\n";
        }
        
        return out;
    }

}