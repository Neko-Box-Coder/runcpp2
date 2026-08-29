#ifndef RUNCPP2_DATA_PROFILES_DEFINES_HPP
#define RUNCPP2_DATA_PROFILES_DEFINES_HPP

#include "runcpp2/Data/ParseCommon.hpp"
#include "runcpp2/LibYAML_Wrapper.hpp"
#include "runcpp2/ParseUtil.hpp"

#include "ssLogger/ssLog.hpp"
#include "DSResult/DSResult.hpp"

#include <unordered_map>
#include <vector>
#include <string>
#include <stddef.h>
#include <utility>

namespace runcpp2
{
namespace Data
{
    struct Define
    {
        std::string Name;
        std::string Value;
        bool HasValue;
        
        inline void Init(const std::string& str)
        {
            size_t equalPos = str.find('=');
            if(equalPos != std::string::npos)
            {
                Name = str.substr(0, equalPos);
                Value = str.substr(equalPos + 1);
                HasValue = true;
            }
            else
            {
                Name = str;
                Value = "";
                HasValue = false;
            }
        }
    };

    struct ProfilesDefines
    {
        std::unordered_map<ProfileName, std::vector<Define>> Defines;

        inline bool ParseYAML_Node(YAML::ConstNodePtr node)
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

        inline bool ParseYAML_NodeWithProfile(YAML::ConstNodePtr node, ProfileName profile)
        {
            ssLOG_FUNC_DEBUG();
            
            if(!node->IsSequence())
            {
                ssLOG_ERROR("ProfilesDefines: Paths type requires a list");
                return false;
            }
            
            for(int i = 0; i < node->GetChildrenCount(); ++i)
            {
                std::string defineStr = node->GetSequenceChildScalar<std::string>(i)
                                            .DS_TRY_ACT(ssLOG_ERROR(DS_TMP_ERROR.ToString()); 
                                                        return false);
                Define define = {};
                define.Init(defineStr);
                Defines[profile].push_back(define);
            }
            
            return true;
        }

        inline bool IsYAML_NodeParsableAsDefault(YAML::ConstNodePtr node) const
        {
            ssLOG_FUNC_DEBUG();
            return node->IsSequence();
        }

        inline std::string ToString(std::string indentation) const
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

        inline bool Equals(const ProfilesDefines& other) const
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
    };
}
}

#endif
