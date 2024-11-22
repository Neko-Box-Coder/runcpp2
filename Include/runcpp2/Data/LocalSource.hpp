#ifndef RUNCPP2_DATA_LOCAL_SOURCE_HPP
#define RUNCPP2_DATA_LOCAL_SOURCE_HPP

#include "runcpp2/YamlLib.hpp"
#include <string>

namespace runcpp2
{
    namespace Data
    {
        class LocalSource
        {
            public:
                std::string Path;
                
                bool ParseYAML_Node(ryml::ConstNodeRef& node);
                std::string ToString(std::string indentation) const;
                bool Equals(const LocalSource& other) const;
        };
    }
}

#endif 