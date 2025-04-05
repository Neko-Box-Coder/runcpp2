#include "runcpp2/Data/FilesToCopyInfo.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "ssLogger/ssLog.hpp"

bool runcpp2::Data::FilesToCopyInfo::ParseYAML_Node(ryml::ConstNodeRef node)
{
    ssLOG_FUNC_DEBUG();
    INTERNAL_RUNCPP2_SAFE_START();
    
    if(!node.is_map())
    {
        ssLOG_ERROR("FilesToCopyInfo: Node is not a Map");
        return false;
    }
    
    //If we skip platform profile
    if(IsYAML_NodeParsableAsDefault(node))
    {
        if(!ParseYAML_NodeWithProfile(node, "DefaultProfile"))
            return false;
    }
    else
    {
        for(int i = 0; i < node.num_children(); i++)
        {
            ProfileName profile = GetKey(node[i]);
            if(!ParseYAML_NodeWithProfile(node[i], profile))
                return false;
        }
    }
    
    return true;
    
    INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
}

bool runcpp2::Data::FilesToCopyInfo::ParseYAML_NodeWithProfile( ryml::ConstNodeRef node, 
                                                                ProfileName profile)
{
    ssLOG_FUNC_DEBUG();
    INTERNAL_RUNCPP2_SAFE_START();
    
    if(!INTERNAL_RUNCPP2_BIT_CONTANTS(node.type().type, ryml::NodeType_e::SEQ))
    {
        ssLOG_ERROR("FilesToCopyInfo: Node is not a sequence");
        return false;
    }
    
    for(int i = 0; i < node.num_children(); i++)
        ProfileFiles[profile].push_back(GetValue(node[i]));
    
    return true;
    
    INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
}

bool runcpp2::Data::FilesToCopyInfo::IsYAML_NodeParsableAsDefault(ryml::ConstNodeRef node) const
{
    ssLOG_FUNC_DEBUG();
    INTERNAL_RUNCPP2_SAFE_START();
    
    if(!INTERNAL_RUNCPP2_BIT_CONTANTS(node.type().type, ryml::NodeType_e::SEQ))
        return false;
    
    return true;
    INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
}

std::string runcpp2::Data::FilesToCopyInfo::ToString(std::string indentation) const
{
    std::string out;
    
    if(ProfileFiles.empty())
        return out;
        
    for(auto it = ProfileFiles.begin(); it != ProfileFiles.end(); it++)
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
