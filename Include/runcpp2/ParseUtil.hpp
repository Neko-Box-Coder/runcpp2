#ifndef RUNCPP2_PARSE_UTIL_HPP
#define RUNCPP2_PARSE_UTIL_HPP

#include "runcpp2/YamlLib.hpp"
#include "runcpp2/Data/ParseCommon.hpp"
#include "ssLogger/ssLog.hpp"

#include <vector>
#include <unordered_map>

namespace runcpp2
{
    struct NodeRequirement
    {
        std::string Name;
        YAML::NodeType NodeType_LibYaml;
        bool Required;
        bool Nullable;
        
        NodeRequirement();
        NodeRequirement(const std::string& name, 
                        YAML::NodeType nodeType, 
                        bool required,
                        bool nullable);
    };
    
    bool CheckNodeRequirement_LibYaml(  YAML::ConstNodePtr parentNode, 
                                        const std::string childName, 
                                        YAML::NodeType childType,
                                        bool required,
                                        bool nullable);
    
    bool CheckNodeRequirements_LibYaml( YAML::ConstNodePtr node, 
                                        const std::vector<NodeRequirement>& requirements);


    bool GetParsableInfo(const std::string& contentToParse, std::string& outParsableInfo);
    
    bool MergeYAML_NodeChildren_LibYaml(YAML::NodePtr nodeToMergeFrom, 
                                        YAML::NodePtr nodeToMergeTo,
                                        YAML::ResourceHandle& yamlResouce);
    
    bool ExistAndHasChild_LibYaml(  YAML::ConstNodePtr node, 
                                    const std::string& childName,
                                    bool nullable = false);
    
    
    std::string GetEscapedYAMLString(const std::string& input);
    
    bool ParseIncludes(const std::string& line, std::string& outIncludePath);

    template<typename T>
    bool ParsePlatformProfileMap_LibYaml(   YAML::ConstNodePtr node, 
                                            const std::string& key,
                                            std::unordered_map<PlatformName, T>& result,
                                            const std::string& errorMessage)
    {
        if(ExistAndHasChild_LibYaml(node, key))
        {
            YAML::ConstNodePtr mapNode = node->GetMapValueNode(key);
            
            //If we skip platform profile
            T defaultValue;
            if(defaultValue.IsYAML_NodeParsableAsDefault_LibYaml(mapNode))
            {
                if(defaultValue.ParseYAML_NodeWithProfile_LibYaml(mapNode, "DefaultProfile"))
                    result["DefaultPlatform"] = defaultValue;
                else
                    return false;
            }
            else
            {
                if(!CheckNodeRequirement_LibYaml(node, key, YAML::NodeType::Map, false, true))
                    return false;
                
                for(int i = 0; i < mapNode->GetChildrenCount(); ++i)
                {
                    PlatformName platform = 
                        mapNode ->GetMapKeyScalarAt<std::string>(i)
                                .DS_TRY_ACT(ssLOG_ERROR(DS_TMP_ERROR.ToString()); return false);
                    T value;
                    YAML::ConstNodePtr currentNode = mapNode->GetMapValueNodeAt(i);
                    if(!value.ParseYAML_Node(currentNode))
                    {
                        ssLOG_ERROR("ScriptInfo: Failed to parse " << errorMessage);
                        return false;
                    }
                    result[platform] = value;
                }
            }
        }
        return true;
    }
}

#endif
