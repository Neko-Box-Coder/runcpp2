#ifndef RUNCPP2_DATA_DEPENDENCY_SOURCE_HPP
#define RUNCPP2_DATA_DEPENDENCY_SOURCE_HPP

#include "runcpp2/Data/DependencySourceType.hpp"
#include "ryml.hpp"
#include <string>

namespace runcpp2
{
    namespace Data
    {
        class DependencySource
        {
            public:
                DependencySourceType Type;
                std::string Value;
                
                bool ParseYAML_Node(ryml::ConstNodeRef& node);
                std::string ToString(std::string indentation) const;
                bool Equals(const DependencySource& other) const;
        };
    }
}

#endif
