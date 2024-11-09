#include "runcpp2/Data/ProfilesCommands.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "runcpp2/Data/ParseCommon.hpp"
#include "ssLogger/ssLog.hpp"

bool runcpp2::Data::ProfilesCommands::ParseYAML_Node(ryml::ConstNodeRef& node)
{
    INTERNAL_RUNCPP2_SAFE_START();
    
    if(!node.is_map())
    {
        ssLOG_ERROR("ProfilesCommands: Node is not a Map");
        return false;
    }
    
    for(int i = 0; i < node.num_children(); i++)
    {
        if(!node[i].is_seq())
        {
            ssLOG_ERROR("ProfilesCommands: Node is not a sequence");
            return false;
        }
        
        ProfileName profile = GetKey(node[i]);
        for(int j = 0; j < node[i].num_children(); j++)
            CommandSteps[profile].push_back(GetValue(node[i][j]));
    }
    
    return true;
    
    INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
}

std::string runcpp2::Data::ProfilesCommands::ToString(std::string indentation) const
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

bool runcpp2::Data::ProfilesCommands::Equals(const ProfilesCommands& other) const
{
    if(CommandSteps.size() != other.CommandSteps.size())
        return false;
    
    for(const auto& it : CommandSteps)
    {
        if(other.CommandSteps.count(it.first) == 0 || other.CommandSteps.at(it.first) != it.second)
            return false;
    }
    
    return true;
}
