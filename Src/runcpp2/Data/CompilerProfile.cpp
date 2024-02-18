#include "runcpp2/Data/CompilerProfile.hpp"
#include "runcpp2/Data/ParseCommon.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "ssLogger/ssLog.hpp"

namespace runcpp2
{
    bool CompilerProfile::ParseYAML_Node(YAML::Node& profileNode)
    {
        INTERNAL_RUNCPP2_SAFE_START();

        std::vector<Internal::NodeRequirement> requirements =
        {
            Internal::NodeRequirement("Name", YAML::NodeType::Scalar, true, false),
            Internal::NodeRequirement("FileExtensions", YAML::NodeType::Sequence, true, false),
            Internal::NodeRequirement("Languages", YAML::NodeType::Sequence, false, true),
            Internal::NodeRequirement("SetupSteps", YAML::NodeType::Sequence, false, true),
            Internal::NodeRequirement("ObjectFileExtensions", YAML::NodeType::Map, true, false),
            Internal::NodeRequirement("SharedLibraryExtensions", YAML::NodeType::Map, true, false),
            Internal::NodeRequirement("StaticLibraryExtensions", YAML::NodeType::Map, true, false),
            Internal::NodeRequirement("DebugSymbolFileExtensions", YAML::NodeType::Map, true, false),
            Internal::NodeRequirement("Compiler", YAML::NodeType::Map, true, false),
            Internal::NodeRequirement("Linker", YAML::NodeType::Map, true, false)
        };
        
        if(!Internal::CheckNodeRequirements(profileNode, requirements))
        {
            ssLOG_ERROR("Compiler profile: Failed to meet requirements");
            return false;
        }
        
        Name = profileNode["Name"].as<std::string>();
        
        for(int i = 0; i < profileNode["FileExtensions"].size(); ++i)
            FileExtensions.insert(profileNode["FileExtensions"][i].as<std::string>());
        
        if(profileNode["Languages"])
        {
            for(int i = 0; i < profileNode["Languages"].size(); ++i)
                Languages.insert(profileNode["Languages"][i].as<std::string>());
        }
        
        if(profileNode["SetupSteps"])
        {
            for(int i = 0; i < profileNode["SetupSteps"].size(); ++i)
                SetupSteps.push_back(profileNode["SetupSteps"][i].as<std::string>());
        }
        
        for(auto it = profileNode["ObjectFileExtensions"].begin();
            it != profileNode["ObjectFileExtensions"].end(); ++it)
        {
            ObjectFileExtensions[it->first.as<std::string>()] = it->second.as<std::string>();
        }
        
        for(auto it = profileNode["SharedLibraryExtensions"].begin();
            it != profileNode["SharedLibraryExtensions"].end(); ++it)
        {
            std::vector<std::string> currentExtensions;
            for(int i = 0; i < it->second.size(); ++i)
                currentExtensions.push_back(it->second[i].as<std::string>());
            
            SharedLibraryExtensions[it->first.as<std::string>()] = currentExtensions;
        }
        
        for(auto it = profileNode["StaticLibraryExtensions"].begin();
            it != profileNode["StaticLibraryExtensions"].end(); ++it)
        {
            std::vector<std::string> currentExtensions;
            for(int i = 0; i < it->second.size(); ++i)
                currentExtensions.push_back(it->second[i].as<std::string>());
            
            StaticLibraryExtensions[it->first.as<std::string>()] = currentExtensions;
        }
        
        for(auto it = profileNode["DebugSymbolFileExtensions"].begin();
            it != profileNode["DebugSymbolFileExtensions"].end(); ++it)
        {
            std::vector<std::string> currentExtensions;
            for(int i = 0; i < it->second.size(); ++i)
                currentExtensions.push_back(it->second[i].as<std::string>());
            
            DebugSymbolFileExtensions[it->first.as<std::string>()] = currentExtensions;
        }
        
        YAML::Node compilerNode = profileNode["Compiler"];
        if(!Compiler.ParseYAML_Node(compilerNode))
        {
            ssLOG_ERROR("Compiler profile: Compiler is invalid");
            return false;
        }
        
        YAML::Node linkerNode = profileNode["Linker"];
        if(!Linker.ParseYAML_Node(linkerNode))
        {
            ssLOG_ERROR("Compiler profile: Linker is invalid");
            return false;
        }
        
        return true;
        
        INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
    }
    
    std::string CompilerProfile::ToString(std::string indentation) const
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

}