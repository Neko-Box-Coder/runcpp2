#ifndef RUNCPP2_DATA_DEFINES_HPP
#define RUNCPP2_DATA_DEFINES_HPP

#include "runcpp2/Data/ParseCommon.hpp"
#include "runcpp2/LibYAML_Wrapper.hpp"
#include "runcpp2/ParseUtil.hpp"

#include "ssLogger/ssLog.hpp"
#include "DSResult/DSResult.hpp"

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

    struct Defines
    {
        std::vector<Define> CurDefines;

        inline bool ParseYAML_Node(YAML::ConstNodePtr node)
        {
            ssLOG_FUNC_DEBUG();
            
            if(!node->IsSequence())
            {
                ssLOG_ERROR("Defines: Paths type requires a list");
                return false;
            }
            
            for(int i = 0; i < node->GetChildrenCount(); ++i)
            {
                std::string defineStr = node->GetSequenceChildScalar<std::string>(i)
                                            .DS_TRY_ACT(ssLOG_ERROR(DS_TMP_ERROR.ToString()); 
                                                        return false);
                Define define = {};
                define.Init(defineStr);
                CurDefines.push_back(define);
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
            std::string out;
            
            if(CurDefines.empty())
                return out;
            
            if(CurDefines.empty())
            {
                out += indentation + "[]\n";
                return out;
            }
            
            for(int i = 0; i < CurDefines.size(); ++i)
            {
                if(CurDefines.at(i).Value.empty())
                    out += indentation + "-   " + GetEscapedYAMLString(CurDefines.at(i).Name) + "\n";
                else
                {
                    out +=  indentation + 
                            "-   " + 
                            GetEscapedYAMLString(   CurDefines.at(i).Name + 
                                                    "=" + 
                                                    CurDefines.at(i).Value) + 
                            "\n";
                }
            }
            
            return out;
        }

        inline bool Equals(const Defines& other) const
        {
            if(CurDefines.size() != other.CurDefines.size())
                return false;
            
            for(int i = 0; i < CurDefines.size(); ++i)
            {
                const Define& define = CurDefines.at(i);
                const Define& otherDefine = other.CurDefines.at(i);
                
                if( define.Name != otherDefine.Name || 
                    define.Value != otherDefine.Value ||
                    define.HasValue != otherDefine.HasValue)
                {
                    return false;
                }
            }
            
            return true;
        }
    };
}
}

#endif
