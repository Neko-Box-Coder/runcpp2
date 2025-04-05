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
        ryml::NodeType NodeType;
        bool Required;
        bool Nullable;
        
        NodeRequirement();
        NodeRequirement(const std::string& name, 
                        ryml::NodeType nodeType, 
                        bool required,
                        bool nullable);
    };
    
    bool CheckNodeRequirement(  ryml::ConstNodeRef node, 
                                const std::string name, 
                                ryml::NodeType nodeType,
                                bool required,
                                bool nullable);
    
    bool CheckNodeRequirements( ryml::ConstNodeRef node, 
                                const std::vector<NodeRequirement>& requirements);

    bool GetParsableInfo(const std::string& contentToParse, std::string& outParsableInfo);
    
    bool ResolveYAML_Stream(ryml::Tree& rootTree, 
                            ryml::ConstNodeRef& outRootNode);
    
    bool ExistAndHasChild(  ryml::ConstNodeRef node, 
                            const std::string& childName,
                            bool nullable = false);
    
    std::string GetValue(ryml::ConstNodeRef node);
    std::string GetKey(ryml::ConstNodeRef node);
    std::string GetEscapedYAMLString(const std::string& input);
    
    bool ParseIncludes(const std::string& line, std::string& outIncludePath);

    template<typename T>
    bool ParsePlatformProfileMap(   ryml::ConstNodeRef node, 
                                    const std::string& key,
                                    std::unordered_map<PlatformName, T>& result,
                                    const std::string& errorMessage)
    {
        if(ExistAndHasChild(node, key))
        {
            ryml::ConstNodeRef mapNode = node[key.c_str()];
            
            //If we skip platform profile
            T defaultValue;
            if(defaultValue.IsYAML_NodeParsableAsDefault(mapNode))
            {
                if(defaultValue.ParseYAML_NodeWithProfile(mapNode, "DefaultProfile"))
                    result["DefaultPlatform"] = defaultValue;
                else
                    return false;
            }
            else
            {
                if(!CheckNodeRequirement(mapNode, key, ryml::NodeType_e::MAP, false, true))
                    return false;
                
                for(int i = 0; i < mapNode.num_children(); ++i)
                {
                    PlatformName platform = GetKey(mapNode[i]);
                    T value;
                    ryml::ConstNodeRef currentNode = mapNode[i];
                    
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
