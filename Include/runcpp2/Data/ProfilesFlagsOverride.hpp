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
                
                bool ParseYAML_Node(ryml::ConstNodeRef node);
                bool ParseYAML_NodeWithProfile(ryml::ConstNodeRef node, ProfileName profile);
                bool IsYAML_NodeParsableAsDefault(ryml::ConstNodeRef node) const;
                std::string ToString(std::string indentation) const;
                bool Equals(const ProfilesFlagsOverride& other) const;
        };
    }
}

#endif
