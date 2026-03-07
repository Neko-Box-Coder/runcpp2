#ifndef RUNCPP2_DATA_FILES_TO_COPY_INFO_HPP
#define RUNCPP2_DATA_FILES_TO_COPY_INFO_HPP

#include "runcpp2/Data/ParseCommon.hpp"
#include "runcpp2/YamlLib.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "ssLogger/ssLog.hpp"

#include <unordered_map>
#include <vector>
#include <string>

namespace runcpp2
{
namespace Data
{
    struct FilesToCopyInfo
    {
        std::unordered_map<ProfileName, std::vector<std::string>> ProfileFiles;
        
        inline bool ParseYAML_Node(YAML::ConstNodePtr node)
        {
            if(!node->IsMap())
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
                for(int i = 0; i < node->GetChildrenCount(); ++i)
                {
                    DS_UNWRAP_DECL_ACT( ProfileName profile, 
                                        node->GetMapKeyScalarAt<std::string>(i), 
                                        ssLOG_ERROR(DS_TMP_ERROR.ToString()); return false);
                    
                    if(!ParseYAML_NodeWithProfile(node->GetMapValueNodeAt(i), profile))
                        return false;
                }
            }
            
            return true;
        }

        inline bool ParseYAML_NodeWithProfile(YAML::ConstNodePtr node, ProfileName profile)
        {
            ssLOG_FUNC_DEBUG();
            if(!node->IsSequence())
            {
                ssLOG_ERROR("FilesToCopyInfo: Node is not a sequence");
                return false;
            }
            
            for(int i = 0; i < node->GetChildrenCount(); i++)
                ProfileFiles[profile].push_back(node->GetSequenceChildScalar<std::string>(i).value());
            
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

        inline bool Equals(const FilesToCopyInfo& other) const
        {
            if(ProfileFiles.size() != other.ProfileFiles.size())
                return false;

            for(const auto& it : ProfileFiles)
            {
                if( other.ProfileFiles.count(it.first) == 0 || 
                    other.ProfileFiles.at(it.first) != it.second)
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
