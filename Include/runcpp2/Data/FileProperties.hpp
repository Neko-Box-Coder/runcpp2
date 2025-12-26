#ifndef RUNCPP2_DATA_FILE_PROPERTIES_HPP
#define RUNCPP2_DATA_FILE_PROPERTIES_HPP

#include "runcpp2/Data/ParseCommon.hpp"
#include "runcpp2/YamlLib.hpp"

#include <unordered_map>

namespace runcpp2
{
    namespace Data
    {
        struct FileProperties
        {
            std::unordered_map<PlatformName, std::string> Prefix;
            std::unordered_map<PlatformName, std::string> Extension;
            
            bool ParseYAML_Node(ryml::ConstNodeRef node);
            bool ParseYAML_Node(YAML::ConstNodePtr node);

            std::string ToString(std::string indentation) const;
            bool Equals(const FileProperties& other) const;
        };
    
    }
}

#endif
