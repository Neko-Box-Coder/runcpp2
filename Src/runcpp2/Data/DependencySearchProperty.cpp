#include "runcpp2/Data/DependencySearchProperty.hpp"
#include "runcpp2/Data/ParseCommon.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "ssLogger/ssLog.hpp"

namespace runcpp2
{
    bool DependencySearchProperty::ParseYAML_Node(YAML::Node& node)
    {
        INTERNAL_RUNCPP2_SAFE_START();
        
        if(!node.IsMap())
        {
            ssLOG_ERROR("DependencySearchProperty: Node is not a Map");
            return false;
        }
        
        std::vector<Internal::NodeRequirement> requirements =
        {
            Internal::NodeRequirement("SearchLibraryName", YAML::NodeType::Scalar, true, false),
            Internal::NodeRequirement("SearchPath", YAML::NodeType::Scalar, true, false)
        };
        
        if(!Internal::CheckNodeRequirements(node, requirements))
        {
            ssLOG_ERROR("DependencySource: Failed to meet requirements");
            return false;
        }
        
        SearchLibraryName = node["SearchLibraryName"].as<std::string>();
        SearchPath = node["SearchPath"].as<std::string>();
        return true;
        
        INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
    }
    
    std::string DependencySearchProperty::ToString(std::string indentation) const
    {
        std::string out;
        out += indentation + "SearchLibraryName: " + SearchLibraryName + "\n";
        out += indentation + "SearchPath: " + SearchPath + "\n";
        
        return out;
    }
}