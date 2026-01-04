#include "runcpp2/Data/FilesTypesInfo.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "ssLogger/ssLog.hpp"

bool runcpp2::Data::FilesTypesInfo::ParseYAML_Node(YAML::ConstNodePtr node)
{
    ssLOG_FUNC_DEBUG();

    std::vector<NodeRequirement> requirements =
    {
        NodeRequirement("ObjectLinkFile", YAML::NodeType::Map, true, false),
        NodeRequirement("SharedLinkFile", YAML::NodeType::Map, true, false),
        NodeRequirement("SharedLibraryFile", YAML::NodeType::Map, true, false),
        NodeRequirement("StaticLinkFile", YAML::NodeType::Map, true, false),
        NodeRequirement("DebugSymbolFile", YAML::NodeType::Map, false, false),
    };

    if(!CheckNodeRequirements(node, requirements))
    {
        ssLOG_ERROR("FilesTypesInfo: Failed to meet requirements");
        return false;
    }

    YAML::ConstNodePtr objectLinkFileNode = node->GetMapValueNode("ObjectLinkFile");
    if(!ObjectLinkFile.ParseYAML_Node(objectLinkFileNode))
    {
        ssLOG_ERROR("Compiler profile: ObjectLinkFile is invalid");
        return false;
    }
    
    YAML::ConstNodePtr sharedLinkFileNode = node->GetMapValueNode("SharedLinkFile");
    if(!SharedLinkFile.ParseYAML_Node(sharedLinkFileNode))
    {
        ssLOG_ERROR("Compiler profile: SharedLinkFile is invalid");
        return false;
    }
    
    YAML::ConstNodePtr sharedLibraryFileNode = node->GetMapValueNode("SharedLibraryFile");
    if(!SharedLibraryFile.ParseYAML_Node(sharedLibraryFileNode))
    {
        ssLOG_ERROR("Compiler profile: SharedLibraryFile is invalid");
        return false;
    }
    
    YAML::ConstNodePtr staticLinkFileNode = node->GetMapValueNode("StaticLinkFile");
    if(!StaticLinkFile.ParseYAML_Node(staticLinkFileNode))
    {
        ssLOG_ERROR("Compiler profile: StaticLinkFile is invalid");
        return false;
    }
    
    if(ExistAndHasChild(node, "DebugSymbolFile"))
    {
        YAML::ConstNodePtr debugSymbolFileNode = node->GetMapValueNode("DebugSymbolFile");
        if(!DebugSymbolFile.ParseYAML_Node(debugSymbolFileNode))
        {
            ssLOG_ERROR("Compiler profile: DebugSymbolFile is invalid");
            return false;
        }
    }
    
    return true;
}

std::string runcpp2::Data::FilesTypesInfo::ToString(std::string indentation) const
{
    ssLOG_FUNC_DEBUG();
    
    std::string out;
    
    out += indentation + "ObjectLinkFile:\n";
    out += ObjectLinkFile.ToString(indentation + "    ");
    
    out += indentation + "SharedLinkFile:\n";
    out += SharedLinkFile.ToString(indentation + "    ");
    
    out += indentation + "SharedLibraryFile:\n";
    out += SharedLibraryFile.ToString(indentation + "    ");
    
    out += indentation + "StaticLinkFile:\n";
    out += StaticLinkFile.ToString(indentation + "    ");
    
    if(!DebugSymbolFile.Prefix.empty() || !DebugSymbolFile.Extension.empty())
    {
        out += indentation + "DebugSymbolFile:\n";
        out += DebugSymbolFile.ToString(indentation + "    ");
    }
    
    return out;
}

bool runcpp2::Data::FilesTypesInfo::Equals(const FilesTypesInfo& other) const
{
    return  ObjectLinkFile.Equals(other.ObjectLinkFile) &&
            SharedLinkFile.Equals(other.SharedLinkFile) &&
            SharedLibraryFile.Equals(other.SharedLibraryFile) &&
            StaticLinkFile.Equals(other.StaticLinkFile) &&
            DebugSymbolFile.Equals(other.DebugSymbolFile);
}
