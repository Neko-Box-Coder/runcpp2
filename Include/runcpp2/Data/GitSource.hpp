#ifndef RUNCPP2_DATA_GIT_SOURCE_HPP
#define RUNCPP2_DATA_GIT_SOURCE_HPP

#include "runcpp2/YamlLib.hpp"
#include <string>

namespace runcpp2
{
    namespace Data
    {
        class GitSource
        {
            public:
                std::string URL;
                
                bool ParseYAML_Node(ryml::ConstNodeRef& node);
                std::string ToString(std::string indentation) const;
                bool Equals(const GitSource& other) const;
        };
    }
}

#endif 