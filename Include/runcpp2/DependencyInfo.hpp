#ifndef RUNCPP2_DEPENDENCY_INFO_HPP
#define RUNCPP2_DEPENDENCY_INFO_HPP

#include "runcpp2/DependencyLibraryType.hpp"
#include "runcpp2/DependencySource.hpp"
#include <string>

namespace runcpp2
{
    class DependencyInfo
    {
        public:
            std::string Name;
            DependencyLibraryType LibraryType;
            std::string SearchLibraryName;
            DependencySource Source;
            std::unordered_map<std::string, std::vector<std::string>> Setup;
            
            bool ParseYAML_Node(YAML::Node& node);
            std::string ToString(std::string indentation) const;
    };
}

#endif