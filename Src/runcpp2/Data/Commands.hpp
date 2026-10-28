#ifndef RUNCPP2_DATA_COMMANDS_HPP
#define RUNCPP2_DATA_COMMANDS_HPP

#include "runcpp2/Data/ParseCommon.hpp"
#include "runcpp2/LibYAML_Wrapper.hpp"
#include "runcpp2/ParseUtil.hpp"

#include "ssLogger/ssLog.hpp"
#include "DSResult/DSResult.hpp"

#include <string>
#include <vector>
#include <utility>

namespace runcpp2
{
namespace Data
{
    struct Commands
    {
        //TODO: Allow specifying command can fail
        std::vector<std::string> Steps;
        
        inline bool ParseYAML_Node(YAML::ConstNodePtr node)
        {
            ssLOG_FUNC_DEBUG();
            
            if(!node->IsSequence())
            {
                ssLOG_ERROR("Commands: Paths type requires a list");
                return false;
            }
            
            for(int i = 0; i < node->GetChildrenCount(); ++i)
            {
                std::string command = node  ->GetSequenceChildScalar<std::string>(i)
                                            .DS_TRY_ACT(return false);
                Steps.push_back(command);
            }
            
            return true;
        }

        inline bool IsYAML_NodeParsableAsDefault(YAML::ConstNodePtr node) const
        {
            return node->IsSequence();
        }

        inline std::string ToString(std::string indentation) const
        {
            std::string out;
            
            if(Steps.empty())
            {
                out += indentation + "[]\n";
                return out;
            }
            
            for(int i = 0; i < Steps.size(); ++i)
                out += indentation + "-   " + GetEscapedYAMLString(Steps.at(i)) + "\n";
            
            return out;
        }

        inline bool Equals(const Commands& other) const
        {
            if(Steps.size() != other.Steps.size())
                return false;
            
            for(int i = 0; i < Steps.size(); ++i)
            {
                if(Steps.at(i) != other.Steps.at(i))
                    return false;
            }
            
            return true;
        }
    };
}
}

#endif
