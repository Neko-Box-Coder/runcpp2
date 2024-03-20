#ifndef RUNCPP2_DATA_LINKER_INFO_HPP
#define RUNCPP2_DATA_LINKER_INFO_HPP

#include "ryml.hpp"
#include <string>

namespace runcpp2
{
    namespace Data
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
                
                bool ParseYAML_Node(ryml::ConstNodeRef& node);
                std::string ToString(std::string indentation) const;
        };
    }
}

#endif