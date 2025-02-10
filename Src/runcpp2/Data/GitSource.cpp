#include "runcpp2/Data/GitSource.hpp"
#include "runcpp2/Data/ParseCommon.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "ssLogger/ssLog.hpp"

bool runcpp2::Data::GitSource::ParseYAML_Node(ryml::ConstNodeRef& node)
{
    INTERNAL_RUNCPP2_SAFE_START();
    
    std::vector<NodeRequirement> requirements =
    {
        NodeRequirement("URL", ryml::NodeType_e::KEYVAL, true, false),
        NodeRequirement("Branch", ryml::NodeType_e::KEYVAL, false, false),
        NodeRequirement("FullHistory", ryml::NodeType_e::KEYVAL, false, false)
    };
    
    if(!CheckNodeRequirements(node, requirements))
    {
        ssLOG_ERROR("GitSource: Failed to meet requirements");
        return false;
    }
    
    node["URL"] >> URL;
    if(ExistAndHasChild(node, "Branch"))
        node["Branch"] >> Branch;
    
    if(ExistAndHasChild(node, "FullHistory"))
        node["FullHistory"] >> FullHistory;
    
    return true;
    
    INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
}

std::string runcpp2::Data::GitSource::ToString(std::string indentation) const
{
    std::string out;
    out += indentation + "Git:\n";
    out += indentation + "    URL: " + GetEscapedYAMLString(URL) + "\n";
    if(!Branch.empty())
        out += indentation + "    Branch: " + GetEscapedYAMLString(Branch) + "\n";
    out += indentation + "    FullHistory: " + (FullHistory ? "true" : "false") + "\n";
    return out;
}

bool runcpp2::Data::GitSource::Equals(const GitSource& other) const
{
    return URL == other.URL && Branch == other.Branch && FullHistory == other.FullHistory;
} 
