#ifndef RUNCPP2_DATA_DEPENDENCY_LINK_PROPERTY_HPP
#define RUNCPP2_DATA_DEPENDENCY_LINK_PROPERTY_HPP

#include "runcpp2/Data/ParseCommon.hpp"

#include "ryml.hpp"

#include <vector>
#include <unordered_map>
#include <string>

namespace runcpp2
{
    namespace Data
    {
        class DependencyLinkProperty
        {
            public:
                std::vector<std::string> SearchLibraryNames;
                std::vector<std::string> SearchDirectories;
                std::vector<std::string> ExcludeLibraryNames;
                std::unordered_map<PlatformName, std::vector<std::string>> AdditionalLinkOptions;
                
                bool ParseYAML_Node(ryml::ConstNodeRef& node);
                std::string ToString(std::string indentation) const;
        };
    }
}

#endif