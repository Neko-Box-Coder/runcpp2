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
    #if 0
    template<typename T>
    bool ParsePlatformProfileMap(   YAML::ConstNodePtr node, 
                                    const std::string& key,
                                    std::unordered_map<PlatformName, T>& result,
                                    const std::string& errorMessage)
    {
        if(ExistAndHasChild(node, key))
        {
            YAML::ConstNodePtr mapNode = node->GetMapValueNode(key);
            
            //If we skip platform profile
            T defaultValue;
            if(defaultValue.IsYAML_NodeParsableAsDefault(mapNode))
            {
                if(defaultValue.ParseYAML_NodeWithProfile(mapNode, "DefaultProfile"))
                    result["DefaultPlatform"] = defaultValue;
                else
                    return false;
            }
            else
            {
                if(!CheckNodeRequirement(node, key, YAML::NodeType::Map, false, true))
                    return false;
                
                for(int i = 0; i < mapNode->GetChildrenCount(); ++i)
                {
                    PlatformName platform = 
                        mapNode ->GetMapKeyScalarAt<std::string>(i)
                                .DS_TRY_ACT(ssLOG_ERROR(DS_TMP_ERROR.ToString()); return false);
                    T value;
                    YAML::ConstNodePtr currentNode = mapNode->GetMapValueNodeAt(i);
                    if(!value.ParseYAML_Node(currentNode))
                    {
                        ssLOG_ERROR("ScriptInfo: Failed to parse " << errorMessage);
                        return false;
                    }
                    result[platform] = value;
                }
            }
        }
        return true;
    }
    #endif
    
    template<typename T>
    struct PlatformProfileContainer
    {
        std::unordered_map<PlatformName, std::unordered_map<ProfileName, T>> Data;
        //std::unordered_map<ProfileName, FlagsOverrideInfo> FlagsOverrides;
        
        #if 0
        inline bool IsYAML_NodeParsableAsDefault(YAML::ConstNodePtr node) const
        {
            ssLOG_FUNC_DEBUG();
            T t;
            return t.IsYAML_NodeParsableAsDefault(node);
        }
        #endif
        
        inline bool ParseYAML_NodeWithProfile(YAML::ConstNodePtr node, ProfileName profile)
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
                    PlatformName platform = 
                        node ->GetMapKeyScalarAt<std::string>(i)
                                .DS_TRY_ACT(ssLOG_ERROR(DS_TMP_ERROR.ToString()); return false);
                    
                    YAML::ConstNodePtr currentPlatformNode = node->GetMapValueNodeAt(i);
                    if(!currentPlatformNode->IsMap())
                    {
                        ssLOG_ERROR("Profile map expected on line " + 
                                    DS_STR(currentPlatformNode->LineNumber));
                        return false;
                    }
                    
                    for(int j = 0; j < currentPlatformNode->GetChildrenCount(); ++j)
                    {
                        ProfileName profile = node  ->GetMapKeyScalarAt<ProfileName>(j)
                                                    .DS_TRY_ACT(return false);
                        T value;
                        if(!value.ParseYAML_Node(node->GetMapValueNodeAt(j)))
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

        inline bool Equals(const PlatformProfileContainer& other) const
        {
            if(Data.size() != other.Data.size())
                return false;

            for(const auto& platformIt : Data)
            {
                if(other.Data.count(platformIt.first) == 0)
                    return false;
                
                for(const auto& profileIt : platformIt)
                {
                    if(!other.Data.at(platformIt.first).count(profileIt.first) == 0)
                        return false;
                
                    const T& otherT = other.Data.at(platformIt.first).at(profileIt.first);
                    if(!otherT.Equals(profileIt.second))
                        return false;
                }
            }
            
            return true;
        }
    };
}
}

#endif
