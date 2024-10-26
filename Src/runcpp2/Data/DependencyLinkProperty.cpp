#include "runcpp2/Data/DependencyLinkProperty.hpp"
#include "runcpp2/Data/ParseCommon.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "ssLogger/ssLog.hpp"

bool runcpp2::Data::DependencyLinkProperty::ParseYAML_Node(ryml::ConstNodeRef& node)
{
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

std::string runcpp2::Data::DependencyLinkProperty::ToString(std::string indentation) const
{
    std::string out;
    for(const std::pair<const ProfileName, ProfileLinkProperty>& profilePair : ProfileProperties)
    {
        out += indentation + profilePair.first + ":\n";
        const ProfileLinkProperty& property = profilePair.second;
        
        out += indentation + "    SearchLibraryNames:\n";
        for(const std::string& name : property.SearchLibraryNames)
            out += indentation + "    -   " + name + "\n";
        
        out += indentation + "    SearchDirectories:\n";
        for(const std::string& dir : property.SearchDirectories)
            out += indentation + "    -   " + dir + "\n";
        
        if(!property.ExcludeLibraryNames.empty())
        {
            out += indentation + "    ExcludeLibraryNames:\n";
            for(const std::string& name : property.ExcludeLibraryNames)
                out += indentation + "    -   " + name + "\n";
        }
        
        if(!property.AdditionalLinkOptions.empty())
        {
            out += indentation + "    AdditionalLinkOptions:\n";
            for(const std::string& option : property.AdditionalLinkOptions)
                out += indentation + "    -   " + option + "\n";
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
