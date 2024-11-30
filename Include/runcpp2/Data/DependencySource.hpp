#ifndef RUNCPP2_DATA_DEPENDENCY_SOURCE_HPP
#define RUNCPP2_DATA_DEPENDENCY_SOURCE_HPP

#include "runcpp2/Data/GitSource.hpp"
#include "runcpp2/Data/LocalSource.hpp"
#include "runcpp2/YamlLib.hpp"
#include "mpark/variant.hpp"

namespace runcpp2
{
    namespace Data
    {
        class DependencySource
        {
            public:
                mpark::variant<GitSource, LocalSource> Source;
                
                bool ParseYAML_Node(ryml::ConstNodeRef& node);
                std::string ToString(std::string indentation) const;
                bool Equals(const DependencySource& other) const;
        };
    }
}

#endif
