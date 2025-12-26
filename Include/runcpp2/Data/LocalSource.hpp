#ifndef RUNCPP2_DATA_LOCAL_SOURCE_HPP
#define RUNCPP2_DATA_LOCAL_SOURCE_HPP

#include "runcpp2/YamlLib.hpp"
#include <string>

namespace runcpp2
{
    namespace Data
    {
        enum class LocalCopyMode
        {
            Auto,
            Symlink,
            Hardlink,
            Copy,
            Count
        };

        class LocalSource
        {
            public:
                std::string Path;
                LocalCopyMode CopyMode = LocalCopyMode::Auto;
                
                bool ParseYAML_Node(ryml::ConstNodeRef node);
                bool ParseYAML_Node(YAML::ConstNodePtr node);
                
                std::string ToString(std::string indentation) const;
                bool Equals(const LocalSource& other) const;
        };
    }
}

#endif 
