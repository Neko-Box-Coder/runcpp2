#ifndef RUNCPP2_DATA_SOURCE_PATH_HPP
#define RUNCPP2_DATA_SOURCE_PATH_HPP


#include "runcpp2/LibYAML_Wrapper.hpp"
#include "runcpp2/LibYAML_Wrapper.hpp"
#include "runcpp2/ParseUtil.hpp"

#include "DSResult/DSResult.hpp"
#include "ssLogger/ssLog.hpp"

#include <string>

namespace runcpp2
{
namespace Data
{
    struct SourcePath
    {
        std::string Path;
        
        inline bool IsYAML_NodeParsableAsDefault(YAML::ConstNodePtr node) const
        {
            return node->IsScalar();
        }
        
        inline bool ParseYAML_Node(YAML::ConstNodePtr node)
        {
            if(!node->IsScalar())
            {
                ssLOG_ERROR("Failed to parse SourcePath, unexpected type: " << 
                            YAML::NodeTypeToString(node->GetType()));
                return false;
            }
            Path = node->GetScalar<std::string>().DS_TRY_ACT(return false);
            return true;
        }
        
        inline std::string ToString(std::string indentation) const
        {
            std::string out;
            out += indentation + GetEscapedYAMLString(Path) + "\n";
            return out;
        }
        
        inline bool Equals(const SourcePath& other) const
        {
            return Path == other.Path;
        }
    };
}
}

#endif
