#ifndef RUNCPP2_DATA_DEPENDENCY_INFO_HPP
#define RUNCPP2_DATA_DEPENDENCY_INFO_HPP

#include "runcpp2/Data/DependencyLibraryType.hpp"
#include "runcpp2/Data/DependencySource.hpp"
#include "runcpp2/Data/DependencyLinkProperty.hpp"
#include "runcpp2/Data/DependencyCommands.hpp"
#include "runcpp2/Data/ParseCommon.hpp"
#include "runcpp2/Data/FilesToCopyInfo.hpp"
#include "ryml.hpp"

#include <string>
#include <unordered_set>

namespace runcpp2
{
    namespace Data
    {
        class DependencyInfo
        {
            public:
                std::string Name;
                std::unordered_set<PlatformName> Platforms;
                DependencySource Source;
                DependencyLibraryType LibraryType;
                std::vector<std::string> IncludePaths;
                std::vector<std::string> AbsoluteIncludePaths;
                std::unordered_map<ProfileName, DependencyLinkProperty> LinkProperties;
                std::unordered_map<PlatformName, DependencyCommands> Setup;
                std::unordered_map<PlatformName, DependencyCommands> Cleanup;
                std::unordered_map<PlatformName, DependencyCommands> Build;
                std::unordered_map<PlatformName, FilesToCopyInfo> FilesToCopy;
                
                bool ParseYAML_Node(ryml::ConstNodeRef& node);
                std::string ToString(std::string indentation) const;
        };
    }
}

#endif
