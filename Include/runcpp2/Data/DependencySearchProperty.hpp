#ifndef RUNCPP2_DEPENDENCY_SEARCH_PROPERTY_HPP
#define RUNCPP2_DEPENDENCY_SEARCH_PROPERTY_HPP

#include "yaml-cpp/yaml.h"
#include <string>

namespace runcpp2
{
    class DependencySearchProperty
    {
        public:
            std::string SearchLibraryName;
            std::string SearchPath;
            
            bool ParseYAML_Node(YAML::Node& node);
            std::string ToString(std::string indentation) const;
    };
}

#endif