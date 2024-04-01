#include "runcpp2/Data/LinkerInfo.hpp"

#include "runcpp2/Data/ParseCommon.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "ssLogger/ssLog.hpp"

bool runcpp2::Data::LinkerInfo::ParseYAML_Node(ryml::ConstNodeRef& node)
{
    INTERNAL_RUNCPP2_SAFE_START();
    
    std::vector<NodeRequirement> requirements = 
    {
        NodeRequirement("EnvironmentSetup", ryml::NodeType_e::MAP, false, true),
        NodeRequirement("Executable", ryml::NodeType_e::KEYVAL, true, false),
        NodeRequirement("DefaultLinkFlags", ryml::NodeType_e::KEYVAL, true, true),
        NodeRequirement("ExecutableLinkFlags", ryml::NodeType_e::KEYVAL, true, true),
        NodeRequirement("StaticLibLinkFlags", ryml::NodeType_e::KEYVAL, true, true),
        NodeRequirement("SharedLibLinkFlags", ryml::NodeType_e::KEYVAL, true, true),
        NodeRequirement("LinkArgs", ryml::NodeType_e::MAP, true, false)
    };
    
    std::vector<NodeRequirement> linkArgsRequirements = 
    {
        NodeRequirement("OutputPart", ryml::NodeType_e::KEYVAL, true, false),
        NodeRequirement("LinkPart", ryml::NodeType_e::KEYVAL, true, false)
    };
    
    if(!CheckNodeRequirements(node, requirements))
    {
        ssLOG_ERROR("Linker Info: Failed to meet requirements");
        return false;
    }
    
    if(node.has_child("EnvironmentSetup"))
    {
        for(int i = 0; i < node["EnvironmentSetup"].num_children(); ++i)
        {
            ryml::ConstNodeRef currentPlatform = node["EnvironmentSetup"][i];
            
            if(!currentPlatform.is_keyval())
            {
                ssLOG_ERROR("Linker Info: EnvironmentSetup should be a map of keyvals");
                return false;
            }
            
            std::string key = GetKey(currentPlatform);
            EnvironmentSetup[key] = GetValue(currentPlatform);
        }
    }
    
    node["Executable"] >> Executable;
    
    for(int i = 0; i < node["DefaultLinkFlags"].num_children(); ++i)
    {
        ryml::ConstNodeRef currentPlatform = node["DefaultLinkFlags"][i];
        
        if(!currentPlatform.is_keyval())
        {
            ssLOG_ERROR("Linker Info: DefaultLinkFlags should be a map of keyvals");
            return false;
        }
        
        std::string key = GetKey(currentPlatform);
        DefaultLinkFlags[key] = GetValue(currentPlatform);
    }
    
    for(int i = 0; i < node["ExecutableLinkFlags"].num_children(); ++i)
    {
        ryml::ConstNodeRef currentPlatform = node["ExecutableLinkFlags"][i];
        
        if(!currentPlatform.is_keyval())
        {
            ssLOG_ERROR("Linker Info: ExecutableLinkFlags should be a map of keyvals");
            return false;
        }
        
        std::string key = GetKey(currentPlatform);
        ExecutableLinkFlags[key] = GetValue(currentPlatform);
    }
    
    for(int i = 0; i < node["StaticLibLinkFlags"].num_children(); ++i)
    {
        ryml::ConstNodeRef currentPlatform = node["StaticLibLinkFlags"][i];
        
        if(!currentPlatform.is_keyval())
        {
            ssLOG_ERROR("Linker Info: StaticLibLinkFlags should be a map of keyvals");
            return false;
        }
        
        std::string key = GetKey(currentPlatform);
        StaticLibLinkFlags[key] = GetValue(currentPlatform);
    }
    
    for(int i = 0; i < node["SharedLibLinkFlags"].num_children(); ++i)
    {
        ryml::ConstNodeRef currentPlatform = node["SharedLibLinkFlags"][i];
        
        if(!currentPlatform.is_keyval())
        {
            ssLOG_ERROR("Linker Info: SharedLibLinkFlags should be a map of keyvals");
            return false;
        }
        
        std::string key = GetKey(currentPlatform);
        SharedLibLinkFlags[key] = GetValue(currentPlatform);
    }
    
    ryml::ConstNodeRef linkerArgsNode = node["LinkArgs"];
    
    if(!CheckNodeRequirements(linkerArgsNode, linkArgsRequirements))
    {
        ssLOG_ERROR("Linker Info: LinkArgs failed to meet requirements");
        return false;
    }
    
    linkerArgsNode["OutputPart"] >> LinkArgs.OutputPart;
    linkerArgsNode["LinkPart"] >> LinkArgs.LinkPart;
    
    return true;
    
    INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
}

std::string runcpp2::Data::LinkerInfo::ToString(std::string indentation) const
{
    std::string out;
    
    out += indentation + "LinkerInfo:\n";
    
    if(EnvironmentSetup.size() != 0)
    {
        out += indentation + "    EnvironmentSetup:\n";
        for(auto it = EnvironmentSetup.begin(); it != EnvironmentSetup.end(); ++it)
            out += indentation + "        " + it->first + ": " + it->second + "\n";
    }
    
    out += indentation + "    Executable: " + Executable + "\n";
    out += indentation + "    DefaultLinkFlags: \n";
    for(auto it = DefaultLinkFlags.begin(); it != DefaultLinkFlags.end(); ++it)
        out += indentation + "        " + it->first + ": " + it->second + "\n";
    
    out += indentation + "    ExecutableLinkFlags: \n";
    for(auto it = ExecutableLinkFlags.begin(); it != ExecutableLinkFlags.end(); ++it)
        out += indentation + "        " + it->first + ": " + it->second + "\n";
    
    out += indentation + "    StaticLibLinkFlags: \n";
    for(auto it = StaticLibLinkFlags.begin(); it != StaticLibLinkFlags.end(); ++it)
        out += indentation + "        " + it->first + ": " + it->second + "\n";
    
    out += indentation + "    SharedLibLinkFlags: \n";
    for(auto it = SharedLibLinkFlags.begin(); it != SharedLibLinkFlags.end(); ++it)
        out += indentation + "        " + it->first + ": " + it->second + "\n";
    
    out += indentation + "    LinkerArgs:\n";
    out += indentation + "        OutputPart: " + LinkArgs.OutputPart + "\n";
    out += indentation + "        LinkPart: " + LinkArgs.LinkPart + "\n";
    
    return out;
}