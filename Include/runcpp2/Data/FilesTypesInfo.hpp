#ifndef RUNCPP2_DATA_FILES_TYPES_HPP
#define RUNCPP2_DATA_FILES_TYPES_HPP

#include "runcpp2/Data/FileProperties.hpp"
#include "runcpp2/Data/ParseCommon.hpp"
#include "ryml.hpp"

namespace runcpp2
{
    namespace Data
    {
        class FilesTypesInfo
        {
            public:
                FileProperties ObjectLinkFile;
                FileProperties SharedLinkFile;
                FileProperties SharedLibraryFile;
                FileProperties StaticLinkFile;
                FileProperties DebugSymbolFile;
            
            bool ParseYAML_Node(ryml::ConstNodeRef& node);
            std::string ToString(std::string indentation) const;
        };
    }
}

#endif
