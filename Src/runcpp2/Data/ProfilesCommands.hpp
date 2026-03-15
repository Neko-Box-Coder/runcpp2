#ifndef RUNCPP2_DATA_PROFILES_COMMANDS_HPP
#define RUNCPP2_DATA_PROFILES_COMMANDS_HPP

#include "runcpp2/Data/ParseCommon.hpp"
#include "runcpp2/LibYAML_Wrapper.hpp"
#include "runcpp2/ParseUtil.hpp"

#include "ssLogger/ssLog.hpp"
#include "DSResult/DSResult.hpp"

#include <string>
#include <unordered_map>
#include <vector>
#include <utility>

namespace runcpp2
{
namespace Data
{
    struct ProfilesCommands
    {
        //TODO: Allow specifying command can fail
        std::unordered_map<ProfileName, std::vector<std::string>> CommandSteps;
        
        inline bool ParseYAML_Node(YAML::ConstNodePtr node)
        {
            ssLOG_FUNC_DEBUG();
            
            if(!node->IsMap())
            {
                ssLOG_ERROR("ProfilesCommands: Node is not a Map");
                return false;
            }
            
            for(int i = 0; i < node->GetChildrenCount(); ++i)
            {
                ProfileName profile = node->GetMapKeyScalarAt<ProfileName>(i).DS_TRY_ACT(return false);
                if(!ParseYAML_NodeWithProfile(node->GetMapValueNodeAt(i), profile))
                    return false;
            }
            
            return true;
        }

        inline bool ParseYAML_NodeWithProfile(YAML::ConstNodePtr node, ProfileName profile)
        {
            ssLOG_FUNC_DEBUG();
            
            if(!node->IsSequence())
            {
                ssLOG_ERROR("ProfilesDefines: Paths type requires a list");
                return false;
            }
            
            for(int i = 0; i < node->GetChildrenCount(); ++i)
            {
                std::string command = node  ->GetSequenceChildScalar<std::string>(i)
                                            .DS_TRY_ACT(return false);
                CommandSteps[profile].push_back(command);
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
            
            if(CommandSteps.empty())
                return out;
                
            for(auto it = CommandSteps.begin(); it != CommandSteps.end(); it++)
            {
                if(it->second.empty())
                    out += indentation + it->first + ": []\n";
                else
                {
                    out += indentation + it->first + ":\n";
                    for(int i = 0; i < it->second.size(); i++)
                        out += indentation + "-   " + GetEscapedYAMLString(it->second[i]) + "\n";
                }
            }
            
            return out;
        }

        inline bool Equals(const ProfilesCommands& other) const
        {
            if(CommandSteps.size() != other.CommandSteps.size())
                return false;
            
            for(const auto& it : CommandSteps)
            {
                if( other.CommandSteps.count(it.first) == 0 || 
                    other.CommandSteps.at(it.first) != it.second)
                {
                    return false;
                }
            }
            
            return true;
        }
    };
}
}

#endif
