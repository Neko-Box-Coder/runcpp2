#include "runcpp2/Data/FilesToCopyInfo.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "ssLogger/ssLog.hpp"

bool runcpp2::Data::FilesToCopyInfo::ParseYAML_Node(const ryml::ConstNodeRef& node)
{
    INTERNAL_RUNCPP2_SAFE_START();
    
    if(!node.is_map())
    {
        ssLOG_ERROR("FilesToCopyInfo: Node is not a Map");
        return false;
    }
    
    for(int i = 0; i < node.num_children(); i++)
    {
        if(!node[i].is_seq())
        {
            ssLOG_ERROR("FilesToCopyInfo: Node is not a sequence");
            return false;
        }
        
        ProfileName profile = GetKey(node[i]);
        for(int j = 0; j < node[i].num_children(); j++)
            ProfileFiles[profile].push_back(GetValue(node[i][j]));
    }
    
    return true;
    
    INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
}

std::string runcpp2::Data::FilesToCopyInfo::ToString(std::string indentation) const
{
    std::string out;
    for(auto it = ProfileFiles.begin(); it != ProfileFiles.end(); it++)
    {
        out += indentation + it->first + ":\n";
        for(int i = 0; i < it->second.size(); i++)
            out += indentation + "-   " + it->second[i] + "\n";
    }
    
    return out;
}

bool runcpp2::Data::FilesToCopyInfo::Equals(const FilesToCopyInfo& other) const
{
    if(ProfileFiles.size() != other.ProfileFiles.size())
        return false;

    for(const auto& it : ProfileFiles)
    {
        if(other.ProfileFiles.count(it.first) == 0 || other.ProfileFiles.at(it.first) != it.second)
            return false;
    }
    
    return true;
}
