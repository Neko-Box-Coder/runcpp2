#ifndef RUNCPP2_DATA_DEPENDENCY_SETUP_HPP
#define RUNCPP2_DATA_DEPENDENCY_SETUP_HPP

#include "runcpp2/Data/ParseCommon.hpp"

#include "ryml.hpp"
#include <string>
#include <unordered_map>
#include <vector>

namespace runcpp2
{
    namespace Data
    {
        class DependencySetup
        {
            public:
                std::unordered_map<ProfileName, std::vector<std::string>> SetupSteps;
                
                bool ParseYAML_Node(ryml::ConstNodeRef& node);
                std::string ToString(std::string indentation) const;
        };
    }
}

#endif