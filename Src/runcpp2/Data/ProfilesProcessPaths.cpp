#include "runcpp2/Data/ProfilesProcessPaths.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "ssLogger/ssLog.hpp"

bool runcpp2::Data::ProfilesProcessPaths::ParseYAML_Node(ryml::ConstNodeRef& node)
{
    ssLOG_FUNC_DEBUG();

    INTERNAL_RUNCPP2_SAFE_START();
    
    if(!node.is_map())
    {
        ssLOG_ERROR("ProfilesProcessPaths: Not a map type");
        return false;
    }
    
    for(int i = 0; i < node.num_children(); ++i)
    {
        if(!INTERNAL_RUNCPP2_BIT_CONTANTS(node[i].type().type, ryml::NodeType_e::SEQ))
        {
            ssLOG_ERROR("ProfilesProcessPaths: Paths type requires a list");
            return false;
        }
        
        ryml::ConstNodeRef currentProfilePathsNode = node[i];
        ProfileName profile = GetKey(currentProfilePathsNode);

        for(int j = 0; j <  currentProfilePathsNode.num_children(); ++j)
            Paths[profile].push_back(GetValue(currentProfilePathsNode[j]));
    }
    
    return true;
    
    INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
}

std::string runcpp2::Data::ProfilesProcessPaths::ToString(std::string indentation) const
{
    std::string out;
    
    if(Paths.empty())
        return out;
        
    for(auto it = Paths.begin(); it != Paths.end(); ++it)
    {
        if(it->second.empty())
            out += indentation + it->first + ": []\n";
        else
        {
            out += indentation + it->first + ":\n";
            for(int i = 0; i < it->second.size(); ++i)
                out += indentation + "-   " + GetEscapedYAMLString(it->second.at(i).string()) + "\n";
        }
    }
    
    return out;
}

bool runcpp2::Data::ProfilesProcessPaths::Equals(const ProfilesProcessPaths& other) const
{
    if(Paths.size() != other.Paths.size())
        return false;
    
    for(const auto& it : Paths)
    {
        if(other.Paths.count(it.first) == 0 || other.Paths.at(it.first) != it.second)
            return false;
    }
    
    return true;
}
