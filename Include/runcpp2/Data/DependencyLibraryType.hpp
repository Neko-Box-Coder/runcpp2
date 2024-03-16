#ifndef RUNCPP2_DATA_DEPENDENCY_LIBRARY_TYPE_HPP
#define RUNCPP2_DATA_DEPENDENCY_LIBRARY_TYPE_HPP

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
            COUNT
        };
    }
}

#endif