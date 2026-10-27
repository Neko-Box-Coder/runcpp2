#ifndef RUNCPP2_DATA_PLATFORM_PROFILE_CONTAINER_HPP
#define RUNCPP2_DATA_PLATFORM_PROFILE_CONTAINER_HPP

#include "runcpp2/Data/ParseCommon.hpp"
#include "runcpp2/LibYAML_Wrapper.hpp"

#include "ssLogger/ssLog.hpp"
#include "DSResult/DSResult.hpp"

#include <unordered_map>
#include <string>
#include <utility>

namespace runcpp2
{
namespace Data
{
    template<typename T>
    struct PlatformProfileContainer
    {
        std::unordered_map<PlatformName, std::unordered_map<ProfileName, T>> Data;
        
        inline bool ParseYAML_Node(YAML::ConstNodePtr node)
        {
            ssLOG_FUNC_DEBUG();
            
            T defaultValue;
            if(defaultValue.IsYAML_NodeParsableAsDefault(node)) //If we skip platform profile
            {
                if(defaultValue.ParseYAML_Node(node))
                    Data["DefaultPlatform"]["DefaultProfile"] = defaultValue;
                else
                    return false;
            }
            else
            {
                if(!node->IsMap())
                {
                    ssLOG_ERROR("Platform profile map expected on line " + DS_STR(node->LineNumber));
                    return false;
                }
                
                for(int i = 0; i < node->GetChildrenCount(); ++i)
                {
                    PlatformName platform = node->GetMapKeyScalarAt<std::string>(i)
                                                .DS_TRY_ACT(ssLOG_ERROR(DS_TMP_ERROR.ToString()); 
                                                            return false);
                    
                    YAML::ConstNodePtr currentPlatformNode = node->GetMapValueNodeAt(i);
                    if(!currentPlatformNode->IsMap())
                    {
                        ssLOG_ERROR("Profile map expected on line " + 
                                    DS_STR(currentPlatformNode->LineNumber));
                        return false;
                    }
                    
                    for(int j = 0; j < currentPlatformNode->GetChildrenCount(); ++j)
                    {
                        ProfileName profile = 
                            currentPlatformNode ->GetMapKeyScalarAt<ProfileName>(j)
                                                .DS_TRY_ACT(return false);
                        T value;
                        if(!value.ParseYAML_Node(currentPlatformNode->GetMapValueNodeAt(j)))
                            return false;
                        Data[platform][profile] = value;
                    }
                }
            }
            
            return true;
        }

        inline std::string ToString(std::string indentation) const
        {
            std::string out;
            
            if(Data.empty())
                return out;
            
            for(auto platformIt = Data.begin(); platformIt != Data.end(); ++platformIt)
            {
                out += indentation + platformIt->first + ":\n";
                for(auto profileIt = platformIt->second.begin(); 
                    profileIt != platformIt->second.end(); 
                    ++profileIt)
                {
                    out += indentation + "    " + profileIt->first + ":\n";
                    out += profileIt->second.ToString(indentation + "        ");
                }
            }
            
            return out;
        }

        inline bool PlatformEquals(const PlatformProfileContainer& other) const
        {
            std::vector<std::string> platformNames = runcpp2::GetPlatformNames();
            for(int i = 0; i < platformNames.size(); ++i)
            {
                if(Data.count(platformNames.at(i)) == 0 || other.Data.count(platformNames.at(i)) == 0)
                    continue;
                
                const std::string& platform = platformNames.at(i);
                for(auto it = Data.at(platform).begin(); 
                    it != Data.at(platform).end(); 
                    it++)
                {
                    if(other.Data.at(platform).count(it->first) == 0)
                        return false;
                
                    const T& otherT = other.Data.at(platform).at(it->first);
                    if(!otherT.Equals(it->second))
                        return false;
                }
            }
            
            return true;
        }

        inline bool Equals(const PlatformProfileContainer& other) const
        {
            if(Data.size() != other.Data.size())
                return false;

            for(const auto& platformIt : Data)
            {
                if(other.Data.count(platformIt.first) == 0)
                    return false;
                
                for(const auto& profileIt : platformIt.second)
                {
                    if(other.Data.at(platformIt.first).count(profileIt.first) == 0)
                        return false;
                
                    const T& otherT = other.Data.at(platformIt.first).at(profileIt.first);
                    if(!otherT.Equals(profileIt.second))
                        return false;
                }
            }
            
            return true;
        }
        
        inline const T* GetValueFromProfile(const runcpp2::Data::Profile& profile) const
        {
            std::vector<std::string> platformNames = runcpp2::GetPlatformNames();
            for(int i = 0; i < platformNames.size(); ++i)
            {
                if(Data.count(platformNames.at(i)) > 0)
                {
                    const std::unordered_map<ProfileName, T>& profileMap = Data.at(platformNames.at(i));
                    std::vector<std::string> profileNames;
                    profile.GetNames(profileNames);
                    
                    for(int i = 0; i < profileNames.size(); ++i)
                    {
                        if(profileMap.count(profileNames.at(i)))
                            return &profileMap.at(profileNames.at(i));
                    }
                }
            }
            
            return NULL;
        }
    };
}
}

#endif
