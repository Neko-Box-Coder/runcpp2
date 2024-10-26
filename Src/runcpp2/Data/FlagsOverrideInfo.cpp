#include "runcpp2/Data/FlagsOverrideInfo.hpp"
#include "runcpp2/Data/ParseCommon.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "ssLogger/ssLog.hpp"

bool runcpp2::Data::FlagsOverrideInfo::ParseYAML_Node(ryml::ConstNodeRef& node)
{
    INTERNAL_RUNCPP2_SAFE_START();

    std::vector<NodeRequirement> requirements =
    {
        NodeRequirement("Remove", ryml::NodeType_e::KEYVAL, false, true),
        NodeRequirement("Append", ryml::NodeType_e::KEYVAL, false, true)
    };
    
    if(!CheckNodeRequirements(node, requirements))
    {
        ssLOG_ERROR("FlagsOverrideInfo: Failed to meet requirements");
        return false;
    }
    
    if(ExistAndHasChild(node, "Remove"))
        node["Remove"] >> Remove;
    
    if(ExistAndHasChild(node, "Append"))
        node["Append"] >> Append;
    
    return true;
    
    INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
}

std::string runcpp2::Data::FlagsOverrideInfo::ToString(std::string indentation) const
{
    std::string out;
    out += indentation + "Remove: " + Remove + "\n";
    out += indentation + "Append: " + Append + "\n";
    
    return out;
}

bool runcpp2::Data::FlagsOverrideInfo::Equals(const FlagsOverrideInfo& other) const
{
    return Remove == other.Remove && Append == other.Append;
}
