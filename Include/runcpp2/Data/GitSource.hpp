#ifndef RUNCPP2_DATA_GIT_SOURCE_HPP
#define RUNCPP2_DATA_GIT_SOURCE_HPP

#include "runcpp2/Data/SubmoduleInitType.hpp"
#include "runcpp2/Data/ParseCommon.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "runcpp2/YamlLib.hpp"

#include "ssLogger/ssLog.hpp"
#include <string>

namespace runcpp2
{
namespace Data
{
    struct GitSource
    {
        std::string URL;
        std::string Branch;
        bool FullHistory = false;
        SubmoduleInitType CurrentSubmoduleInitType = SubmoduleInitType::SHALLOW;
        
        inline bool ParseYAML_Node(YAML::ConstNodePtr node)
        {
            std::vector<NodeRequirement> requirements =
            {
                NodeRequirement("URL", YAML::NodeType::Scalar, true, false),
                NodeRequirement("Branch", YAML::NodeType::Scalar, false, false),
                NodeRequirement("FullHistory", YAML::NodeType::Scalar, false, false),
                NodeRequirement("SubmoduleInitType", YAML::NodeType::Scalar, false, false)
            };
            
            if(!CheckNodeRequirements(node, requirements))
            {
                ssLOG_ERROR("GitSource: Failed to meet requirements");
                return false;
            }
            
            URL = node->GetMapValueScalar<std::string>("URL").value();
            
            if(ExistAndHasChild(node, "Branch"))
            {
                Branch = node->GetMapValueScalar<std::string>("Branch").DS_TRY_ACT(return false);
            }
            if(ExistAndHasChild(node, "FullHistory"))
            {
                FullHistory = node->GetMapValueScalar<bool>("FullHistory").DS_TRY_ACT(return false);
            }
            if(ExistAndHasChild(node, "SubmoduleInitType"))
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

        inline std::string ToString(std::string indentation) const
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

        inline bool Equals(const GitSource& other) const
        {
            return  URL == other.URL && 
                    Branch == other.Branch && 
                    FullHistory == other.FullHistory &&
                    CurrentSubmoduleInitType == other.CurrentSubmoduleInitType;
        }
    };
}
}

#endif 
