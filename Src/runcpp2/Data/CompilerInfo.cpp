#include "runcpp2/Data/CompilerInfo.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "runcpp2/Data/ParseCommon.hpp"
#include "ssLogger/ssLog.hpp"

bool runcpp2::Data::CompilerInfo::ParseYAML_Node(ryml::ConstNodeRef& node)
{
    INTERNAL_RUNCPP2_SAFE_START();
    
    std::vector<NodeRequirement> requirements = 
    {
        NodeRequirement("EnvironmentSetup", ryml::NodeType_e::MAP, false, true),
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
    
    if(!CheckNodeRequirements(node, requirements))
    {
        ssLOG_ERROR("Compiler Info: Failed to meet requirements");
        return false;
    }
    
    if(node.has_child("EnvironmentSetup"))
    {
        for(int i = 0; i < node["EnvironmentSetup"].num_children(); ++i)
        {
            ryml::ConstNodeRef currentPlatform = node["EnvironmentSetup"][i];
            
            if(!currentPlatform.is_keyval())
            {
                ssLOG_ERROR("Compiler Info: EnvironmentSetup should be a keyval");
                return false;
            }
            
            std::string key = GetKey(currentPlatform);
            EnvironmentSetup[key] = GetValue(currentPlatform);
        }
    }
    
    node["Executable"] >> Executable;
    node["DefaultCompileFlags"] >> DefaultCompileFlags;
    
    ryml::ConstNodeRef compileArgsNode = node["CompileArgs"];
    
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
    
    if(EnvironmentSetup.size() != 0)
    {
        out += indentation + "    EnvironmentSetup:\n";
        for(auto it = EnvironmentSetup.begin(); it != EnvironmentSetup.end(); ++it)
            out += indentation + "        " + it->first + ": " + it->second + "\n";
    }
    
    out += indentation + "    Executable: " + Executable + "\n";
    out += indentation + "    DefaultCompileFlags: " + DefaultCompileFlags + "\n";
    out += indentation + "    CompileArgs: \n";
    out += indentation + "        CompilePart: " + CompileArgs.CompilePart + "\n";
    out += indentation + "        IncludePart: " + CompileArgs.IncludePart + "\n";
    out += indentation + "        InputPart: " + CompileArgs.InputPart + "\n";
    out += indentation + "        OutputPart: " + CompileArgs.OutputPart + "\n";
    return out;
}