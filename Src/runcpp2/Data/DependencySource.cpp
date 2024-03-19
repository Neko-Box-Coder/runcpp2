#include "runcpp2/Data/DependencySource.hpp"
#include "runcpp2/Data/ParseCommon.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "ssLogger/ssLog.hpp"

bool runcpp2::Data::DependencySource::ParseYAML_Node(ryml::ConstNodeRef& node)
{
    std::vector<NodeRequirement> requirements =
    {
        NodeRequirement("Type", ryml::NodeType_e::KEYVAL, true, false),
        NodeRequirement("Value", ryml::NodeType_e::KEYVAL, true, false)
    };
    
    if(!CheckNodeRequirements(node, requirements))
    {
        ssLOG_ERROR("DependencySource: Failed to meet requirements");
        return false;
    }
    
    static_assert((int)DependencySourceType::COUNT == 2, "");
    
    if(node["Type"].val() == "Git")
        Type = DependencySourceType::GIT;
    else if(node["Type"].val() == "Local")
        Type = DependencySourceType::LOCAL;
    else
    {
        ssLOG_ERROR("DependencySource: Type is invalid");
        return false;
    }
    
    node["Value"] >> Value;
    return true;
}

std::string runcpp2::Data::DependencySource::ToString(std::string indentation) const
{
    std::string out = indentation + "Source:\n";
    
    static_assert((int)DependencySourceType::COUNT == 2, "");
    
    if(Type == DependencySourceType::GIT)
        out += indentation + "    Type: Git\n";
    else if(Type == DependencySourceType::LOCAL)
        out += indentation + "    Type: Local\n";
    
    out += indentation + "    Value: " + Value + "\n";
    
    return out;
}