#include "runcpp2/Data/StageInfo.hpp"
#include "runcpp2/ParseUtil.hpp"

#include "ssLogger/ssLog.hpp"

namespace
{
    bool ParseOutputTypes(  ryml::ConstNodeRef outputTypesSubNode, 
                            std::vector<runcpp2::NodeRequirement> requirements, 
                            std::unordered_map< PlatformName, 
                                                runcpp2::Data::StageInfo::OutputTypeInfo> outInfos)
    {
        ssLOG_FUNC_DEBUG();
        
        std::string subNodeName = runcpp2::GetKey(outputTypesSubNode);
        
        for(int i = 0; i < outputTypesSubNode.num_children(); ++i)
        {
            ryml::ConstNodeRef currentPlatformNode = outputTypesSubNode[i];
            std::string platformName = runcpp2::GetKey(currentPlatformNode);

            if(!CheckNodeRequirements(currentPlatformNode, requirements))
            {
                ssLOG_ERROR("Failed to parse outputTypesSubNode in " << subNodeName << 
                            " for platform " << platformName);
                
                return false;
            }

            outInfos[platformName].Flags = runcpp2::GetValue(currentPlatformNode["Flags"]);
            outInfos[platformName].Executable = runcpp2::GetValue(currentPlatformNode["Executable"]);
        }
        
        return true;
    }
}


bool runcpp2::Data::StageInfo::ParseYAML_Node(ryml::ConstNodeRef& node, std::string outputTypeKeyName)
{
    ssLOG_FUNC_DEBUG();
    
    INTERNAL_RUNCPP2_SAFE_START();
    std::vector<NodeRequirement> requirements =
    {
        NodeRequirement("PreRun", ryml::NodeType_e::MAP, false, true),
        NodeRequirement("CheckExistence", ryml::NodeType_e::KEYVAL, true, false),
        NodeRequirement(outputTypeKeyName, ryml::NodeType_e::MAP, true, false),
        NodeRequirement("RunParts", ryml::NodeType_e::MAP, true, false),
    };

    if(!CheckNodeRequirements(node, requirements))
    {
        ssLOG_ERROR("StageInfo: Failed to meet requirements");
        return false;
    }

    if(ExistAndHasChild(node, "PreRun"))
    {
        ryml::ConstNodeRef preRunNode = node["PreRun"];
        for(int i = 0; i < preRunNode.num_children(); ++i)
        {
            std::string key = GetKey(preRunNode[i]);
            std::string value = GetValue(preRunNode[i]);
            PreRun[key] = value;
        }
    }
    
    node["CheckExistence"] >> CheckExistence;
    
    //OutputTypes
    {
        if(!ExistAndHasChild(node, outputTypeKeyName))
        {
            ssLOG_ERROR("Failed to parse " << outputTypeKeyName);
            return false;
        }
        
        ryml::ConstNodeRef outputTypeNode = node[outputTypeKeyName.c_str()];
        std::vector<NodeRequirement> outputTypeRequirements =
        {
            NodeRequirement("Executable", ryml::NodeType_e::MAP, true, false),
            NodeRequirement("Static", ryml::NodeType_e::MAP, true, false),
            NodeRequirement("Shared", ryml::NodeType_e::MAP, true, false)
        };
        
        if(!CheckNodeRequirements(outputTypeNode, outputTypeRequirements))
        {
            ssLOG_ERROR("Failed to parse " << outputTypeKeyName);
            return false;
        }

        std::vector<NodeRequirement> outputTypeInfoRequirements =
        {
            NodeRequirement("Flags", ryml::NodeType_e::KEYVAL, true, false),
            NodeRequirement("Executable", ryml::NodeType_e::KEYVAL, true, false)
        };

        ryml::ConstNodeRef executableNode = outputTypeNode["Executable"];
        if(!ParseOutputTypes(executableNode, outputTypeInfoRequirements, OutputTypes.Executable))
            return false;
        
        ryml::ConstNodeRef staticNode = outputTypeNode["Static"];
        if(!ParseOutputTypes(staticNode, outputTypeInfoRequirements, OutputTypes.Static))
            return false;
        
        ryml::ConstNodeRef sharedNode = outputTypeNode["Shared"];
        if(!ParseOutputTypes(sharedNode, outputTypeInfoRequirements, OutputTypes.Shared))
            return false;
    }
    
    //RunParts
    {
        if(!ExistAndHasChild(node, "RunParts"))
        {
            ssLOG_ERROR("Failed to parse RunParts");
            return false;
        }
        
        ryml::ConstNodeRef runPartsNode = node["RunParts"];
        for(int i = 0; i < runPartsNode.num_children(); ++i)
        {
            ryml::ConstNodeRef currentPlatformNode = runPartsNode[i];
            std::string platformName = GetKey(currentPlatformNode);
            if(!currentPlatformNode.is_seq())
            {
                ssLOG_ERROR("Failed to parse " << platformName << " in RunParts");
                return false;
            }
            
            for(int j = 0; j < currentPlatformNode.num_children(); ++j)
            {
                ryml::ConstNodeRef currentPartNode = currentPlatformNode[j];
                
                std::vector<NodeRequirement> currentRunPartRequirements =
                {
                    NodeRequirement("Type", ryml::NodeType_e::KEYVAL, true, false),
                    NodeRequirement("CommandPart", ryml::NodeType_e::KEYVAL, true, false)
                };
                
                if(!CheckNodeRequirements(currentPartNode, currentRunPartRequirements))
                {
                    ssLOG_ERROR("Failed to parse RunPart at index " << j << " for " << platformName);
                    return false;
                }
                
                RunParts[platformName].push_back({});
                std::string currentType = GetValue(currentPartNode["Type"]);
                
                if(currentType == "Once")
                    RunParts[platformName].back().Type = RunPart::RunType::ONCE;
                else if(currentType == "Repeats")
                    RunParts[platformName].back().Type = RunPart::RunType::REPEATS;
                else
                {
                    ssLOG_WARNING(  "Invalid RunPart type " << currentType << " at index " << j << 
                                    " for " << platformName);
                    ssLOG_INFO("Defaulting to Once");
                    RunParts[platformName].back().Type = RunPart::RunType::ONCE;
                }
                
                RunParts[platformName].back().CommandPart = GetValue(currentPartNode["CommandPart"]);
            }
        }
    }
    
    return true;


    INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
}

std::string runcpp2::Data::StageInfo::ToString(std::string indentation) const
{
    return "";
}
