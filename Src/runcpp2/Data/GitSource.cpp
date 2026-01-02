#include "runcpp2/Data/GitSource.hpp"
#include "runcpp2/Data/ParseCommon.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "ssLogger/ssLog.hpp"

bool runcpp2::Data::GitSource::ParseYAML_Node(YAML::ConstNodePtr node)
{
    std::vector<NodeRequirement> requirements =
    {
        NodeRequirement("URL", YAML::NodeType::Scalar, true, false),
        NodeRequirement("Branch", YAML::NodeType::Scalar, false, false),
        NodeRequirement("FullHistory", YAML::NodeType::Scalar, false, false),
        NodeRequirement("SubmoduleInitType", YAML::NodeType::Scalar, false, false)
    };
    
    if(!CheckNodeRequirements_LibYaml(node, requirements))
    {
        ssLOG_ERROR("GitSource: Failed to meet requirements");
        return false;
    }
    
    URL = node->GetMapValueScalar<std::string>("URL").value();
    
    if(ExistAndHasChild_LibYaml(node, "Branch"))
    {
        Branch = node->GetMapValueScalar<std::string>("Branch").DS_TRY_ACT(return false);
    }
    if(ExistAndHasChild_LibYaml(node, "FullHistory"))
    {
        FullHistory = node->GetMapValueScalar<bool>("FullHistory").DS_TRY_ACT(return false);
    }
    if(ExistAndHasChild_LibYaml(node, "SubmoduleInitType"))
    {
        std::string submoduleTypeString = 
            node->GetMapValueScalar<std::string>("SubmoduleInitType").value();
        
        CurrentSubmoduleInitType = StringToSubmoduleInitType(submoduleTypeString);
        if(CurrentSubmoduleInitType == SubmoduleInitType::COUNT)
        {
            ssLOG_ERROR("GitSource: Invalid submodule init type " << submoduleTypeString);
            return false;
        }
    }
    
    return true;
}

std::string runcpp2::Data::GitSource::ToString(std::string indentation) const
{
    std::string out;
    out += indentation + "Git:\n";
    out += indentation + "    URL: " + GetEscapedYAMLString(URL) + "\n";
    if(!Branch.empty())
        out += indentation + "    Branch: " + GetEscapedYAMLString(Branch) + "\n";
    out += indentation + "    FullHistory: " + (FullHistory ? "true" : "false") + "\n";
    out +=  indentation + 
            "    SubmoduleInitType: " + 
            SubmoduleInitTypeToString(CurrentSubmoduleInitType) + 
            "\n";
    return out;
}

bool runcpp2::Data::GitSource::Equals(const GitSource& other) const
{
    return  URL == other.URL && 
            Branch == other.Branch && 
            FullHistory == other.FullHistory &&
            CurrentSubmoduleInitType == other.CurrentSubmoduleInitType;
} 
