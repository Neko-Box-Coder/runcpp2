#include "runcpp2/Data/LocalSource.hpp"
#include "runcpp2/Data/ParseCommon.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "ssLogger/ssLog.hpp"

bool runcpp2::Data::LocalSource::ParseYAML_Node(ryml::ConstNodeRef& node)
{
    INTERNAL_RUNCPP2_SAFE_START();
    
    std::vector<NodeRequirement> requirements =
    {
        NodeRequirement("Path", ryml::NodeType_e::KEYVAL, true, false)
    };
    
    if(!CheckNodeRequirements(node, requirements))
    {
        ssLOG_ERROR("LocalSource: Failed to meet requirements");
        return false;
    }
    
    node["Path"] >> Path;
    return true;
    
    INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
}

std::string runcpp2::Data::LocalSource::ToString(std::string indentation) const
{
    std::string out;
    out += indentation + "Local:\n";
    out += indentation + "    Path: " + GetEscapedYAMLString(Path) + "\n";
    return out;
}

bool runcpp2::Data::LocalSource::Equals(const LocalSource& other) const
{
    return Path == other.Path;
} 