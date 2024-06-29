#include "runcpp2/Data/Profile.hpp"
#include "runcpp2/Data/ParseCommon.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "ssLogger/ssLog.hpp"

bool runcpp2::Data::Profile::ParseYAML_Node(ryml::ConstNodeRef& profileNode)
{
    INTERNAL_RUNCPP2_SAFE_START();

    std::vector<NodeRequirement> requirements =
    {
        NodeRequirement("Name", ryml::NodeType_e::KEYVAL, true, false),
        NodeRequirement("NameAliases", ryml::NodeType_e::SEQ, false, true),
        NodeRequirement("FileExtensions", ryml::NodeType_e::SEQ, true, false),
        NodeRequirement("Languages", ryml::NodeType_e::SEQ, false, true),
        NodeRequirement("SetupSteps", ryml::NodeType_e::MAP, false, true),
        NodeRequirement("ObjectLinkFile", ryml::NodeType_e::MAP, true, false),
        NodeRequirement("SharedLinkFile", ryml::NodeType_e::MAP, true, false),
        NodeRequirement("SharedLibraryFile", ryml::NodeType_e::MAP, true, false),
        NodeRequirement("StaticLinkFile", ryml::NodeType_e::MAP, true, false),
        NodeRequirement("DebugSymbolFile", ryml::NodeType_e::MAP, false, false),
        NodeRequirement("Compiler", ryml::NodeType_e::MAP, true, false),
        NodeRequirement("Linker", ryml::NodeType_e::MAP, true, false)
    };
    
    if(!CheckNodeRequirements(profileNode, requirements))
    {
        ssLOG_ERROR("Compiler profile: Failed to meet requirements");
        return false;
    }
    
    profileNode["Name"] >> Name;
    
    if(ExistAndHasChild(profileNode, "NameAliases"))
    {
        for(int i = 0; i < profileNode["NameAliases"].num_children(); ++i)
            NameAliases.insert(GetValue(profileNode["NameAliases"][i]));
    }

    for(int i = 0; i < profileNode["FileExtensions"].num_children(); ++i)
        FileExtensions.insert(GetValue(profileNode["FileExtensions"][i]));
    
    if(ExistAndHasChild(profileNode, "Languages"))
    {
        for(int i = 0; i < profileNode["Languages"].num_children(); ++i)
            Languages.insert(GetValue(profileNode["Languages"][i]));
    }
    
    if(ExistAndHasChild(profileNode, "SetupSteps"))
    {
        for(int i = 0; i < profileNode["SetupSteps"].num_children(); ++i)
        {
            ryml::ConstNodeRef currentPlatform = profileNode["SetupSteps"][i];
            
            std::string key = GetKey(currentPlatform);
            std::vector<std::string> extensions;
            
            for(int j = 0; j < currentPlatform.num_children(); ++j)
                extensions.push_back(GetValue(currentPlatform[j]));
            
            SetupSteps[key] = extensions;
        }
    }
    
    ryml::ConstNodeRef objectLinkFileNode = profileNode["ObjectLinkFile"];
    if(!ObjectLinkFile.ParseYAML_Node(objectLinkFileNode))
    {
        ssLOG_ERROR("Compiler profile: ObjectLinkFile is invalid");
        return false;
    }
    
    ryml::ConstNodeRef sharedLinkFileNode = profileNode["SharedLinkFile"];
    if(!SharedLinkFile.ParseYAML_Node(sharedLinkFileNode))
    {
        ssLOG_ERROR("Compiler profile: SharedLinkFile is invalid");
        return false;
    }
    
    ryml::ConstNodeRef sharedLibraryFileNode = profileNode["SharedLibraryFile"];
    if(!SharedLibraryFile.ParseYAML_Node(sharedLibraryFileNode))
    {
        ssLOG_ERROR("Compiler profile: SharedLibraryFile is invalid");
        return false;
    }
    
    ryml::ConstNodeRef staticLinkFileNode = profileNode["StaticLinkFile"];
    if(!StaticLinkFile.ParseYAML_Node(staticLinkFileNode))
    {
        ssLOG_ERROR("Compiler profile: StaticLinkFile is invalid");
        return false;
    }
    
    if(ExistAndHasChild(profileNode, "DebugSymbolFile"))
    {
        ryml::ConstNodeRef debugSymbolFileNode = profileNode["DebugSymbolFile"];
        if(!DebugSymbolFile.ParseYAML_Node(debugSymbolFileNode))
        {
            ssLOG_ERROR("Compiler profile: DebugSymbolFile is invalid");
            return false;
        }
    }
    
    ryml::ConstNodeRef compilerNode = profileNode["Compiler"];
    if(!Compiler.ParseYAML_Node(compilerNode))
    {
        ssLOG_ERROR("Compiler profile: Compiler is invalid");
        return false;
    }
    
    ryml::ConstNodeRef linkerNode = profileNode["Linker"];
    if(!Linker.ParseYAML_Node(linkerNode))
    {
        ssLOG_ERROR("Compiler profile: Linker is invalid");
        return false;
    }
    
    return true;
    
    INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
}

std::string runcpp2::Data::Profile::ToString(std::string indentation) const
{
    std::string out;
    
    out += indentation + "Name: " + Name + "\n";
    out += indentation + "NameAliases:\n";
    for(auto it = NameAliases.begin(); it != NameAliases.end(); ++it)
        out += indentation + "-   " + *it + "\n";
    
    out += indentation + "FileExtensions:\n";
    for(auto it = FileExtensions.begin(); it != FileExtensions.end(); ++it)
        out += indentation + "-   " + *it + "\n";
    
    out += indentation + "Languages:\n";
    for(auto it = Languages.begin(); it != Languages.end(); ++it)
        out += indentation + "-   " + *it + "\n";
    
    out += indentation + "SetupSteps:\n";
    for(auto it = SetupSteps.begin(); it != SetupSteps.end(); ++it)
    {
        out += indentation + "    " + it->first + ":\n";
        for(int i = 0; i < it->second.size(); ++i)
            out += indentation + "    -   " + it->second[i] + "\n";
    }
    
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
    
    out += Compiler.ToString(indentation + "    ");
    out += Linker.ToString(indentation + "    ");
    
    return out;
}
