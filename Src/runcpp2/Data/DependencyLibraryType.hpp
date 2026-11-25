#ifndef RUNCPP2_DATA_DEPENDENCY_LIBRARY_TYPE_HPP
#define RUNCPP2_DATA_DEPENDENCY_LIBRARY_TYPE_HPP

#include <string>
#include <cassert>

namespace runcpp2
{
    namespace Data
    {
        enum class DependencyLibraryType
        {
            STATIC,
            SHARED,
            OBJECT,
            HEADER,
            SYSTEM,
            COUNT
        };
        
        inline std::string DependencyLibraryTypeToString(DependencyLibraryType type)
        {
            static_assert(static_cast<int>(DependencyLibraryType::COUNT) == 5, "Update below");
            switch(type)
            {
                case DependencyLibraryType::STATIC:
                    return "DependencyLibraryType::STATIC";
                case DependencyLibraryType::SHARED:
                    return "DependencyLibraryType::SHARED";
                case DependencyLibraryType::OBJECT:
                    return "DependencyLibraryType::OBJECT";
                case DependencyLibraryType::HEADER:
                    return "DependencyLibraryType::HEADER";
                case DependencyLibraryType::SYSTEM:
                    return "DependencyLibraryType::SYSTEM";
                default:
                    assert(false);
                    return "Unknown";
            }
        }
    }
}

#endif
