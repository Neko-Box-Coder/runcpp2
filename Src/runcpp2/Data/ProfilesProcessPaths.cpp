#include "runcpp2/Data/ProfilesProcessPaths.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "ssLogger/ssLog.hpp"

bool runcpp2::Data::ProfilesProcessPaths::ParseYAML_Node(YAML::ConstNodePtr node)
{
    ssLOG_FUNC_DEBUG();
    
    if(!node->IsMap())
    {
        ssLOG_ERROR("ProfilesProcessPaths: Not a map type");
        return false;
    }
    
    for(int i = 0; i < node->GetChildrenCount(); ++i)
    {
        YAML::ConstNodePtr currentProfilePathsNode = node->GetMapValueNodeAt(i);
        ProfileName profile = node->GetMapKeyScalarAt<ProfileName>(i).DS_TRY_ACT(return false);
        if(!ParseYAML_NodeWithProfile(currentProfilePathsNode, profile))
            return false;
    }
    
    return true;
}

bool runcpp2::Data::ProfilesProcessPaths::ParseYAML_NodeWithProfile(YAML::ConstNodePtr node, 
                                                                    ProfileName profile)
{
    ssLOG_FUNC_DEBUG();
    
    if(!node->IsSequence())
    {
        ssLOG_ERROR("ProfilesProcessPaths: Paths type requires a list");
        return false;
    }
    
    for(int i = 0; i < node->GetChildrenCount(); ++i)
    {
        std::string path = node->GetSequenceChildScalar<std::string>(i).DS_TRY_ACT(return false);
        Paths[profile].push_back(path);
    }
    
    return true;
}

bool 
runcpp2::Data::ProfilesProcessPaths::IsYAML_NodeParsableAsDefault(YAML::ConstNodePtr node) const
{
    ssLOG_FUNC_DEBUG();
    return node->IsSequence();
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
