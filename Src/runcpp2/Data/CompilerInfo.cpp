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
        NodeRequirement("CheckExistence", ryml::NodeType_e::KEYVAL, true, false),
        NodeRequirement("DefaultCompileFlags", ryml::NodeType_e::MAP, true, true),
        NodeRequirement("ExecutableCompileFlags", ryml::NodeType_e::MAP, true, true),
        NodeRequirement("StaticLibCompileFlags", ryml::NodeType_e::MAP, true, true),
        NodeRequirement("SharedLibCompileFlags", ryml::NodeType_e::MAP, true, true),
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
                ssLOG_ERROR("Compiler Info: EnvironmentSetup should be a map of keyvals");
                return false;
            }
            
            std::string key = GetKey(currentPlatform);
            EnvironmentSetup[key] = GetValue(currentPlatform);
        }
    }
    
    node["Executable"] >> Executable;
    node["CheckExistence"] >> CheckExistence;
    
    for(int i = 0; i < node["DefaultCompileFlags"].num_children(); ++i)
    {
        ryml::ConstNodeRef currentPlatform = node["DefaultCompileFlags"][i];
        
        if(!currentPlatform.is_keyval())
        {
            ssLOG_ERROR("Compiler Info: DefaultCompileFlags should be a map of keyvals");
            return false;
        }
        
        std::string key = GetKey(currentPlatform);
        DefaultCompileFlags[key] = GetValue(currentPlatform);
    }
    
    for(int i = 0; i < node["ExecutableCompileFlags"].num_children(); ++i)
    {
        ryml::ConstNodeRef currentPlatform = node["ExecutableCompileFlags"][i];
        
        if(!currentPlatform.is_keyval())
        {
            ssLOG_ERROR("Compiler Info: ExecutableCompileFlags should be a map of keyvals");
            return false;
        }
        
        std::string key = GetKey(currentPlatform);
        ExecutableCompileFlags[key] = GetValue(currentPlatform);
    }
    
    for(int i = 0; i < node["StaticLibCompileFlags"].num_children(); ++i)
    {
        ryml::ConstNodeRef currentPlatform = node["StaticLibCompileFlags"][i];
        
        if(!currentPlatform.is_keyval())
        {
            ssLOG_ERROR("Compiler Info: StaticLibCompileFlags should be a map of keyvals");
            return false;
        }
        
        std::string key = GetKey(currentPlatform);
        StaticLibCompileFlags[key] = GetValue(currentPlatform);
    }
    
    for(int i = 0; i < node["SharedLibCompileFlags"].num_children(); ++i)
    {
        ryml::ConstNodeRef currentPlatform = node["SharedLibCompileFlags"][i];
        
        if(!currentPlatform.is_keyval())
        {
            ssLOG_ERROR("Compiler Info: SharedLibCompileFlags should be a map of keyvals");
            return false;
        }
        
        std::string key = GetKey(currentPlatform);
        SharedLibCompileFlags[key] = GetValue(currentPlatform);
    }
    
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
    out += indentation + "    CheckExistence: " + CheckExistence + "\n";
    out += indentation + "    DefaultCompileFlags: \n";
    for(auto it = DefaultCompileFlags.begin(); it != DefaultCompileFlags.end(); ++it)
        out += indentation + "        " + it->first + ": " + it->second + "\n";
    
    out += indentation + "    ExecutableCompileFlags: \n";
    for(auto it = ExecutableCompileFlags.begin(); it != ExecutableCompileFlags.end(); ++it)
        out += indentation + "        " + it->first + ": " + it->second + "\n";
    
    out += indentation + "    StaticLibCompileFlags: \n";
    for(auto it = StaticLibCompileFlags.begin(); it != StaticLibCompileFlags.end(); ++it)
        out += indentation + "        " + it->first + ": " + it->second + "\n";
    
    out += indentation + "    SharedLibCompileFlags: \n";
    for(auto it = SharedLibCompileFlags.begin(); it != SharedLibCompileFlags.end(); ++it)
        out += indentation + "        " + it->first + ": " + it->second + "\n";
    
    out += indentation + "    CompileArgs: \n";
    out += indentation + "        CompilePart: " + CompileArgs.CompilePart + "\n";
    out += indentation + "        IncludePart: " + CompileArgs.IncludePart + "\n";
    out += indentation + "        InputPart: " + CompileArgs.InputPart + "\n";
    out += indentation + "        OutputPart: " + CompileArgs.OutputPart + "\n";
    out += "\n";
    return out;
}

bool runcpp2::Data::CompilerInfo::Equals(const CompilerInfo& other) const
{
    if( Executable != other.Executable || 
        CheckExistence != other.CheckExistence ||
        EnvironmentSetup.size() != other.EnvironmentSetup.size() ||
        DefaultCompileFlags.size() != other.DefaultCompileFlags.size() ||
        ExecutableCompileFlags.size() != other.ExecutableCompileFlags.size() ||
        StaticLibCompileFlags.size() != other.StaticLibCompileFlags.size() ||
        SharedLibCompileFlags.size() != other.SharedLibCompileFlags.size())
    {
        return false;
    }

    for(const auto& it : EnvironmentSetup)
    {
        if( other.EnvironmentSetup.count(it.first) == 0 || 
            other.EnvironmentSetup.at(it.first) != it.second)
        {
            return false;
        }
    }

    for(const auto& it : DefaultCompileFlags)
    {
        if( other.DefaultCompileFlags.count(it.first) == 0 || 
            other.DefaultCompileFlags.at(it.first) != it.second)
        {
            return false;
        }
    }

    for(const auto& it : ExecutableCompileFlags)
    {
        if( other.ExecutableCompileFlags.count(it.first) == 0 || 
            other.ExecutableCompileFlags.at(it.first) != it.second)
        {
            return false;
        }
    }

    for(const auto& it : StaticLibCompileFlags)
    {
        if( other.StaticLibCompileFlags.count(it.first) == 0 || 
            other.StaticLibCompileFlags.at(it.first) != it.second)
        {
            return false;
        }
    }

    for(const auto& it : SharedLibCompileFlags)
    {
        if( other.SharedLibCompileFlags.count(it.first) == 0 || 
            other.SharedLibCompileFlags.at(it.first) != it.second)
        {
            return false;
        }
    }

    return  CompileArgs.CompilePart == other.CompileArgs.CompilePart &&
            CompileArgs.IncludePart == other.CompileArgs.IncludePart &&
            CompileArgs.InputPart == other.CompileArgs.InputPart &&
            CompileArgs.OutputPart == other.CompileArgs.OutputPart;
}
