#include "runcpp2/Data/CompilerInfo.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "runcpp2/Data/ParseCommon.hpp"
#include "ssLogger/ssLog.hpp"

bool runcpp2::Data::CompilerInfo::ParseYAML_Node(YAML::Node& profileNode)
{
    INTERNAL_RUNCPP2_SAFE_START();
    
    std::vector<NodeRequirement> requirements = 
    {
        NodeRequirement("Executable", YAML::NodeType::Scalar, true, false),
        NodeRequirement("DefaultCompileFlags", YAML::NodeType::Scalar, true, true),
        NodeRequirement("CompileArgs", YAML::NodeType::Map, true, false)
    };
    
    std::vector<NodeRequirement> argsRequirements = 
    {
        NodeRequirement("CompilePart", YAML::NodeType::Scalar, true, false),
        NodeRequirement("IncludePart", YAML::NodeType::Scalar, true, false),
        NodeRequirement("InputPart", YAML::NodeType::Scalar, true, false),
        NodeRequirement("OutputPart", YAML::NodeType::Scalar, true, false)
    };
    
    if(!CheckNodeRequirements(profileNode, requirements))
    {
        ssLOG_ERROR("Compiler Info: Failed to meet requirements");
        return false;
    }
    
    Executable = profileNode["Executable"].as<std::string>();
    DefaultCompileFlags = profileNode["DefaultCompileFlags"].as<std::string>();
    
    YAML::Node compileArgsNode = profileNode["CompileArgs"];
    
    if(!CheckNodeRequirements(compileArgsNode, argsRequirements))
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

std::string runcpp2::Data::CompilerInfo::ToString(std::string indentation) const
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