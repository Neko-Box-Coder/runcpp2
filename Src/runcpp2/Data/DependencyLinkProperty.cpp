#include "runcpp2/Data/DependencyLinkProperty.hpp"
#include "runcpp2/Data/ParseCommon.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "ssLogger/ssLog.hpp"

bool runcpp2::Data::DependencyLinkProperty::ParseYAML_Node(ryml::ConstNodeRef node)
{
    ssLOG_FUNC_DEBUG();
    INTERNAL_RUNCPP2_SAFE_START();
    
    if(!node.is_map())
    {
        ssLOG_ERROR("DependencyLinkProperty: Node is not a Map");
        return false;
    }
    
    for(int i = 0; i < node.num_children(); ++i)
    {
        ProfileName profile = GetKey(node[i]);
        ryml::ConstNodeRef profileNode = node[i];
        
        ProfileLinkProperty& property = ProfileProperties[profile];
        
        std::vector<NodeRequirement> requirements =
        {
            NodeRequirement("SearchLibraryNames", ryml::NodeType_e::SEQ, true, false),
            NodeRequirement("SearchDirectories", ryml::NodeType_e::SEQ, true, false),
            NodeRequirement("ExcludeLibraryNames", ryml::NodeType_e::SEQ, false, true),
            NodeRequirement("AdditionalLinkOptions", ryml::NodeType_e::SEQ, false, true)
        };
        
        if(!CheckNodeRequirements(profileNode, requirements))
        {
            ssLOG_ERROR("DependencyLinkProperty: Failed to meet requirements for profile " << 
                        profile);
            return false;
        }
        
        for(int j = 0; j < profileNode["SearchLibraryNames"].num_children(); ++j)
            property.SearchLibraryNames.push_back(GetValue(profileNode["SearchLibraryNames"][j]));
        
        for(int j = 0; j < profileNode["SearchDirectories"].num_children(); ++j)
            property.SearchDirectories.push_back(GetValue(profileNode["SearchDirectories"][j]));

        if(ExistAndHasChild(profileNode, "ExcludeLibraryNames"))
        {
            for(int j = 0; j < profileNode["ExcludeLibraryNames"].num_children(); ++j)
            {
                property.ExcludeLibraryNames
                        .push_back(GetValue(profileNode["ExcludeLibraryNames"][j]));
                
            }
        }

        if(ExistAndHasChild(profileNode, "AdditionalLinkOptions"))
        {
            for(int j = 0; j < profileNode["AdditionalLinkOptions"].num_children(); ++j)
            {
                property.AdditionalLinkOptions
                        .push_back(GetValue(profileNode["AdditionalLinkOptions"][j]));
            }
        }
    }

    return true;
    
    INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
}

