#include "runcpp2/Data/ProfilesFlagsOverride.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "ssLogger/ssLog.hpp"

bool runcpp2::Data::ProfilesFlagsOverride::ParseYAML_Node(ryml::ConstNodeRef node)
{
    ssLOG_FUNC_DEBUG();
    INTERNAL_RUNCPP2_SAFE_START();
    
    if(!node.is_map())
    {
        ssLOG_ERROR("ProfilesFlagsOverrides: Not a map type");
        return false;
    }
    
    for(int i = 0; i < node.num_children(); ++i)
    {
        ProfileName profile = GetKey(node[i]);
        if(!ParseYAML_NodeWithProfile(node[i], profile))
            return false;
    }
    
    return true;
    
    INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
}

bool runcpp2::Data::ProfilesFlagsOverride::ParseYAML_Node(YAML::ConstNodePtr node)
{
    ssLOG_FUNC_DEBUG();
    
    if(!node->IsMap())
    {
        ssLOG_ERROR("ProfilesFlagsOverrides: Not a map type");
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

bool runcpp2::Data::ProfilesFlagsOverride::ParseYAML_NodeWithProfile(   ryml::ConstNodeRef node, 
                                                                        ProfileName profile)
{
    ssLOG_FUNC_DEBUG();
    INTERNAL_RUNCPP2_SAFE_START();
    
    if(!INTERNAL_RUNCPP2_BIT_CONTANTS(node.type().type, ryml::NodeType_e::MAP))
    {
        ssLOG_ERROR("ProfilesFlagsOverrides: FlagsOverrideInfo type requires a map");
        return false;
    }
    
    FlagsOverrideInfo flags;
    ryml::ConstNodeRef flagsOverrideNode = node;
    
    if(!flags.ParseYAML_Node(flagsOverrideNode))
    {
        ssLOG_ERROR("ProfilesFlagsOverrides: Unable to parse FlagsOverride.");
        return false;
    }
    
    FlagsOverrides[profile] = flags;
    
    return true;
    INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
}

bool runcpp2::Data::ProfilesFlagsOverride::IsYAML_NodeParsableAsDefault(ryml::ConstNodeRef node) const
{
    ssLOG_FUNC_DEBUG();
    INTERNAL_RUNCPP2_SAFE_START();

    FlagsOverrideInfo flags;
    return flags.IsYAML_NodeParsableAsDefault(node);
    
    INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
}

bool 
runcpp2::Data::ProfilesFlagsOverride::ParseYAML_NodeWithProfile_LibYaml(YAML::ConstNodePtr node, 
                                                                        ProfileName profile)
{
    ssLOG_FUNC_DEBUG();
    
    if(!node->IsMap())
    {
        ssLOG_ERROR("ProfilesFlagsOverrides: FlagsOverrideInfo type requires a map");
        return false;
    }
    
    FlagsOverrideInfo flags;
    YAML::ConstNodePtr flagsOverrideNode = node;
    
    if(!flags.ParseYAML_Node(flagsOverrideNode))
    {
        ssLOG_ERROR("ProfilesFlagsOverrides: Unable to parse FlagsOverride.");
        return false;
    }
    
    FlagsOverrides[profile] = flags;
    
    return true;
}

bool 
runcpp2::Data::ProfilesFlagsOverride::IsYAML_NodeParsableAsDefault_LibYaml(YAML::ConstNodePtr node) const
{
    ssLOG_FUNC_DEBUG();
    FlagsOverrideInfo flags;
    return flags.IsYAML_NodeParsableAsDefault_LibYaml(node);
}

std::string runcpp2::Data::ProfilesFlagsOverride::ToString(std::string indentation) const
{
    std::string out;
    
    if(FlagsOverrides.empty())
        return out;
    
    for(auto it = FlagsOverrides.begin(); it != FlagsOverrides.end(); ++it)
    {
        out += indentation + it->first + ":\n";
        out += it->second.ToString(indentation + "    ");
    }
    
    return out;
}

bool runcpp2::Data::ProfilesFlagsOverride::Equals(const ProfilesFlagsOverride& other) const
{
    if(FlagsOverrides.size() != other.FlagsOverrides.size())
        return false;

    for(const auto& it : FlagsOverrides)
    {
        if( other.FlagsOverrides.count(it.first) == 0 || 
            !other.FlagsOverrides.at(it.first).Equals(it.second))
        {
            return false;
        }
    }
    
    return true;
}
