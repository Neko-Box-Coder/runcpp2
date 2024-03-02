#include "runcpp2/Data/FlagsOverrideInfo.hpp"
#include "runcpp2/Data/ParseCommon.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "ssLogger/ssLog.hpp"


namespace runcpp2
{
    bool FlagsOverrideInfo::ParseYAML_Node(YAML::Node& node)
    {
        INTERNAL_RUNCPP2_SAFE_START();

        std::vector<Internal::NodeRequirement> requirements =
        {
            Internal::NodeRequirement("Remove", YAML::NodeType::Scalar, false, true),
            Internal::NodeRequirement("Append", YAML::NodeType::Scalar, false, true)
        };
        
        if(!Internal::CheckNodeRequirements(node, requirements))
        {
            ssLOG_ERROR("FlagsOverrideInfo: Failed to meet requirements");
            return false;
        }
        
        if(node["Remove"])
            Remove = node["Remove"].as<std::string>();
        
        if(node["Append"])
            Append = node["Append"].as<std::string>();
        
        return true;
        
        INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
    }
    
    std::string FlagsOverrideInfo::ToString(std::string indentation) const
    {
        std::string out;
        out += indentation + "Remove: " + Remove + "\n";
        out += indentation + "Append: " + Append + "\n";
        
        return out;
    }

}