bool runcpp2::Data::DependencyLinkProperty::ParseYAML_Node(YAML::ConstNodePtr node)
{
    ssLOG_FUNC_DEBUG();
    
    if(!node->IsMap())
    {
        ssLOG_ERROR("DependencyLinkProperty: Node is not a Map");
        return false;
    }
    
    for(int i = 0; i < node->GetChildrenCount(); ++i)
    {
        ProfileName profile = node->GetMapKeyScalarAt<ProfileName>(i).DS_TRY_ACT(return false);
        YAML::ConstNodePtr profileNode = node->GetMapValueNodeAt(i);
        
        //TODO: Maybe use `ParseYAML_NodeWithProfile_LibYaml()`?
        
        ProfileLinkProperty& property = ProfileProperties[profile];
        
        std::vector<NodeRequirement> requirements =
        {
            NodeRequirement("SearchLibraryNames", YAML::NodeType::Sequence, true, false),
            NodeRequirement("SearchDirectories", YAML::NodeType::Sequence, true, false),
            NodeRequirement("ExcludeLibraryNames", YAML::NodeType::Sequence, false, true),
            NodeRequirement("AdditionalLinkOptions", YAML::NodeType::Sequence, false, true)
        };
        
        if(!CheckNodeRequirements_LibYaml(profileNode, requirements))
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

        if(ExistAndHasChild_LibYaml(profileNode, "ExcludeLibraryNames"))
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

        if(ExistAndHasChild_LibYaml(profileNode, "AdditionalLinkOptions"))
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

bool runcpp2::Data::DependencyLinkProperty::ParseYAML_NodeWithProfile(  ryml::ConstNodeRef node, 
                                                                        ProfileName profile)
{
    ssLOG_FUNC_DEBUG();
    INTERNAL_RUNCPP2_SAFE_START();
    
    ProfileLinkProperty& property = ProfileProperties[profile];
    
    std::vector<NodeRequirement> requirements =
    {
        NodeRequirement("SearchLibraryNames", ryml::NodeType_e::SEQ, true, false),
        NodeRequirement("SearchDirectories", ryml::NodeType_e::SEQ, true, false),
        NodeRequirement("ExcludeLibraryNames", ryml::NodeType_e::SEQ, false, true),
        NodeRequirement("AdditionalLinkOptions", ryml::NodeType_e::SEQ, false, true)
    };
    
    if(!CheckNodeRequirements(node, requirements))
    {
        ssLOG_ERROR("DependencyLinkProperty: Failed to meet requirements for profile " << 
                    profile);
        return false;
    }
    
    for(int i = 0; i < node["SearchLibraryNames"].num_children(); ++i)
        property.SearchLibraryNames.push_back(GetValue(node["SearchLibraryNames"][i]));
    
    for(int i = 0; i < node["SearchDirectories"].num_children(); ++i)
        property.SearchDirectories.push_back(GetValue(node["SearchDirectories"][i]));

    if(ExistAndHasChild(node, "ExcludeLibraryNames"))
    {
        for(int i = 0; i < node["ExcludeLibraryNames"].num_children(); ++i)
        {
            property.ExcludeLibraryNames
                    .push_back(GetValue(node["ExcludeLibraryNames"][i]));
            
        }
    }

    if(ExistAndHasChild(node, "AdditionalLinkOptions"))
    {
        for(int i = 0; i < node["AdditionalLinkOptions"].num_children(); ++i)
        {
            property.AdditionalLinkOptions
                    .push_back(GetValue(node["AdditionalLinkOptions"][i]));
        }
    }
    
    ProfileProperties[profile] = property;
    
    return true;
    INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
}

bool 
runcpp2::Data::DependencyLinkProperty::IsYAML_NodeParsableAsDefault(ryml::ConstNodeRef node) const
{
    ssLOG_FUNC_DEBUG();
    INTERNAL_RUNCPP2_SAFE_START();

    if(!INTERNAL_RUNCPP2_BIT_CONTANTS(node.type().type, ryml::NodeType_e::MAP))
    {
        ssLOG_ERROR("DependencyLinkProperty type requires a map");
        return false;
    }

    if(ExistAndHasChild(node, "SearchLibraryNames") && ExistAndHasChild(node, "SearchDirectories"))
    {
        std::vector<NodeRequirement> requirements =
        {
            NodeRequirement("SearchLibraryNames", ryml::NodeType_e::SEQ, true, false),
            NodeRequirement("SearchDirectories", ryml::NodeType_e::SEQ, true, false)
        };
        
        return CheckNodeRequirements(node, requirements);
    }
    
    return false;
    INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
}

bool 
runcpp2::Data::DependencyLinkProperty::ParseYAML_NodeWithProfile_LibYaml(   YAML::ConstNodePtr node, 
                                                                            ProfileName profile)
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
    
    if(!CheckNodeRequirements_LibYaml(node, requirements))
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

    if(ExistAndHasChild_LibYaml(node, "ExcludeLibraryNames"))
    {
        for(int j = 0; j < node->GetMapValueNode("ExcludeLibraryNames")->GetChildrenCount(); ++j)
        {
            std::string excludeName =
                node->GetMapValueNode("ExcludeLibraryNames")
                    ->GetSequenceChildScalar<std::string>(j)
                    .DS_TRY_ACT(return false);
            property.ExcludeLibraryNames.push_back(excludeName);
        }
    }

    if(ExistAndHasChild_LibYaml(node, "AdditionalLinkOptions"))
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

bool 
runcpp2::Data::DependencyLinkProperty::IsYAML_NodeParsableAsDefault_LibYaml(YAML::ConstNodePtr node) const
{
    ssLOG_FUNC_DEBUG();

    if(!node->IsMap())
    {
        ssLOG_ERROR("DependencyLinkProperty type requires a map");
        return false;
    }

    if( ExistAndHasChild_LibYaml(node, "SearchLibraryNames") && 
        ExistAndHasChild_LibYaml(node, "SearchDirectories"))
    {
        std::vector<NodeRequirement> requirements =
        {
            NodeRequirement("SearchLibraryNames", YAML::NodeType::Sequence, true, false),
            NodeRequirement("SearchDirectories", YAML::NodeType::Sequence, true, false)
        };
        
        return CheckNodeRequirements_LibYaml(node, requirements);
    }
    
    return false;
}

std::string runcpp2::Data::DependencyLinkProperty::ToString(std::string indentation) const
{
    std::string out;
    for(const std::pair<const ProfileName, ProfileLinkProperty>& profilePair : ProfileProperties)
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

bool runcpp2::Data::DependencyLinkProperty::Equals(const DependencyLinkProperty& other) const
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
