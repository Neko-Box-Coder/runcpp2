#ifndef RUNCPP2_DATA_FLAGS_OVERRIDE_INFO_HPP
#define RUNCPP2_DATA_FLAGS_OVERRIDE_INFO_HPP

#include "runcpp2/YamlLib.hpp"
#include <string>

namespace runcpp2
{
    namespace Data
    {
        class FlagsOverrideInfo
        {
            public:
                std::string Remove;
                std::string Append;
                
                bool ParseYAML_Node(ryml::ConstNodeRef node);
                bool ParseYAML_Node(YAML::ConstNodePtr node);
                
                bool IsYAML_NodeParsableAsDefault(ryml::ConstNodeRef node) const;
                bool IsYAML_NodeParsableAsDefault_LibYaml(YAML::ConstNodePtr node) const;
                
                std::string ToString(std::string indentation) const;
                bool Equals(const FlagsOverrideInfo& other) const;
        };
    }
}


#endif
