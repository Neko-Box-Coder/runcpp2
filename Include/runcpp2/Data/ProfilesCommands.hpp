#ifndef RUNCPP2_DATA_PROFILES_COMMANDS_HPP
#define RUNCPP2_DATA_PROFILES_COMMANDS_HPP

#include "runcpp2/Data/ParseCommon.hpp"
#include "runcpp2/YamlLib.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace runcpp2
{
    namespace Data
    {
        class ProfilesCommands
        {
            public:
                //TODO: Allow specifying command can fail
                std::unordered_map<ProfileName, std::vector<std::string>> CommandSteps;
                
                bool ParseYAML_Node(ryml::ConstNodeRef node);
                bool ParseYAML_NodeWithProfile(ryml::ConstNodeRef node, ProfileName profile);
                bool IsYAML_NodeParsableAsDefault(ryml::ConstNodeRef node) const;
                std::string ToString(std::string indentation) const;
                bool Equals(const ProfilesCommands& other) const;
        };
    }
}

#endif
