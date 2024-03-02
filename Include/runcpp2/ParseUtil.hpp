#ifndef RUNCPP2_PARSE_UTIL_HPP
#define RUNCPP2_PARSE_UTIL_HPP

#include "yaml-cpp/yaml.h"

namespace runcpp2
{

//TODO: Use rapidyaml instead

namespace Internal
{
    struct NodeRequirement
    {
        std::string Name;
        YAML::NodeType::value NodeType;
        bool Required;
        bool Nullable;
        
        NodeRequirement();
        NodeRequirement(const std::string& name, 
                        YAML::NodeType::value nodeType, 
                        bool required,
                        bool nullable);
    };
    
    bool CheckNodeRequirements(YAML::Node& node, const std::vector<NodeRequirement>& requirements);
    
    bool GetParsableInfo(const std::string& contentToParse, std::string& outParsableInfo);
}

}

#endif