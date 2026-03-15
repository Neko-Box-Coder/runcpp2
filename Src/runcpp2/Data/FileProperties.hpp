#ifndef RUNCPP2_DATA_FILE_PROPERTIES_HPP
#define RUNCPP2_DATA_FILE_PROPERTIES_HPP

#include "runcpp2/Data/ParseCommon.hpp"
#include "runcpp2/LibYAML_Wrapper.hpp"
#include "runcpp2/ParseUtil.hpp"

#include "ssLogger/ssLog.hpp"
#include "DSResult/DSResult.hpp"

#include <unordered_map>
#include <string>
#include <utility>
#include <vector>

namespace runcpp2
{
namespace Data
{
    struct FileProperties
    {
        std::unordered_map<PlatformName, std::string> Prefix;
        std::unordered_map<PlatformName, std::string> Extension;
        
        inline bool ParseYAML_Node(YAML::ConstNodePtr node)
        {
            std::vector<NodeRequirement> requirements =
            {
                NodeRequirement("Prefix", YAML::NodeType::Map, true, true),
                NodeRequirement("Extension", YAML::NodeType::Map, true, false)
            };
            
            if(!CheckNodeRequirements(node, requirements))
            {
                ssLOG_ERROR("File Properties: Failed to meet requirements");
                return false;
            }
            
            if(ExistAndHasChild(node, "Prefix"))
            {
                YAML::ConstNodePtr prefixNode = node->GetMapValueNode("Prefix");
                for(int i = 0; i < prefixNode->GetChildrenCount(); ++i)
                {
                    std::string key = prefixNode->GetMapKeyScalarAt<std::string>(i)
                                                .DS_TRY_ACT(return false);
                    std::string value = prefixNode  ->GetMapValueScalarAt<std::string>(i)
                                                    .DS_TRY_ACT(return false);
                    Prefix[key] = value;
                }
            }
            
            if(ExistAndHasChild(node, "Extension"))
            {
                YAML::ConstNodePtr extensionNode = node->GetMapValueNode("Extension");
                for(int i = 0; i < extensionNode->GetChildrenCount(); ++i)
                {
                    std::string key = extensionNode ->GetMapKeyScalarAt<std::string>(i)
                                                    .DS_TRY_ACT(return false);
                    std::string value = extensionNode   ->GetMapValueScalarAt<std::string>(i)
                                                        .DS_TRY_ACT(return false);
                    Extension[key] = value;
                }
            }
            
            return true;
        }

        inline std::string ToString(std::string indentation) const
        {
            std::string out;
            
            if(!Prefix.empty())
            {
                out += indentation + "Prefix:\n";
                for(const auto& it : Prefix)
                {
                    out +=  indentation + "    " + it.first + ": " + 
                            GetEscapedYAMLString(it.second) + "\n";
                }
            }
            
            if(!Extension.empty())
            {
                out += indentation + "Extension:\n";
                for(const auto& it : Extension)
                {
                    out +=  indentation + "    " + it.first + ": " + 
                            GetEscapedYAMLString(it.second) + "\n";
                }
            }
            
            out += "\n";
            return out;
        }

        inline bool Equals(const FileProperties& other) const
        {
            if(Prefix.size() != other.Prefix.size() || Extension.size() != other.Extension.size())
                return false;

            for(const auto& it : Prefix)
            {
                if(other.Prefix.count(it.first) == 0 || other.Prefix.at(it.first) != it.second)
                    return false;
            }
            
            for(const auto& it : Extension)
            {
                if(other.Extension.count(it.first) == 0 || other.Extension.at(it.first) != it.second)
                    return false;
            }
            
            return true;
        }
    };
}
}

#endif
