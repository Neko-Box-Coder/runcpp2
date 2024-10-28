#ifndef RUNCPP2_PARSE_UTIL_HPP
#define RUNCPP2_PARSE_UTIL_HPP

#include "ryml.hpp"

#include <vector>

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
    
    bool CheckNodeRequirements( ryml::ConstNodeRef& node, 
                                const std::vector<NodeRequirement>& requirements);

    bool GetParsableInfo(const std::string& contentToParse, std::string& outParsableInfo);
    
    bool ResolveYAML_Stream(ryml::Tree& rootTree, 
                            ryml::ConstNodeRef& outRootNode);
    
    bool ExistAndHasChild(  const ryml::ConstNodeRef& node, 
                            const std::string& childName,
                            bool nullable = false);
    
    std::string GetValue(ryml::ConstNodeRef node);
    std::string GetKey(ryml::ConstNodeRef node);
    std::string GetEscapedYAMLString(const std::string& input);
}

#endif
