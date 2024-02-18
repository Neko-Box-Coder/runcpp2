#include "runcpp2/Data/LinkerInfo.hpp"

#include "runcpp2/Data/ParseCommon.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "ssLogger/ssLog.hpp"

namespace runcpp2
{
    bool LinkerInfo::ParseYAML_Node(YAML::Node& profileNode)
    {
        INTERNAL_RUNCPP2_SAFE_START();
        
        std::vector<Internal::NodeRequirement> requirements = 
        {
            Internal::NodeRequirement("Executable", YAML::NodeType::Scalar, true, false),
            Internal::NodeRequirement("DefaultLinkFlags", YAML::NodeType::Scalar, true, true),
            Internal::NodeRequirement("LinkerArgs", YAML::NodeType::Map, true, false)
        };
        
        std::vector<Internal::NodeRequirement> argsRequirements = 
        {
            Internal::NodeRequirement("OutputPart", YAML::NodeType::Scalar, true, false),
            Internal::NodeRequirement("DependenciesPart", YAML::NodeType::Scalar, true, false)
        };
        
        if(!Internal::CheckNodeRequirements(profileNode, requirements))
        {
            ssLOG_ERROR("Linker Info: Failed to meet requirements");
            return false;
        }
        
        Executable = profileNode["Executable"].as<std::string>();
        DefaultLinkFlags = profileNode["DefaultLinkFlags"].as<std::string>();
        
        YAML::Node linkerArgsNode = profileNode["LinkerArgs"];
        
        if(!Internal::CheckNodeRequirements(linkerArgsNode, argsRequirements))
        {
            ssLOG_ERROR("Linker Info: LinkerArgs failed to meet requirements");
            return false;
        }
        
        LinkerArgs.OutputPart = linkerArgsNode["OutputPart"].as<std::string>();
        LinkerArgs.DependenciesPart = linkerArgsNode["DependenciesPart"].as<std::string>();
        
        return true;
        
        INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
    }
    
    std::string LinkerInfo::ToString(std::string indentation) const
    {
        std::string out;
        
        out += indentation + "LinkerInfo:\n";
        out += indentation + "    Executable: " + Executable + "\n";
        out += indentation + "    DefaultLinkFlags: " + DefaultLinkFlags + "\n";
        out += indentation + "    LinkerArgs:\n";
        out += indentation + "        OutputPart: " + LinkerArgs.OutputPart + "\n";
        out += indentation + "        DependenciesPart: " + LinkerArgs.DependenciesPart + "\n";
        
        return out;
    }

}