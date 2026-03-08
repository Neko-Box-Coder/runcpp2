#ifndef RUNCPP2_DATA_DEPENDENCY_LINK_PROPERTY_HPP
#define RUNCPP2_DATA_DEPENDENCY_LINK_PROPERTY_HPP

#include "runcpp2/Data/ParseCommon.hpp"

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
    struct ProfileLinkProperty
    {
        std::vector<std::string> SearchLibraryNames;
        std::vector<std::string> SearchDirectories;
        std::vector<std::string> ExcludeLibraryNames;
        std::vector<std::string> AdditionalLinkOptions;
    };

    struct DependencyLinkProperty
    {
        std::unordered_map<ProfileName, ProfileLinkProperty> ProfileProperties;
        
        inline bool ParseYAML_Node(YAML::ConstNodePtr node)
        {
            ssLOG_FUNC_DEBUG();
            
            if(!node->IsMap())
            {
                ssLOG_ERROR("DependencyLinkProperty: Node is not a Map");
                return false;
            }
            
            for(int i = 0; i < node->GetChildrenCount(); ++i)
            {
                ProfileName profile = node  ->GetMapKeyScalarAt<ProfileName>(i)
                                            .DS_TRY_ACT(return false);
                YAML::ConstNodePtr profileNode = node->GetMapValueNodeAt(i);
                
                //TODO: Maybe use `ParseYAML_NodeWithProfile()`?
                
                ProfileLinkProperty& property = ProfileProperties[profile];
                
                std::vector<NodeRequirement> requirements =
                {
                    NodeRequirement("SearchLibraryNames", YAML::NodeType::Sequence, true, false),
                    NodeRequirement("SearchDirectories", YAML::NodeType::Sequence, true, false),
                    NodeRequirement("ExcludeLibraryNames", YAML::NodeType::Sequence, false, true),
                    NodeRequirement("AdditionalLinkOptions", YAML::NodeType::Sequence, false, true)
                };
                
                if(!CheckNodeRequirements(profileNode, requirements))
                {
                    ssLOG_ERROR("DependencyLinkProperty: Failed to meet requirements for profile " << 
                                profile);
                    return false;
                }
                
                for(int j = 0; 
                    j < profileNode->GetMapValueNode("SearchLibraryNames")->GetChildrenCount(); 
                    ++j)
                {
                    std::string searchLibraryName = 
                        profileNode ->GetMapValueNode("SearchLibraryNames")
                                    ->GetSequenceChildScalar<std::string>(j)
                                    .DS_TRY_ACT(return false);
                    
                    property.SearchLibraryNames.push_back(searchLibraryName);
                }
                
                for(int j = 0; 
                    j < profileNode->GetMapValueNode("SearchDirectories")->GetChildrenCount(); 
                    ++j)
                {
                    std::string searchPath = 
                        profileNode ->GetMapValueNode("SearchDirectories")
                                    ->GetSequenceChildScalar<std::string>(j)
                                    .DS_TRY_ACT(return false);
                    property.SearchDirectories.push_back(searchPath);
                }

                if(ExistAndHasChild(profileNode, "ExcludeLibraryNames"))
                {
                    for(int j = 0; 
                        j < profileNode->GetMapValueNode("ExcludeLibraryNames")->GetChildrenCount(); 
                        ++j)
                    {
                        std::string excludeName =
                            profileNode ->GetMapValueNode("ExcludeLibraryNames")
                                        ->GetSequenceChildScalar<std::string>(j)
                                        .DS_TRY_ACT(return false);
                        property.ExcludeLibraryNames.push_back(excludeName);
                    }
                }

                if(ExistAndHasChild(profileNode, "AdditionalLinkOptions"))
                {
                    for(int j = 0; 
                        j < profileNode->GetMapValueNode("AdditionalLinkOptions")->GetChildrenCount();
                        ++j)
                    {
                        std::string linkOption =
                            profileNode ->GetMapValueNode("AdditionalLinkOptions")
                                        ->GetSequenceChildScalar<std::string>(j)
                                        .DS_TRY_ACT(return false);
                        property.AdditionalLinkOptions.push_back(linkOption);
                    }
                }
            }

            return true;
        }

        inline bool ParseYAML_NodeWithProfile(YAML::ConstNodePtr node, ProfileName profile)
        {
            ssLOG_FUNC_DEBUG();
            
            ProfileLinkProperty& property = ProfileProperties[profile];
            
            std::vector<NodeRequirement> requirements =
            {
                NodeRequirement("SearchLibraryNames", YAML::NodeType::Sequence, true, false),
                NodeRequirement("SearchDirectories", YAML::NodeType::Sequence, true, false),
                NodeRequirement("ExcludeLibraryNames", YAML::NodeType::Sequence, false, true),
                NodeRequirement("AdditionalLinkOptions", YAML::NodeType::Sequence, false, true)
            };
            
            if(!CheckNodeRequirements(node, requirements))
            {
                ssLOG_ERROR("DependencyLinkProperty: Failed to meet requirements for profile " << 
                            profile);
                return false;
            }
            
            for(int j = 0; j < node->GetMapValueNode("SearchLibraryNames")->GetChildrenCount(); ++j)
            {
                std::string searchLibraryName = 
                    node->GetMapValueNode("SearchLibraryNames")
                        ->GetSequenceChildScalar<std::string>(j)
                        .DS_TRY_ACT(return false);
                
                property.SearchLibraryNames.push_back(searchLibraryName);
            }
            
            for(int j = 0; j < node->GetMapValueNode("SearchDirectories")->GetChildrenCount(); ++j)
            {
                std::string searchPath = 
                    node->GetMapValueNode("SearchDirectories")
                        ->GetSequenceChildScalar<std::string>(j)
                        .DS_TRY_ACT(return false);
                property.SearchDirectories.push_back(searchPath);
            }

            if(ExistAndHasChild(node, "ExcludeLibraryNames"))
            {
                for(int j = 0; 
                    j < node->GetMapValueNode("ExcludeLibraryNames")->GetChildrenCount(); 
                    ++j)
                {
                    std::string excludeName =
                        node->GetMapValueNode("ExcludeLibraryNames")
                            ->GetSequenceChildScalar<std::string>(j)
                            .DS_TRY_ACT(return false);
                    property.ExcludeLibraryNames.push_back(excludeName);
                }
            }

            if(ExistAndHasChild(node, "AdditionalLinkOptions"))
            {
                for(int j = 0; j < node->GetMapValueNode("AdditionalLinkOptions")->GetChildrenCount(); ++j)
                {
                    std::string linkOption =
                        node->GetMapValueNode("AdditionalLinkOptions")
                            ->GetSequenceChildScalar<std::string>(j)
                            .DS_TRY_ACT(return false);
                    property.AdditionalLinkOptions.push_back(linkOption);
                }
            }
            
            ProfileProperties[profile] = property;
            
            return true;
        }

        inline bool IsYAML_NodeParsableAsDefault(YAML::ConstNodePtr node) const
        {
            ssLOG_FUNC_DEBUG();

            if(!node->IsMap())
            {
                ssLOG_ERROR("DependencyLinkProperty type requires a map");
                return false;
            }

            if( ExistAndHasChild(node, "SearchLibraryNames") && 
                ExistAndHasChild(node, "SearchDirectories"))
            {
                std::vector<NodeRequirement> requirements =
                {
                    NodeRequirement("SearchLibraryNames", YAML::NodeType::Sequence, true, false),
                    NodeRequirement("SearchDirectories", YAML::NodeType::Sequence, true, false)
                };
                
                return CheckNodeRequirements(node, requirements);
            }
            
            return false;
        }

        inline std::string ToString(std::string indentation) const
        {
            std::string out;
            for(const std::pair<const ProfileName, ProfileLinkProperty>& profilePair : 
                ProfileProperties)
            {
                out += indentation + profilePair.first + ":\n";
                const ProfileLinkProperty& property = profilePair.second;
                
                if(property.SearchLibraryNames.empty())
                    out += indentation + "    SearchLibraryNames: []\n";
                else
                {
                    out += indentation + "    SearchLibraryNames:\n";
                    for(const std::string& name : property.SearchLibraryNames)
                        out += indentation + "    -   " + GetEscapedYAMLString(name) + "\n";
                }
                
                if(property.SearchDirectories.empty())
                    out += indentation + "    SearchDirectories: []\n";
                else
                {
                    out += indentation + "    SearchDirectories:\n";
                    for(const std::string& dir : property.SearchDirectories)
                        out += indentation + "    -   " + GetEscapedYAMLString(dir) + "\n";
                }
                
                if(!property.ExcludeLibraryNames.empty())
                {
                    out += indentation + "    ExcludeLibraryNames:\n";
                    for(const std::string& name : property.ExcludeLibraryNames)
                        out += indentation + "    -   " + GetEscapedYAMLString(name) + "\n";
                }
                
                if(!property.AdditionalLinkOptions.empty())
                {
                    out += indentation + "    AdditionalLinkOptions:\n";
                    for(const std::string& option : property.AdditionalLinkOptions)
                        out += indentation + "    -   " + GetEscapedYAMLString(option) + "\n";
                }
            }
            
            return out;
        }

        inline bool Equals(const DependencyLinkProperty& other) const
        {
            if(ProfileProperties.size() != other.ProfileProperties.size())
                return false;

            for(const auto& it : ProfileProperties)
            {
                if(other.ProfileProperties.count(it.first) == 0)
                    return false;
                    
                const ProfileLinkProperty& otherProperty = other.ProfileProperties.at(it.first);
                const ProfileLinkProperty& property = it.second;
                
                if( property.SearchLibraryNames != otherProperty.SearchLibraryNames ||
                    property.SearchDirectories != otherProperty.SearchDirectories ||
                    property.ExcludeLibraryNames != otherProperty.ExcludeLibraryNames ||
                    property.AdditionalLinkOptions != otherProperty.AdditionalLinkOptions)
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
