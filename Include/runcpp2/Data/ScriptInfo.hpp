#ifndef RUNCPP2_DATA_SCRIPT_INFO_HPP
#define RUNCPP2_DATA_SCRIPT_INFO_HPP

#include "runcpp2/Data/DependencyInfo.hpp"
#include "runcpp2/Data/FlagsOverrideInfo.hpp"
#include "runcpp2/Data/ParseCommon.hpp"

#include <string>
#include <vector>
#include <unordered_map>

namespace runcpp2
{
    namespace Data
    {
        class ScriptInfo
        {
            public:
                std::string Language;
                std::unordered_map<PlatformName, std::vector<ProfileName>> RequiredProfiles;
                
                std::unordered_map<ProfileName, FlagsOverrideInfo> OverrideCompileFlags;
                std::unordered_map<ProfileName, FlagsOverrideInfo> OverrideLinkFlags;
                
                std::vector<DependencyInfo> Dependencies;
                
                
                bool Populated = false;
                
                bool ParseYAML_Node(YAML::Node& node);
                std::string ToString(std::string indentation) const;
        };
    }
}

#endif