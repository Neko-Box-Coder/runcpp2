#include "runcpp2/Data/FileProperties.hpp"
#include "runcpp2/Data/ParseCommon.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "ssLogger/ssLog.hpp"

bool runcpp2::Data::FileProperties::ParseYAML_Node(ryml::ConstNodeRef& node)
{
    INTERNAL_RUNCPP2_SAFE_START();

    std::vector<NodeRequirement> requirements =
    {
        NodeRequirement("Prefix", ryml::NodeType_e::MAP, true, true),
        NodeRequirement("Extension", ryml::NodeType_e::MAP, true, false)
    };
    
    if(!CheckNodeRequirements(node, requirements))
    {
        ssLOG_ERROR("File Properties: Failed to meet requirements");
        return false;
    }
    
    if(ExistAndHasChild(node, "Prefix"))
    {
        for(int i = 0; i < node["Prefix"].num_children(); ++i)
        {
            std::string key = GetKey(node["Prefix"][i]);
            std::string value = GetValue(node["Prefix"][i]);
            Prefix[key] = value;
        }
    }
    
    if(ExistAndHasChild(node, "Extension"))
    {
        for(int i = 0; i < node["Extension"].num_children(); ++i)
        {
            std::string key = GetKey(node["Extension"][i]);
            std::string value = GetValue(node["Extension"][i]);
            Extension[key] = value;
        }
    }
    
    return true;
    
    INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
}

std::string runcpp2::Data::FileProperties::ToString(std::string indentation) const
{
    std::string out;
    
    if(!Prefix.empty())
        out += indentation + "Prefix: \n";
    
    for(auto it = Prefix.begin(); it != Prefix.end(); ++it)
        out += indentation + "    " + it->first + ": " + it->second + "\n";
    
    if(!Extension.empty())
        out += indentation + "Extension: \n";
    
    for(auto it = Extension.begin(); it != Extension.end(); ++it)
        out += indentation + "    " + it->first + ": " + it->second + "\n";
    
    out += "\n";
    return out;
}
