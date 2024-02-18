#ifndef RUNCPP2_DEPENDENCY_INFO_HPP
#define RUNCPP2_DEPENDENCY_INFO_HPP

#include "runcpp2/Data/DependencyLibraryType.hpp"
#include "runcpp2/Data/DependencySource.hpp"
#include "runcpp2/Data/DependencySearchProperty.hpp"
#include "runcpp2/Data/DependencySetup.hpp"
#include "runcpp2/Data/ParseCommon.hpp"

#include <string>
#include <unordered_set>

namespace runcpp2
{
    class DependencyInfo
    {
        public:
            std::string Name;
            std::unordered_set<PlatformName> Platforms;
            DependencySource Source;
            DependencyLibraryType LibraryType;
            std::unordered_map<ProfileName, DependencySearchProperty> SearchProperties;
            std::unordered_map<PlatformName, DependencySetup> Setup;
            
            bool ParseYAML_Node(YAML::Node& node);
            std::string ToString(std::string indentation) const;
    };
}

#endif