#include "runcpp2/Data/FlagsOverrideInfo.hpp"
#include "runcpp2/Data/ParseCommon.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "ssLogger/ssLog.hpp"

bool runcpp2::Data::FlagsOverrideInfo::ParseYAML_Node(YAML::ConstNodePtr node)
{
    ssLOG_FUNC_DEBUG();

    std::vector<NodeRequirement> requirements =
    {
        NodeRequirement("Remove", YAML::NodeType::Scalar, false, true),
        NodeRequirement("Append", YAML::NodeType::Scalar, false, true)
    };
    
    if(!CheckNodeRequirements(node, requirements))
    {
        ssLOG_ERROR("FlagsOverrideInfo: Failed to meet requirements");
        return false;
    }
    
    if(ExistAndHasChild(node, "Remove"))
    {
        Remove = node->GetMapValueScalar<std::string>("Remove").DS_TRY_ACT(return false);
    }
    
    if(ExistAndHasChild(node, "Append"))
    {
        Append = node->GetMapValueScalar<std::string>("Append").DS_TRY_ACT(return false);
    }
    
    return true;
}

bool runcpp2::Data::FlagsOverrideInfo::IsYAML_NodeParsableAsDefault(YAML::ConstNodePtr node) const
{
    ssLOG_FUNC_DEBUG();
    
    if(!node->IsMap())
    {
        ssLOG_ERROR("FlagsOverrideInfo type requires a map");
        return false;
    }
    
    if(ExistAndHasChild(node, "Remove") || ExistAndHasChild(node, "Append"))
    {
        std::vector<NodeRequirement> requirements =
        {
            NodeRequirement("Remove", YAML::NodeType::Scalar, false, true),
            NodeRequirement("Append", YAML::NodeType::Scalar, false, true)
        };
        
        if(!CheckNodeRequirements(node, requirements))
            return false;
        
        return true;
    }
    
    return false;
}

std::string runcpp2::Data::FlagsOverrideInfo::ToString(std::string indentation) const
{
    std::string out;
    
    if(!Remove.empty())
        out += indentation + "Remove: " + GetEscapedYAMLString(Remove) + "\n";
    
    if(!Append.empty())
        out += indentation + "Append: " + GetEscapedYAMLString(Append) + "\n";
    
    return out;
}

bool runcpp2::Data::FlagsOverrideInfo::Equals(const FlagsOverrideInfo& other) const
{
    return Remove == other.Remove && Append == other.Append;
}
