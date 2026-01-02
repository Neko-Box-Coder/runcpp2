#ifndef RUNCPP2_DATA_PROFILES_FLAGS_OVERRIDE_HPP
#define RUNCPP2_DATA_PROFILES_FLAGS_OVERRIDE_HPP

#include "FlagsOverrideInfo.hpp"
#include "runcpp2/Data/ParseCommon.hpp"

#include <unordered_map>

namespace runcpp2
{
    namespace Data
    {
        class ProfilesFlagsOverride
        {
            public:
                std::unordered_map<ProfileName, FlagsOverrideInfo> FlagsOverrides;
                
                bool ParseYAML_Node(YAML::ConstNodePtr node);
                
                bool ParseYAML_NodeWithProfile_LibYaml(YAML::ConstNodePtr node, ProfileName profile);
                bool IsYAML_NodeParsableAsDefault_LibYaml(YAML::ConstNodePtr node) const;
                
                std::string ToString(std::string indentation) const;
                bool Equals(const ProfilesFlagsOverride& other) const;
        };
    }
}

#endif
