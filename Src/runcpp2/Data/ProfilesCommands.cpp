#include "runcpp2/Data/ProfilesCommands.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "runcpp2/Data/ParseCommon.hpp"
#include "ssLogger/ssLog.hpp"

bool runcpp2::Data::ProfilesCommands::ParseYAML_Node(ryml::ConstNodeRef node)
{
    ssLOG_FUNC_DEBUG();
    INTERNAL_RUNCPP2_SAFE_START();
    
    if(!node.is_map())
    {
        ssLOG_ERROR("ProfilesCommands: Node is not a Map");
        return false;
    }
    
    for(int i = 0; i < node.num_children(); i++)
    {
        ProfileName profile = GetKey(node[i]);
        if(!ParseYAML_NodeWithProfile(node[i], profile))
            return false;
    }
    
    return true;
    
    INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
}

bool runcpp2::Data::ProfilesCommands::ParseYAML_Node(YAML::ConstNodePtr node)
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
        if(!ParseYAML_NodeWithProfile_LibYaml(node->GetMapValueNodeAt(i), profile))
            return false;
    }
    
    return true;
}

bool runcpp2::Data::ProfilesCommands::ParseYAML_NodeWithProfile(ryml::ConstNodeRef node, 
                                                                ProfileName profile)
{
    ssLOG_FUNC_DEBUG();
    INTERNAL_RUNCPP2_SAFE_START();
    
    if(!INTERNAL_RUNCPP2_BIT_CONTANTS(node.type().type, ryml::NodeType_e::SEQ))
    {
        ssLOG_ERROR("ProfilesDefines: Paths type requires a list");
        return false;
    }
    
    for(int i = 0; i < node.num_children(); ++i)
        CommandSteps[profile].push_back(GetValue(node[i]));
    
    return true;
    INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
}

bool runcpp2::Data::ProfilesCommands::IsYAML_NodeParsableAsDefault(ryml::ConstNodeRef node) const
{
    ssLOG_FUNC_DEBUG();
    INTERNAL_RUNCPP2_SAFE_START();
    
    if(!INTERNAL_RUNCPP2_BIT_CONTANTS(node.type().type, ryml::NodeType_e::SEQ))
        return false;
    
    return true;
    INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
}

bool runcpp2::Data::ProfilesCommands::ParseYAML_NodeWithProfile_LibYaml(YAML::ConstNodePtr node, 
                                                                        ProfileName profile)
{
    ssLOG_FUNC_DEBUG();
    
    if(!node->IsSequence())
    {
        ssLOG_ERROR("ProfilesDefines: Paths type requires a list");
        return false;
    }
    
    for(int i = 0; i < node->GetChildrenCount(); ++i)
    {
        std::string command = node->GetSequenceChildScalar<std::string>(i).DS_TRY_ACT(return false);
        CommandSteps[profile].push_back(command);
    }
    
    return true;
}

bool 
runcpp2::Data::ProfilesCommands::IsYAML_NodeParsableAsDefault_LibYaml(YAML::ConstNodePtr node) const
{
    return node->IsSequence();
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
