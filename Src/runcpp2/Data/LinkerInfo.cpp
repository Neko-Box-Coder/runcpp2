#include "runcpp2/Data/LinkerInfo.hpp"

#include "runcpp2/Data/ParseCommon.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "ssLogger/ssLog.hpp"

bool runcpp2::Data::LinkerInfo::ParseYAML_Node(ryml::ConstNodeRef& node)
{
    INTERNAL_RUNCPP2_SAFE_START();
    
    std::vector<NodeRequirement> requirements = 
    {
        NodeRequirement("Executable", ryml::NodeType_e::KEYVAL, true, false),
        NodeRequirement("DefaultLinkFlags", ryml::NodeType_e::KEYVAL, true, true),
        NodeRequirement("LinkerArgs", ryml::NodeType_e::MAP, true, false)
    };
    
    std::vector<NodeRequirement> argsRequirements = 
    {
        NodeRequirement("OutputPart", ryml::NodeType_e::KEYVAL, true, false),
        NodeRequirement("DependenciesPart", ryml::NodeType_e::KEYVAL, true, false)
    };
    
    if(!CheckNodeRequirements(node, requirements))
    {
        ssLOG_ERROR("Linker Info: Failed to meet requirements");
        return false;
    }
    
    node["Executable"] >> Executable;
    node["DefaultLinkFlags"] >> DefaultLinkFlags;
    
    ryml::ConstNodeRef linkerArgsNode = node["LinkerArgs"];
    
    if(!CheckNodeRequirements(linkerArgsNode, argsRequirements))
    {
        ssLOG_ERROR("Linker Info: LinkerArgs failed to meet requirements");
        return false;
    }
    
    linkerArgsNode["OutputPart"] >> LinkerArgs.OutputPart;
    linkerArgsNode["DependenciesPart"] >> LinkerArgs.DependenciesPart;
    
    return true;
    
    INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
}

std::string runcpp2::Data::LinkerInfo::ToString(std::string indentation) const
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