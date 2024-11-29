#include "runcpp2/Data/DependencyInfo.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "runcpp2/Data/ParseCommon.hpp"
#include "ssLogger/ssLog.hpp"

bool runcpp2::Data::DependencyInfo::ParseYAML_Node(ryml::ConstNodeRef& node)
{
    INTERNAL_RUNCPP2_SAFE_START();

    //If import is needed, we only need to parse the Source section
    do
    {
        if(!ExistAndHasChild(node, "Source"))
        {
            ssLOG_ERROR("DependencyInfo: Missing Source");
            return false;
        }

        ryml::ConstNodeRef sourceNode = node["Source"];

        if(!ExistAndHasChild(sourceNode, "ImportPath"))
            break;

        if(!Source.ParseYAML_Node(sourceNode))
        {
            ssLOG_ERROR("DependencyInfo: Failed to parse Source");
            return false;
        }

        ssLOG_DEBUG("DependencyInfo: Importing from " << Source.ImportPath.string());
        ssLOG_DEBUG("Skipping the rest of the DependencyInfo");
        return true;
    }
    while(false);

    std::vector<NodeRequirement> requirements =
    {
        NodeRequirement("Name", ryml::NodeType_e::KEYVAL, true, false),
        NodeRequirement("Platforms", ryml::NodeType_e::SEQ, true, false),
        NodeRequirement("Source", ryml::NodeType_e::MAP, true, false),
        NodeRequirement("LibraryType", ryml::NodeType_e::KEYVAL, true, false),
        NodeRequirement("IncludePaths", ryml::NodeType_e::SEQ, false, true),
        NodeRequirement("LinkProperties", ryml::NodeType_e::MAP, false, false),
        NodeRequirement("Setup", ryml::NodeType_e::MAP, false, true),
        NodeRequirement("Cleanup", ryml::NodeType_e::MAP, false, true),
        NodeRequirement("Build", ryml::NodeType_e::MAP, false, true),
        NodeRequirement("FilesToCopy", ryml::NodeType_e::MAP, false, true)
    };
    
    if(!CheckNodeRequirements(node, requirements))
    {
        ssLOG_ERROR("DependencyInfo: Failed to meet requirements");
        return false;
    }
    
    node["Name"] >> Name;
    
    for(int i = 0; i < node["Platforms"].num_children(); ++i)
        Platforms.insert(GetValue(node["Platforms"][i]));
    
    ryml::ConstNodeRef sourceNode = node["Source"];
    if(!Source.ParseYAML_Node(sourceNode))
    {
        ssLOG_ERROR("DependencyInfo: Failed to parse Source");
        return false;
    }
    
    static_assert((int)DependencyLibraryType::COUNT == 4, "");
    
    if(node["LibraryType"].val() == "Static")
        LibraryType = DependencyLibraryType::STATIC;
    else if(node["LibraryType"].val() == "Shared")
        LibraryType = DependencyLibraryType::SHARED;
    else if(node["LibraryType"].val() == "Object")
        LibraryType = DependencyLibraryType::OBJECT;
    else if(node["LibraryType"].val() == "Header")
        LibraryType = DependencyLibraryType::HEADER;
    else
    {
        ssLOG_ERROR("DependencyInfo: LibraryType is invalid");
        return false;
    }
    
    if(ExistAndHasChild(node, "IncludePaths"))
    {
        ryml::ConstNodeRef includePathsNode = node["IncludePaths"];
        
        for(int i = 0; i < includePathsNode.num_children(); i++)
            IncludePaths.push_back(GetValue(includePathsNode[i]));
    }
    
    if(ExistAndHasChild(node, "LinkProperties"))
    {
        ryml::ConstNodeRef linkPropertiesNode = node["LinkProperties"];
        
        for(int i = 0; i < linkPropertiesNode.num_children(); ++i)
        {
            PlatformName platform = GetKey(linkPropertiesNode[i]);
            ryml::ConstNodeRef platformNode = linkPropertiesNode[i];
            
            //Insert an empty DependencyLinkProperty and get a reference to it
            DependencyLinkProperty& linkProperty = LinkProperties[platform];
            
            if(!linkProperty.ParseYAML_Node(platformNode))
            {
                ssLOG_ERROR("DependencyInfo: Failed to parse LinkProperties for platform " << 
                            platform);
                return false;
            }
        }
    }
    else if(LibraryType != DependencyLibraryType::HEADER)
    {
        ssLOG_ERROR("DependencyInfo: Missing LinkProperties with library type " << 
                    Data::DependencyLibraryTypeToString(LibraryType));
        return false;
    }
    
    if(ExistAndHasChild(node, "Setup"))
    {
        for(int i = 0; i < node["Setup"].num_children(); ++i)
        {
            ProfilesCommands currentSetup;
            ryml::ConstNodeRef currentSetupNode = node["Setup"][i];
            
            if(!currentSetup.ParseYAML_Node(currentSetupNode))
            {
                ssLOG_ERROR("DependencyInfo: Failed to parse Setup");
                return false;
            }
            
            Setup[GetKey(node["Setup"][i])] = currentSetup;
        }
    }
    
    if(ExistAndHasChild(node, "Cleanup"))
    {
        for(int i = 0; i < node["Cleanup"].num_children(); ++i)
        {
            ProfilesCommands currentCleanup;
            ryml::ConstNodeRef currentCleanupNode = node["Cleanup"][i];
            
            if(!currentCleanup.ParseYAML_Node(currentCleanupNode))
            {
                ssLOG_ERROR("DependencyInfo: Failed to parse Cleanup");
                return false;
            }
            
            Cleanup[GetKey(node["Cleanup"][i])] = currentCleanup;
        }
    }
    
    if(ExistAndHasChild(node, "Build"))
    {
        for(int i = 0; i < node["Build"].num_children(); ++i)
        {
            ProfilesCommands currentBuild;
            ryml::ConstNodeRef currentBuildNode = node["Build"][i];
            
            if(!currentBuild.ParseYAML_Node(currentBuildNode))
            {
                ssLOG_ERROR("DependencyInfo: Failed to parse Build");
                return false;
            }
            
            Build[GetKey(node["Build"][i])] = currentBuild;
        }
    }
    
    if(ExistAndHasChild(node, "FilesToCopy"))
    {
        for(int i = 0; i < node["FilesToCopy"].num_children(); ++i)
        {
            FilesToCopyInfo currentFilesToCopy;
            ryml::ConstNodeRef currentFilesToCopyNode = node["FilesToCopy"][i];
            PlatformName platform = GetKey(currentFilesToCopyNode);
            
            if(!currentFilesToCopy.ParseYAML_Node(currentFilesToCopyNode))
            {
                ssLOG_ERROR("DependencyInfo: Failed to parse FilesToCopy");
                return false;
            }
            
            FilesToCopy[platform] = currentFilesToCopy;
        }
    }

    return true;
    
    INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
}

