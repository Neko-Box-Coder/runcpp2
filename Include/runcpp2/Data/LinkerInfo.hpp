#ifndef RUNCPP2_DATA_LINKER_INFO_HPP
#define RUNCPP2_DATA_LINKER_INFO_HPP

#include "runcpp2/Data/ParseCommon.hpp"
#include "ryml.hpp"

#include <vector>
#include <string>
#include <unordered_map>

namespace runcpp2
{
    namespace Data
    {
        class LinkerInfo
        {
            public:
                std::unordered_map<PlatformName, std::string> EnvironmentSetup;
                std::string Executable;
                std::string DefaultLinkFlags;
                
                struct Args
                {
                    std::string OutputPart;
                    std::string DependenciesPart;
                };
                
                Args LinkerArgs;
                
                bool ParseYAML_Node(ryml::ConstNodeRef& node);
                std::string ToString(std::string indentation) const;
        };
    }
}

#endif