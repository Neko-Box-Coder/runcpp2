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
                std::string CheckExistence;
                std::unordered_map<PlatformName, std::string> DefaultLinkFlags;
                std::unordered_map<PlatformName, std::string> ExecutableLinkFlags;
                std::unordered_map<PlatformName, std::string> StaticLibLinkFlags;
                std::unordered_map<PlatformName, std::string> SharedLibLinkFlags;
                
                struct Args
                {
                    std::string OutputPart;
                    std::string LinkPart;
                };
                
                Args LinkArgs;
                
                bool ParseYAML_Node(ryml::ConstNodeRef& node);
                std::string ToString(std::string indentation) const;
                bool Equals(const LinkerInfo& other) const;
        };
    }
}

#endif
