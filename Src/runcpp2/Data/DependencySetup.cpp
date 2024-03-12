#include "runcpp2/Data/DependencySetup.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "runcpp2/Data/ParseCommon.hpp"
#include "ssLogger/ssLog.hpp"

bool runcpp2::DependencySetup::ParseYAML_Node(YAML::Node& node)
{
    INTERNAL_RUNCPP2_SAFE_START();
    
    if(!node.IsMap())
    {
        ssLOG_ERROR("DependencySetup: Node is not a Map");
        return false;
    }
    
    for(auto it = node.begin(); it != node.end(); it++)
    {
        ProfileName profile = it->first.as<std::string>();
        
        if(it->second.IsSequence())
        {
            for(int i = 0; i < it->second.size(); i++)
                SetupSteps[profile].push_back(it->second[i].as<std::string>());
        }
        else
        {
            //Can't convert to a sequence
            ssLOG_ERROR("DependencySetup: Failed to convert to sequence");
            return false;
        }
    }
    
    return true;
    
    INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
}

std::string runcpp2::DependencySetup::ToString(std::string indentation) const
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