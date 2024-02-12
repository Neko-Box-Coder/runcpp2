#ifndef RUNCPP2_SCRIPT_INFO_HPP
#define RUNCPP2_SCRIPT_INFO_HPP

#include "runcpp2/DependencyInfo.hpp"
#include "runcpp2/FlagsOverrideInfo.hpp"

#include <string>
#include <vector>

namespace runcpp2
{
    class ScriptInfo
    {
        public:
            std::string Language;
            std::vector<std::string> PreferredProfiles;
            std::vector<DependencyInfo> Dependencies;
            
            FlagsOverrideInfo OverrideCompileFlags;
            FlagsOverrideInfo OverrideLinkFlags;
            
            bool Populated = false;
            
            bool ParseYAML_Node(YAML::Node& node);
            std::string ToString(std::string indentation) const;
    };
}

#endif