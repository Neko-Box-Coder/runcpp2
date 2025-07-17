#ifndef RUNCPP2_UNIT_TESTS_CONFIG_PARSING_MOCK_COMPONENTS_HPP
#define RUNCPP2_UNIT_TESTS_CONFIG_PARSING_MOCK_COMPONENTS_HPP

#if !defined(NOMINMAX)
    #define NOMINMAX 1
#endif
#include "ghc/filesystem.hpp"
#include "CppOverride.hpp"

#include <sstream>
#include <type_traits>

extern CO_DECLARE_INSTANCE(OverrideInstance);

namespace ghc
{
    namespace filesystem
    {
        CO_INSERT_METHOD(   OverrideInstance, 
                            bool, 
                            Mock_exists, 
                            (const std::string&, std::error_code&),
                            /* no prepend */,
                            noexcept)
        
        CO_INSERT_METHOD(   OverrideInstance, 
                            bool, 
                            Mock_is_directory, 
                            (const std::string&, std::error_code&),
                            /* no prepend */,
                            noexcept)
    }
}

namespace std
{
    class Mock_ifstream
    {
        public:
            CO_INSERT_MEMBER_METHOD_CTOR(OverrideInstance, Mock_ifstream, const ghc::filesystem::path&)
            CO_INSERT_MEMBER_METHOD_DTOR(OverrideInstance, Mock_ifstream)
            CO_INSERT_MEMBER_METHOD(OverrideInstance, bool, operator!, ())
            CO_INSERT_MEMBER_METHOD(OverrideInstance, std::string, rdbuf, ())
            CO_INSERT_METHOD(   OverrideInstance, 
                                Mock_ifstream&, 
                                operator<<, 
                                (Mock_ifstream&, T const&), 
                                template<typename T> friend)
    };
}

#define exists Mock_exists
#define is_directory Mock_is_directory
#define ifstream Mock_ifstream

#if INTERNAL_RUNCPP2_UNDEF_MOCKS
    #undef exists
    #undef is_directory
    #undef ifstream
#endif

#endif 
