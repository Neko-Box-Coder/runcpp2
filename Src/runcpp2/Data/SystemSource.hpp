#ifndef RUNCPP2_DATA_SYSTEM_SOURCE_HPP
#define RUNCPP2_DATA_SYSTEM_SOURCE_HPP

#include "runcpp2/LibYAML_Wrapper.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "runcpp2/Data/PlatformProfileContainer.hpp"
#include "runcpp2/Data/SourcePath.hpp"

#include "DSResult/DSResult.hpp"
#include "ssLogger/ssLog.hpp"

#include <vector>

namespace runcpp2
{
namespace Data
{
    struct SystemSource
    {
        PlatformProfileContainer<SourcePath> Name;
        
        inline bool ParseYAML_Node(YAML::ConstNodePtr node)
        {
            if(!ExistAndHasChild(node, "Name"))
            {
                ssLOG_ERROR("SystemSource must container \"Name\"");
                return false;
            }
            
            YAML::ConstNodePtr pathNode = node->GetMapValueNode("Name");
            if(!Name.ParseYAML_Node(pathNode))
                return false;
            
            return true;
        }

        inline std::string ToString(std::string indentation) const
        {
            std::string out;
            out += indentation + "System:\n";
            out += indentation + "    Name: \n" + Name.ToString(indentation + "        ");
            return out;
        }

        inline bool Equals(const SystemSource& other) const
        {
            return Name.Equals(other.Name);
        }
    };
}
}

#endif 
