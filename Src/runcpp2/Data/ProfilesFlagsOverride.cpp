#include "runcpp2/Data/ProfilesFlagsOverride.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "ssLogger/ssLog.hpp"

bool runcpp2::Data::ProfilesFlagsOverride::ParseYAML_Node(ryml::ConstNodeRef& node)
{
    INTERNAL_RUNCPP2_SAFE_START();
    
    if(!node.is_map())
    {
        ssLOG_ERROR("ProfilesFlagsOverrides: Not a map type");
        return false;
    }
    
    for(int i = 0; i < node.num_children(); ++i)
    {
        if(!INTERNAL_RUNCPP2_BIT_CONTANTS(node[i].type().type, ryml::NodeType_e::MAP))
        {
            ssLOG_ERROR("ProfilesFlagsOverrides: FlagsOverrideInfo type requires a map");
            return false;
        }
        
        ProfileName profile = GetKey(node[i]);
        FlagsOverrideInfo flags;
        
        ryml::ConstNodeRef flagsOverrideNode = node[i];
        
        if(!flags.ParseYAML_Node(flagsOverrideNode))
        {
            ssLOG_ERROR("ProfilesFlagsOverrides: Unable to parse FlagsOverride.");
            return false;
        }
        
        FlagsOverrides[profile] = flags;
    }
    
    return true;
    
    INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
}

std::string runcpp2::Data::ProfilesFlagsOverride::ToString(std::string indentation) const
{
    std::string out;
    
    for(auto it = FlagsOverrides.begin(); it != FlagsOverrides.end(); ++it)
        out += indentation + it->first + ":\n" + it->second.ToString(indentation + "    ");
    
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