std::string runcpp2::Data::DependencyInfo::ToString(std::string indentation) const
{
    std::string out;
    out += indentation + "Name: " + GetEscapedYAMLString(Name) + "\n";
    
    out += indentation + "Platforms:\n";
    for(auto it = Platforms.begin(); it != Platforms.end(); ++it)
        out += indentation + "-   " + GetEscapedYAMLString(*it) + "\n";
    
    out += indentation + "Source:\n";
    out += Source.ToString(indentation + "    ");
    
    static_assert((int)DependencyLibraryType::COUNT == 4, "");
    
    if(LibraryType == DependencyLibraryType::STATIC)
        out += indentation + "LibraryType: Static\n";
    else if(LibraryType == DependencyLibraryType::SHARED)
        out += indentation + "LibraryType: Shared\n";
    else if(LibraryType == DependencyLibraryType::OBJECT)
        out += indentation + "LibraryType: Object\n";
    else if(LibraryType == DependencyLibraryType::HEADER)
        out += indentation + "LibraryType: Header\n";
    
    if(!IncludePaths.empty())
    {
        out += indentation + "IncludePaths:\n";
        for(auto it = IncludePaths.begin(); it != IncludePaths.end(); ++it)
            out += indentation + "-   " + GetEscapedYAMLString(*it) + "\n";
    }
    
    if(!LinkProperties.empty())
    {
        out += indentation + "LinkProperties:\n";
        for(auto it = LinkProperties.begin(); it != LinkProperties.end(); ++it)
        {
            out += indentation + "    " + it->first + ":\n";
            out += it->second.ToString(indentation + "        ");
        }
    }
    
    if(!Setup.empty())
    {
        out += indentation + "Setup:\n";
        for(auto it = Setup.begin(); it != Setup.end(); ++it)
        {
            out += indentation + "    " + it->first + ":\n";
            out += it->second.ToString(indentation + "        ");
        }
    }
    
    if(!Cleanup.empty())
    {
        out += indentation + "Cleanup:\n";
        for(auto it = Cleanup.begin(); it != Cleanup.end(); ++it)
        {
            out += indentation + "    " + it->first + ":\n";
            out += it->second.ToString(indentation + "        ");
        }
    }
    
    if(!Build.empty())
    {
        out += indentation + "Build:\n";
        for(auto it = Build.begin(); it != Build.end(); ++it)
        {
            out += indentation + "    " + it->first + ":\n";
            out += it->second.ToString(indentation + "        ");
        }
    }
    
    if(!FilesToCopy.empty())
    {
        out += indentation + "FilesToCopy:\n";
        for(auto it = FilesToCopy.begin(); it != FilesToCopy.end(); ++it)
        {
            out += indentation + "    " + it->first + ":\n";
            out += it->second.ToString(indentation + "        ");
        }
    }
    
    return out;
}

bool runcpp2::Data::DependencyInfo::Equals(const DependencyInfo& other) const
{
    if( Name != other.Name || 
        Platforms.size() != other.Platforms.size() ||
        !Source.Equals(other.Source) ||
        LibraryType != other.LibraryType ||
        IncludePaths != other.IncludePaths ||
        AbsoluteIncludePaths != other.AbsoluteIncludePaths ||
        LinkProperties.size() != other.LinkProperties.size() ||
        Setup.size() != other.Setup.size() ||
        Cleanup.size() != other.Cleanup.size() ||
        Build.size() != other.Build.size() ||
        FilesToCopy.size() != other.FilesToCopy.size())
    {
        return false;
    }

    for(const std::string& it : Platforms)
    {
        if(other.Platforms.count(it) == 0)
            return false;
    }

    for(const auto& it : LinkProperties)
    {
        if( other.LinkProperties.count(it.first) == 0 || 
            !other.LinkProperties.at(it.first).Equals(it.second))
        {
            return false;
        }
    }

    for(const auto& it : Setup)
    {
        if(other.Setup.count(it.first) == 0 || !other.Setup.at(it.first).Equals(it.second))
            return false;
    }

    for(const auto& it : Cleanup)
    {
        if(other.Cleanup.count(it.first) == 0 || !other.Cleanup.at(it.first).Equals(it.second))
            return false;
    }

    for(const auto& it : Build)
    {
        if(other.Build.count(it.first) == 0 || !other.Build.at(it.first).Equals(it.second))
            return false;
    }

    for(const auto& it : FilesToCopy)
    {
        if( other.FilesToCopy.count(it.first) == 0 || 
            !other.FilesToCopy.at(it.first).Equals(it.second))
        {
            return false;
        }
    }

    return true;
}
