#ifndef RUNCPP2_DATA_PROFILES_DEFINES_HPP
#define RUNCPP2_DATA_PROFILES_DEFINES_HPP

#include "runcpp2/Data/ParseCommon.hpp"
#include "runcpp2/YamlLib.hpp"

#include <unordered_map>
#include <vector>
#include <string>

namespace runcpp2
{
    namespace Data
    {
        struct Define
        {
            std::string Name;
            std::string Value;
            bool HasValue;
        };

        class ProfilesDefines
        {
            public:
                std::unordered_map<ProfileName, std::vector<Define>> Defines;

                bool ParseYAML_Node(ryml::ConstNodeRef& node);
                std::string ToString(std::string indentation) const;
                bool Equals(const ProfilesDefines& other) const;
        };
    }
}

#endif
