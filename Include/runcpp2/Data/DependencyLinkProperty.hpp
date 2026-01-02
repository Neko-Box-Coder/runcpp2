#ifndef RUNCPP2_DATA_DEPENDENCY_LINK_PROPERTY_HPP
#define RUNCPP2_DATA_DEPENDENCY_LINK_PROPERTY_HPP

#include "runcpp2/Data/ParseCommon.hpp"
#include "runcpp2/YamlLib.hpp"

#include <vector>
#include <unordered_map>
#include <string>

namespace runcpp2
{
    namespace Data
    {
        struct ProfileLinkProperty
        {
            std::vector<std::string> SearchLibraryNames;
            std::vector<std::string> SearchDirectories;
            std::vector<std::string> ExcludeLibraryNames;
            std::vector<std::string> AdditionalLinkOptions;
        };

        class DependencyLinkProperty
        {
            public:
                std::unordered_map<ProfileName, ProfileLinkProperty> ProfileProperties;
                
                bool ParseYAML_Node(YAML::ConstNodePtr node);
                
                bool ParseYAML_NodeWithProfile_LibYaml(YAML::ConstNodePtr node, ProfileName profile);
                bool IsYAML_NodeParsableAsDefault_LibYaml(YAML::ConstNodePtr node) const;
                
                std::string ToString(std::string indentation) const;
                bool Equals(const DependencyLinkProperty& other) const;
        };
    }
}

#endif
