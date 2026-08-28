#ifndef RUNCPP2_DATA_DEPENDENCY_COMPILE_PROPERTY_HPP
#define RUNCPP2_DATA_DEPENDENCY_COMPILE_PROPERTY_HPP

#include "runcpp2/Data/ParseCommon.hpp"
#include "runcpp2/Data/ProfilesDefines.hpp"

#include "runcpp2/LibYAML_Wrapper.hpp"
#include "runcpp2/ParseUtil.hpp"

#include "DSResult/DSResult.hpp"
#include "ssLogger/ssLog.hpp"

#include <vector>
#include <unordered_map>
#include <string>
#include <utility>


namespace runcpp2
{
namespace Data
{
    struct ProfileCompileProperty
    {
        std::vector<Define> Defines;
        std::vector<std::string> AdditionalCompileOptions;
    };

    struct DependencyCompileProperty
    {
        std::unordered_map<ProfileName, ProfileCompileProperty> ProfileProperties;
        
        inline bool ParseYAML_Node(YAML::ConstNodePtr node)
        {
            ssLOG_FUNC_DEBUG();
            
            if(!node->IsMap())
            {
                ssLOG_ERROR("DependencyCompileProperty: Node is not a Map");
                return false;
            }
            
            for(int i = 0; i < node->GetChildrenCount(); ++i)
            {
                ProfileName profile = node  ->GetMapKeyScalarAt<ProfileName>(i)
                                            .DS_TRY_ACT(return false);
                YAML::ConstNodePtr profileNode = node->GetMapValueNodeAt(i);
                if(!ParseYAML_NodeWithProfile(profileNode, profile))
                    return false;
            }

            return true;
        }

        inline bool ParseYAML_NodeWithProfile(YAML::ConstNodePtr node, ProfileName profile)
        {
            ssLOG_FUNC_DEBUG();
            
            ProfileCompileProperty& property = ProfileProperties[profile];
            
            std::vector<NodeRequirement> requirements =
            {
                NodeRequirement("Defines", YAML::NodeType::Sequence, false, true),
                NodeRequirement("AdditionalCompileOptions", YAML::NodeType::Sequence, false, true)
            };
            
            if(!CheckNodeRequirements(node, requirements))
            {
                ssLOG_ERROR("DependencyCompileProperty: Failed to meet requirements for profile " << 
                            profile);
                return false;
            }
            
            if(ExistAndHasChild(node, "Defines"))
            {
                for(int j = 0; 
                    j < node->GetMapValueNode("Defines")->GetChildrenCount();
                    ++j)
                {
                    std::string defineStr = node->GetMapValueNode("Defines")
                                                ->GetSequenceChildScalar<std::string>(j)
                                                .DS_TRY_ACT(return false);
                    Define d = {};
                    d.Init(defineStr);
                    property.Defines.push_back(d);
                }
            }
            
            if(ExistAndHasChild(node, "AdditionalCompileOptions"))
            {
                for(int j = 0; 
                    j < node->GetMapValueNode("AdditionalCompileOptions")->GetChildrenCount();
                    ++j)
                {
                    std::string compileOption = node->GetMapValueNode("AdditionalCompileOptions")
                                                    ->GetSequenceChildScalar<std::string>(j)
                                                    .DS_TRY_ACT(return false);
                    property.AdditionalCompileOptions.push_back(compileOption);
                }
            }
            
            return true;
        }

        inline bool IsYAML_NodeParsableAsDefault(YAML::ConstNodePtr node) const
        {
            ssLOG_FUNC_DEBUG();
    
            if(!node->IsMap())
            {
                ssLOG_ERROR("DependencyCompileProperty type requires a map");
                return false;
            }

            if( ExistAndHasChild(node, "Defines") ||
                ExistAndHasChild(node, "AdditionalCompileOptions"))
            {
                std::vector<NodeRequirement> requirements =
                {
                    NodeRequirement("Defines", YAML::NodeType::Sequence, false, true),
                    NodeRequirement("AdditionalCompileOptions", YAML::NodeType::Sequence, false, true)
                };
                
                return CheckNodeRequirements(node, requirements);
            }
            
            return false;
        }

        inline std::string ToString(std::string indentation) const
        {
            std::string out;
            for(const std::pair<const ProfileName, ProfileCompileProperty>& profilePair : 
                ProfileProperties)
            {
                out += indentation + profilePair.first + ":\n";
                const ProfileCompileProperty& property = profilePair.second;
                
                if(!property.Defines.empty())
                {
                    out += indentation + "    Defines:\n";
                    for(const Define& define : property.Defines)
                    {
                        out += indentation + "    -   ";
                        if(define.Value.empty())
                            out += GetEscapedYAMLString(define.Name) + "\n";
                        else
                            out += GetEscapedYAMLString(define.Name + "=" + define.Value) + "\n";
                    }
                }
                
                if(!property.AdditionalCompileOptions.empty())
                {
                    out += indentation + "    AdditionalCompileOptions:\n";
                    for(const std::string& option : property.AdditionalCompileOptions)
                        out += indentation + "    -   " + GetEscapedYAMLString(option) + "\n";
                }
            }
            
            return out;
        }

        inline bool Equals(const DependencyCompileProperty& other) const
        {
            if(ProfileProperties.size() != other.ProfileProperties.size())
                return false;

            for(const auto& it : ProfileProperties)
            {
                if(other.ProfileProperties.count(it.first) == 0)
                    return false;
                    
                const ProfileCompileProperty& otherProperty = other.ProfileProperties.at(it.first);
                const ProfileCompileProperty& property = it.second;
                if( property.Defines.size() != otherProperty.Defines.size() ||
                    property.AdditionalCompileOptions.size() != 
                    property.AdditionalCompileOptions.size())
                {
                    return false;
                }
                
                for(size_t i = 0; i < property.Defines.size(); ++i)
                {
                    const Define& define = property.Defines[i];
                    const Define& otherDefine = otherProperty.Defines[i];
                    
                    if( define.Name != otherDefine.Name || 
                        define.Value != otherDefine.Value ||
                        define.HasValue != otherDefine.HasValue)
                    {
                        return false;
                    }
                }
                
                if(property.AdditionalCompileOptions != otherProperty.AdditionalCompileOptions)
                    return false;
            }
            
            return true;
        }
    };
}
}

#endif
