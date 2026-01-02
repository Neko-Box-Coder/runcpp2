#ifndef RUNCPP2_DATA_DEPENDENCY_INFO_HPP
#define RUNCPP2_DATA_DEPENDENCY_INFO_HPP

#include "runcpp2/Data/DependencyLibraryType.hpp"
#include "runcpp2/Data/DependencySource.hpp"
#include "runcpp2/Data/DependencyLinkProperty.hpp"
#include "runcpp2/Data/ProfilesCommands.hpp"
#include "runcpp2/Data/ParseCommon.hpp"
#include "runcpp2/Data/FilesToCopyInfo.hpp"
#include "runcpp2/YamlLib.hpp"

#include <string>
#include <unordered_set>

namespace runcpp2
{
    namespace Data
    {
        struct DependencyInfo
        {
            std::string Name;
            std::unordered_set<PlatformName> Platforms;
            DependencySource Source;
            DependencyLibraryType LibraryType;
            std::vector<std::string> IncludePaths;
            std::vector<std::string> AbsoluteIncludePaths;
            std::unordered_map<PlatformName, DependencyLinkProperty> LinkProperties;
            std::unordered_map<PlatformName, ProfilesCommands> Setup;
            std::unordered_map<PlatformName, ProfilesCommands> Cleanup;
            std::unordered_map<PlatformName, ProfilesCommands> Build;
            std::unordered_map<PlatformName, FilesToCopyInfo> FilesToCopy;
            
            bool ParseYAML_Node(YAML::ConstNodePtr node);
            
            std::string ToString(std::string indentation) const;
            bool Equals(const DependencyInfo& other) const;
        };
    }
}

#endif
