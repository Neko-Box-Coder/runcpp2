#ifndef RUNCPP2_DATA_DEPENDENCY_COMMANDS_HPP
#define RUNCPP2_DATA_DEPENDENCY_COMMANDS_HPP

#include "runcpp2/Data/ParseCommon.hpp"

#include "ryml.hpp"
#include <string>
#include <unordered_map>
#include <vector>

namespace runcpp2
{
    namespace Data
    {
        class DependencyCommands
        {
            public:
                //TODO: Allow specifying command can fail
                std::unordered_map<ProfileName, std::vector<std::string>> CommandSteps;
                
                bool ParseYAML_Node(ryml::ConstNodeRef& node);
                std::string ToString(std::string indentation) const;
                bool Equals(const DependencyCommands& other) const;
        };
    }
}

#endif
