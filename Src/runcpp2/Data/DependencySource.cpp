#include "runcpp2/Data/DependencySource.hpp"
#include "runcpp2/Data/ParseCommon.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "ssLogger/ssLog.hpp"

namespace runcpp2
{
    bool DependencySource::ParseYAML_Node(YAML::Node& node)
    {
        INTERNAL_RUNCPP2_SAFE_START();
        
        std::vector<Internal::NodeRequirement> requirements =
        {
            Internal::NodeRequirement("Type", YAML::NodeType::Scalar, true, false),
            Internal::NodeRequirement("Value", YAML::NodeType::Scalar, true, false)
        };
        
        if(!Internal::CheckNodeRequirements(node, requirements))
        {
            ssLOG_ERROR("DependencySource: Failed to meet requirements");
            return false;
        }
        
        static_assert((int)DependencySourceType::COUNT == 2, "");
        
        if(node["Type"].as<std::string>() == "Git")
            Type = DependencySourceType::GIT;
        else if(node["Type"].as<std::string>() == "Local")
            Type = DependencySourceType::LOCAL;
        else
        {
            ssLOG_ERROR("DependencySource: Type is invalid");
            return false;
        }
        
        Value = node["Value"].as<std::string>();
        return true;
        
        INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
    }
    
    std::string DependencySource::ToString(std::string indentation) const
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

}