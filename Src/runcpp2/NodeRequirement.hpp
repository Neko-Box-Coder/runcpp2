#ifndef RUNCPP2_NODE_REQUIREMENT_HPP
#define RUNCPP2_NODE_REQUIREMENT_HPP

#include "runcpp2/LibYAML_Wrapper.hpp"
#include <string>

namespace runcpp2
{
    struct NodeRequirement
    {
        std::string Name;
        YAML::NodeType NodeType;
        bool Required;
        bool Nullable;
        
        inline NodeRequirement() :  Name(""),
                                    NodeType(YAML::NodeType::Scalar),
                                    Required(false),
                                    Nullable(true)
        {}
        
        inline NodeRequirement( const std::string& name, 
                                YAML::NodeType nodeType, 
                                bool required,
                                bool nullable) :    Name(name), 
                                                                NodeType(nodeType),
                                                                Required(required), 
                                                                Nullable(nullable)
        {}
    };
}


#endif
