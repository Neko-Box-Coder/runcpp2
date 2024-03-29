#ifndef RUNCPP2_DATA_FLAGS_OVERRIDE_INFO_HPP
#define RUNCPP2_DATA_FLAGS_OVERRIDE_INFO_HPP

#include "ryml.hpp"
#include <string>

namespace runcpp2
{
    namespace Data
    {
        class FlagsOverrideInfo
        {
            public:
                std::string Remove;
                std::string Append;
                
                bool ParseYAML_Node(ryml::ConstNodeRef& node);
                std::string ToString(std::string indentation) const;
        };
    }
}


#endif