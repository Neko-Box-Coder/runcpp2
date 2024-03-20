#include "runcpp2/Data/DependencySetup.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "runcpp2/Data/ParseCommon.hpp"
#include "ssLogger/ssLog.hpp"

bool runcpp2::Data::DependencySetup::ParseYAML_Node(ryml::ConstNodeRef& node)
{
    INTERNAL_RUNCPP2_SAFE_START();
    
    if(!node.is_map())
    {
        ssLOG_ERROR("DependencySetup: Node is not a Map");
        return false;
    }
    
    for(int i = 0; i < node.num_children(); i++)
    {
        if(!node[i].is_seq())
        {
            ssLOG_ERROR("DependencySetup: Node is not a sequence");
            return false;
        }
        
        ProfileName profile = GetKey(node[i]);
        for(int j = 0; j < node[i].num_children(); j++)
            SetupSteps[profile].push_back(GetValue(node[i][j]));
    }
    
    return true;
    
    INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
}

std::string runcpp2::Data::DependencySetup::ToString(std::string indentation) const
{
    std::string out;
    for(auto it = SetupSteps.begin(); it != SetupSteps.end(); it++)
    {
        out += indentation + it->first + ":\n";
        for(int i = 0; i < it->second.size(); i++)
            out += indentation + "-   " + it->second[i] + "\n";
    }
    
    return out;
}