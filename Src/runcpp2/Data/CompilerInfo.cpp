#include "runcpp2/Data/CompilerInfo.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "runcpp2/Data/ParseCommon.hpp"
#include "ssLogger/ssLog.hpp"

bool runcpp2::Data::CompilerInfo::ParseYAML_Node(ryml::ConstNodeRef& profileNode)
{
    INTERNAL_RUNCPP2_SAFE_START();
    
    std::vector<NodeRequirement> requirements = 
    {
        NodeRequirement("Executable", ryml::NodeType_e::KEYVAL, true, false),
        NodeRequirement("DefaultCompileFlags", ryml::NodeType_e::KEYVAL, true, true),
        NodeRequirement("CompileArgs", ryml::NodeType_e::MAP, true, false)
    };
    
    std::vector<NodeRequirement> argsRequirements = 
    {
        NodeRequirement("CompilePart", ryml::NodeType_e::KEYVAL, true, false),
        NodeRequirement("IncludePart", ryml::NodeType_e::KEYVAL, true, false),
        NodeRequirement("InputPart", ryml::NodeType_e::KEYVAL, true, false),
        NodeRequirement("OutputPart", ryml::NodeType_e::KEYVAL, true, false)
    };
    
    if(!CheckNodeRequirements(profileNode, requirements))
    {
        ssLOG_ERROR("Compiler Info: Failed to meet requirements");
        return false;
    }
    
    profileNode["Executable"] >> Executable;
    profileNode["DefaultCompileFlags"] >> DefaultCompileFlags;
    
    ryml::ConstNodeRef compileArgsNode = profileNode["CompileArgs"];
    
    if(!CheckNodeRequirements(compileArgsNode, argsRequirements))
    {
        ssLOG_ERROR("Compiler Info: CompileArgs failed to meet requirements");
        return false;
    }
    
    compileArgsNode["CompilePart"] >> CompileArgs.CompilePart;
    compileArgsNode["IncludePart"] >> CompileArgs.IncludePart;
    compileArgsNode["InputPart"] >> CompileArgs.InputPart;
    compileArgsNode["OutputPart"] >> CompileArgs.OutputPart;
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