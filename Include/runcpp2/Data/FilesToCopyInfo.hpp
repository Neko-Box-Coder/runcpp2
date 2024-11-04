#ifndef RUNCPP2_DATA_FILES_TO_COPY_INFO_HPP
#define RUNCPP2_DATA_FILES_TO_COPY_INFO_HPP

#include "runcpp2/Data/ParseCommon.hpp"
#include "runcpp2/YamlLib.hpp"

#include <unordered_map>
#include <vector>
#include <string>

namespace runcpp2
{
    namespace Data
    {
        struct FilesToCopyInfo
        {
            std::unordered_map<ProfileName, std::vector<std::string>> ProfileFiles;
            
            bool ParseYAML_Node(const ryml::ConstNodeRef& node);
            std::string ToString(std::string indentation) const;
            bool Equals(const FilesToCopyInfo& other) const;
        };
    }
}

#endif
