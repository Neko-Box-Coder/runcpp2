#include "runcpp2/Data/ProfilesDefines.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "ssLogger/ssLog.hpp"
#include <string>

bool runcpp2::Data::ProfilesDefines::ParseYAML_Node(YAML::ConstNodePtr node)
{
    ssLOG_FUNC_DEBUG();
    if(!node->IsMap())
    {
        ssLOG_ERROR("ProfilesDefines: Not a map type");
        return false;
    }
    
    for(int i = 0; i < node->GetChildrenCount(); ++i)
    {
        DS_UNWRAP_DECL_ACT( ProfileName profile, 
                            node->GetMapKeyScalarAt<std::string>(i), 
                            ssLOG_ERROR(DS_TMP_ERROR.ToString()); return false);
        if(!ParseYAML_NodeWithProfile(node->GetMapValueNodeAt(i), profile))
            return false;
    }
    
    return true;
}

bool runcpp2::Data::ProfilesDefines::ParseYAML_NodeWithProfile( YAML::ConstNodePtr node, 
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
        DS_UNWRAP_DECL_ACT( std::string defineStr, 
                            node->GetSequenceChildScalar<std::string>(i), 
                            ssLOG_ERROR(DS_TMP_ERROR.ToString()); return false);
        Define define;
        size_t equalPos = defineStr.find('=');
        if(equalPos != std::string::npos)
        {
            define.Name = defineStr.substr(0, equalPos);
            define.Value = defineStr.substr(equalPos + 1);
            define.HasValue = true;
        }
        else
        {
            define.Name = defineStr;
            define.Value = "";
            define.HasValue = false;
        }
        
        Defines[profile].push_back(define);
    }
    
    return true;
}

bool 
runcpp2::Data::ProfilesDefines::IsYAML_NodeParsableAsDefault(YAML::ConstNodePtr node) const
{
    ssLOG_FUNC_DEBUG();
    return node->IsSequence();
}

std::string runcpp2::Data::ProfilesDefines::ToString(std::string indentation) const
{
    std::string result;
    
    if(Defines.empty())
        return result;
    
    for(auto it = Defines.begin(); it != Defines.end(); ++it)
    {
        if(it->second.empty())
            result += indentation + it->first + ": []\n";
        else
        {
            result += indentation + it->first + ":\n";
            for(int j = 0; j < it->second.size(); ++j)
            {
                const Define& define = it->second.at(j);
                result += indentation + "-   ";
                if(define.Value.empty())
                    result += GetEscapedYAMLString(define.Name) + "\n";
                else
                    result += GetEscapedYAMLString(define.Name + "=" + define.Value) + "\n";
            }
        }
    }
    
    return result;
}

bool runcpp2::Data::ProfilesDefines::Equals(const ProfilesDefines& other) const
{
    if(Defines.size() != other.Defines.size())
        return false;
    
    for(const auto& it : Defines)
    {
        if(other.Defines.count(it.first) == 0)
            return false;
            
        const std::vector<Define>& otherDefines = other.Defines.at(it.first);
        if(it.second.size() != otherDefines.size())
            return false;
            
        for(size_t i = 0; i < it.second.size(); ++i)
        {
            const Define& define = it.second[i];
            const Define& otherDefine = otherDefines[i];
            
            if( define.Name != otherDefine.Name || 
                define.Value != otherDefine.Value ||
                define.HasValue != otherDefine.HasValue)
            {
                return false;
            }
        }
    }
    
    return true;
}
