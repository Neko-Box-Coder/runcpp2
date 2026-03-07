#ifndef RUNCPP2_DATA_FLAGS_OVERRIDE_INFO_HPP
#define RUNCPP2_DATA_FLAGS_OVERRIDE_INFO_HPP

#include "runcpp2/Data/ParseCommon.hpp"
#include "runcpp2/YamlLib.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "ssLogger/ssLog.hpp"

#include <string>

namespace runcpp2
{
namespace Data
{
    struct FlagsOverrideInfo
    {
        std::string Remove;
        std::string Append;
        
        inline bool ParseYAML_Node(YAML::ConstNodePtr node)
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

        inline bool IsYAML_NodeParsableAsDefault(YAML::ConstNodePtr node) const
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

        inline std::string ToString(std::string indentation) const
        {
            std::string out;
            
            if(!Remove.empty())
                out += indentation + "Remove: " + GetEscapedYAMLString(Remove) + "\n";
            
            if(!Append.empty())
                out += indentation + "Append: " + GetEscapedYAMLString(Append) + "\n";
            
            return out;
        }

        inline bool Equals(const FlagsOverrideInfo& other) const
        {
            return Remove == other.Remove && Append == other.Append;
        }
    };
}
}


#endif
