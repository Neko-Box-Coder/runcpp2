#include "runcpp2/Data/FilesTypesInfo.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "ssLogger/ssLog.hpp"

bool runcpp2::Data::FilesTypesInfo::ParseYAML_Node(ryml::ConstNodeRef& node)
{
    ssLOG_FUNC_DEBUG();
    
    INTERNAL_RUNCPP2_SAFE_START();

    std::vector<NodeRequirement> requirements =
    {
        NodeRequirement("ObjectLinkFile", ryml::NodeType_e::MAP, true, false),
        NodeRequirement("SharedLinkFile", ryml::NodeType_e::MAP, true, false),
        NodeRequirement("SharedLibraryFile", ryml::NodeType_e::MAP, true, false),
        NodeRequirement("StaticLinkFile", ryml::NodeType_e::MAP, true, false),
        NodeRequirement("DebugSymbolFile", ryml::NodeType_e::MAP, false, false),
    };

    if(!CheckNodeRequirements(node, requirements))
    {
        ssLOG_ERROR("FilesTypesInfo: Failed to meet requirements");
        return false;
    }

    ryml::ConstNodeRef objectLinkFileNode = node["ObjectLinkFile"];
    if(!ObjectLinkFile.ParseYAML_Node(objectLinkFileNode))
    {
        ssLOG_ERROR("Compiler profile: ObjectLinkFile is invalid");
        return false;
    }
    
    ryml::ConstNodeRef sharedLinkFileNode = node["SharedLinkFile"];
    if(!SharedLinkFile.ParseYAML_Node(sharedLinkFileNode))
    {
        ssLOG_ERROR("Compiler profile: SharedLinkFile is invalid");
        return false;
    }
    
    ryml::ConstNodeRef sharedLibraryFileNode = node["SharedLibraryFile"];
    if(!SharedLibraryFile.ParseYAML_Node(sharedLibraryFileNode))
    {
        ssLOG_ERROR("Compiler profile: SharedLibraryFile is invalid");
        return false;
    }
    
    ryml::ConstNodeRef staticLinkFileNode = node["StaticLinkFile"];
    if(!StaticLinkFile.ParseYAML_Node(staticLinkFileNode))
    {
        ssLOG_ERROR("Compiler profile: StaticLinkFile is invalid");
        return false;
    }
    
    if(ExistAndHasChild(node, "DebugSymbolFile"))
    {
        ryml::ConstNodeRef debugSymbolFileNode = node["DebugSymbolFile"];
        if(!DebugSymbolFile.ParseYAML_Node(debugSymbolFileNode))
        {
            ssLOG_ERROR("Compiler profile: DebugSymbolFile is invalid");
            return false;
        }
    }
    
    return true;
        
    INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
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
    
    out += indentation + "DebugSymbolFile:\n";
    out += DebugSymbolFile.ToString(indentation + "    ");
    
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
