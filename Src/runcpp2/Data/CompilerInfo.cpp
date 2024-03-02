#include "runcpp2/Data/CompilerInfo.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "runcpp2/Data/ParseCommon.hpp"
#include "ssLogger/ssLog.hpp"

namespace runcpp2
{
    bool CompilerInfo::ParseYAML_Node(YAML::Node& profileNode)
    {
        INTERNAL_RUNCPP2_SAFE_START();
        
        std::vector<Internal::NodeRequirement> requirements = 
        {
            Internal::NodeRequirement("Executable", YAML::NodeType::Scalar, true, false),
            Internal::NodeRequirement("DefaultCompileFlags", YAML::NodeType::Scalar, true, true),
            Internal::NodeRequirement("CompileArgs", YAML::NodeType::Map, true, false)
        };
        
        std::vector<Internal::NodeRequirement> argsRequirements = 
        {
            Internal::NodeRequirement("CompilePart", YAML::NodeType::Scalar, true, false),
            Internal::NodeRequirement("IncludePart", YAML::NodeType::Scalar, true, false),
            Internal::NodeRequirement("InputPart", YAML::NodeType::Scalar, true, false),
            Internal::NodeRequirement("OutputPart", YAML::NodeType::Scalar, true, false)
        };
        
        if(!Internal::CheckNodeRequirements(profileNode, requirements))
        {
            ssLOG_ERROR("Compiler Info: Failed to meet requirements");
            return false;
        }
        
        Executable = profileNode["Executable"].as<std::string>();
        DefaultCompileFlags = profileNode["DefaultCompileFlags"].as<std::string>();
        
        YAML::Node compileArgsNode = profileNode["CompileArgs"];
        
        if(!Internal::CheckNodeRequirements(compileArgsNode, argsRequirements))
        {
            ssLOG_ERROR("Compiler Info: CompileArgs failed to meet requirements");
            return false;
        }
        
        CompileArgs.CompilePart = compileArgsNode["CompilePart"].as<std::string>();
        CompileArgs.IncludePart = compileArgsNode["IncludePart"].as<std::string>();
        CompileArgs.InputPart = compileArgsNode["InputPart"].as<std::string>();
        CompileArgs.OutputPart = compileArgsNode["OutputPart"].as<std::string>();
        return true;
        
        INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
    }
    
    std::string CompilerInfo::ToString(std::string indentation) const
    {
        std::string out;
        
        out += indentation + "CompilerInfo:\n";
        out += indentation + "    Executable: " + Executable + "\n";
        out += indentation + "    DefaultCompileFlags: " + DefaultCompileFlags + "\n";
        out += indentation + "    CompileArgs: \n";
        out += indentation + "        CompilePart: " + CompileArgs.CompilePart + "\n";
        out += indentation + "        IncludePart: " + CompileArgs.IncludePart + "\n";
        out += indentation + "        InputPart: " + CompileArgs.InputPart + "\n";
        out += indentation + "        OutputPart: " + CompileArgs.OutputPart + "\n";
        return out;
    }

}