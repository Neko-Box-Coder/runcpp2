#ifndef RUNCPP2_DATA_DEPENDENCY_SOURCE_HPP
#define RUNCPP2_DATA_DEPENDENCY_SOURCE_HPP

#include "runcpp2/Data/DependencySourceType.hpp"
#include "yaml-cpp/yaml.h"
#include <string>

namespace runcpp2
{
    class DependencySource
    {
        public:
            DependencySourceType Type;
            std::string Value;
            
            bool ParseYAML_Node(YAML::Node& node);
            std::string ToString(std::string indentation) const;
    };
}

#endif