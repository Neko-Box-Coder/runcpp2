#include "runcpp2/Data/CompilerProfile.hpp"
#include "runcpp2/Data/ParseCommon.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "ssLogger/ssLog.hpp"

bool runcpp2::Data::CompilerProfile::ParseYAML_Node(ryml::ConstNodeRef& profileNode)
{
    std::vector<NodeRequirement> requirements =
    {
        NodeRequirement("Name", ryml::NodeType_e::KEYVAL, true, false),
        NodeRequirement("FileExtensions", ryml::NodeType_e::SEQ, true, false),
        NodeRequirement("Languages", ryml::NodeType_e::SEQ, false, true),
        NodeRequirement("SetupSteps", ryml::NodeType_e::SEQ, false, true),
        NodeRequirement("ObjectFileExtensions", ryml::NodeType_e::MAP, true, false),
        NodeRequirement("SharedLibraryExtensions", ryml::NodeType_e::MAP, true, false),
        NodeRequirement("StaticLibraryExtensions", ryml::NodeType_e::MAP, true, false),
        NodeRequirement("DebugSymbolFileExtensions", ryml::NodeType_e::MAP, true, false),
        NodeRequirement("Compiler", ryml::NodeType_e::MAP, true, false),
        NodeRequirement("Linker", ryml::NodeType_e::MAP, true, false)
    };
    
    if(!CheckNodeRequirements(profileNode, requirements))
    {
        ssLOG_ERROR("Compiler profile: Failed to meet requirements");
        return false;
    }
    
    profileNode["Name"] >> Name;
    
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
            SetupSteps.push_back(GetValue(profileNode["SetupSteps"][i]));
    }
    
    for(int i = 0; i < profileNode["ObjectFileExtensions"].num_children(); ++i)
    {
        std::string key = GetKey(profileNode["ObjectFileExtensions"][i]);
        std::string value = GetValue(profileNode["ObjectFileExtensions"][i]);
        ObjectFileExtensions[key] = value;
    }
    
    for(int i = 0; i < profileNode["SharedLibraryExtensions"].num_children(); ++i)
    {
        ryml::ConstNodeRef currentPlatform = profileNode["SharedLibraryExtensions"][i];
        
        std::string key = GetKey(currentPlatform);
        std::vector<std::string> extensions;
        
        for(int j = 0; j < currentPlatform.num_children(); ++j)
            extensions.push_back(GetValue(currentPlatform[j]));
        
        SharedLibraryExtensions[key] = extensions;
    }
    
    for(int i = 0; i < profileNode["StaticLibraryExtensions"].num_children(); ++i)
    {
        ryml::ConstNodeRef currentPlatform = profileNode["StaticLibraryExtensions"][i];
        
        std::string key = GetKey(currentPlatform);
        std::vector<std::string> extensions;
        
        for(int j = 0; j < currentPlatform.num_children(); ++j)
            extensions.push_back(GetValue(currentPlatform[j]));
        
        StaticLibraryExtensions[key] = extensions;
    }
    
    for(int i = 0; i < profileNode["DebugSymbolFileExtensions"].num_children(); ++i)
    {
        ryml::ConstNodeRef currentPlatform = profileNode["DebugSymbolFileExtensions"][i];
        
        std::string key = GetKey(currentPlatform);
        std::vector<std::string> extensions;
        
        for(int j = 0; j < currentPlatform.num_children(); ++j)
            extensions.push_back(GetValue(currentPlatform[j]));
        
        DebugSymbolFileExtensions[key] = extensions;
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
}

std::string runcpp2::Data::CompilerProfile::ToString(std::string indentation) const
{
    std::string out;
    
    out += indentation + "Name: " + Name + "\n";
    
    out += indentation + "FileExtensions:\n";
    for(auto it = FileExtensions.begin(); it != FileExtensions.end(); ++it)
        out += indentation + "-   " + *it + "\n";
    
    out += indentation + "Languages:\n";
    for(auto it = Languages.begin(); it != Languages.end(); ++it)
        out += indentation + "-   " + *it + "\n";
    
    out += indentation + "SetupSteps:\n";
    for(int i = 0; i < SetupSteps.size(); ++i)
        out += indentation + "-   " + SetupSteps[i] + "\n";
    
    out += indentation + "ObjectFileExtensions:\n";
    for(auto it = ObjectFileExtensions.begin(); it != ObjectFileExtensions.end(); ++it)
        out += indentation + "-   " + it->first + ": " + it->second + "\n";
    
    out += indentation + "SharedLibraryExtensions:\n";
    for(auto it = SharedLibraryExtensions.begin(); it != SharedLibraryExtensions.end(); ++it)
    {
        out += indentation + "    " + it->first + ":\n";
        for(int i = 0; i < it->second.size(); ++i)
            out += indentation + "    -   " + it->second[i] + "\n";
    }
    
    out += indentation + "StaticLibraryExtensions:\n";
    for(auto it = StaticLibraryExtensions.begin(); it != StaticLibraryExtensions.end(); ++it)
    {
        out += indentation + "    " + it->first + ":\n";
        for(int i = 0; i < it->second.size(); ++i)
            out += indentation + "    -   " + it->second[i] + "\n";
    }
    
    out += indentation + "DebugSymbolFileExtensions:\n";
    for(auto it = DebugSymbolFileExtensions.begin(); it != DebugSymbolFileExtensions.end(); ++it)
    {
        out += indentation + "    " + it->first + ":\n";
        for(int i = 0; i < it->second.size(); ++i)
            out += indentation + "    -   " + it->second[i] + "\n";
    }
    
    out += Compiler.ToString(indentation + "    ");
    out += Linker.ToString(indentation + "    ");
    
    return out;
}