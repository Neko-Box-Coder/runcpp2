#ifndef RUNCPP2_DATA_LINK_SOURCE_HPP
#define RUNCPP2_DATA_LINK_SOURCE_HPP

#include "runcpp2/ParseUtil.hpp"
#include "runcpp2/LibYAML_Wrapper.hpp"

#include "DSResult/DSResult.hpp"
#include "ssLogger/ssLog.hpp"

#include <string>
#include <vector>

namespace runcpp2
{
namespace Data
{
    struct LinkSource
    {
        std::string URL; //TODO(NOW): Make this Platform profile map
        std::string Name;
        
        inline bool ParseYAML_Node(YAML::ConstNodePtr node)
        {
            std::vector<NodeRequirement> requirements =
            {
                NodeRequirement("URL", YAML::NodeType::Scalar, true, false),
                NodeRequirement("Name", YAML::NodeType::Scalar, true, false),
            };
            
            if(!CheckNodeRequirements(node, requirements))
            {
                ssLOG_ERROR("LinkSource: Failed to meet requirements");
                return false;
            }
            
            URL = node->GetMapValueScalar<std::string>("URL").value();
            Name = node->GetMapValueScalar<std::string>("Name").value();
            
            return true;
        }

        inline std::string ToString(std::string indentation) const
        {
            std::string out;
            out += indentation + "Link:\n";
            out += indentation + "    URL: " + GetEscapedYAMLString(URL) + "\n";
            out += indentation + "    Name: " + GetEscapedYAMLString(Name) + "\n";
            return out;
        }

        inline bool Equals(const LinkSource& other) const
        {
            return URL == other.URL && Name == other.Name;
        }
    };
}
}

#endif 
