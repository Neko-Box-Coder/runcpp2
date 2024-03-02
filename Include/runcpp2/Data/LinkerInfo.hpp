#ifndef RUNCPP2_DATA_LINKER_INFO_HPP
#define RUNCPP2_DATA_LINKER_INFO_HPP

#include "yaml-cpp/yaml.h"
#include <string>

namespace runcpp2
{
    class LinkerInfo
    {
        public:
            std::string Executable;
            std::string DefaultLinkFlags;
            
            struct Args
            {
                std::string OutputPart;
                std::string DependenciesPart;
            };
            
            Args LinkerArgs;
            
            bool ParseYAML_Node(YAML::Node& profileNode);
            std::string ToString(std::string indentation) const;
    };
}

#endif