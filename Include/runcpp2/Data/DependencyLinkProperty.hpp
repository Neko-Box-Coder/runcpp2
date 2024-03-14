#ifndef RUNCPP2_DATA_DEPENDENCY_LINK_PROPERTY_HPP
#define RUNCPP2_DATA_DEPENDENCY_LINK_PROPERTY_HPP

#include "yaml-cpp/yaml.h"
#include <string>

namespace runcpp2
{
    class DependencyLinkProperty
    {
        public:
            std::vector<std::string> SearchLibraryNames;
            std::vector<std::string> SearchDirectories;
            
            bool ParseYAML_Node(YAML::Node& node);
            std::string ToString(std::string indentation) const;
    };
}

#endif