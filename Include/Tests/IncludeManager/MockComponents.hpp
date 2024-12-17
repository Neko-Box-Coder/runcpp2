#ifndef RUNCPP2_UNIT_TESTS_INCLUDE_MANAGER_MOCK_COMPONENTS_HPP
#define RUNCPP2_UNIT_TESTS_INCLUDE_MANAGER_MOCK_COMPONENTS_HPP

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
        CO_OVERRIDE_METHOD( OverrideInstance, 
                            bool, 
                            Mock_exists, 
                            (const path&, std::error_code&),
                            /* no prepend */,
                            noexcept)
        
        CO_OVERRIDE_METHOD( OverrideInstance,
                            bool,
                            Mock_create_directories,
                            (const path&, std::error_code&),
                            /* no prepend */,
                            noexcept)
        
        CO_OVERRIDE_METHOD( OverrideInstance,
                            file_time_type,
                            Mock_last_write_time,
                            (const path&, std::error_code&),
                            /* no prepend */,
                            noexcept)
    }
}

namespace std
{
    class Mock_ifstream
    {
        public:
            CO_OVERRIDE_MEMBER_METHOD_CTOR(OverrideInstance, Mock_ifstream, const ghc::filesystem::path&)
            CO_OVERRIDE_MEMBER_METHOD(OverrideInstance, bool, is_open, ())
    };
    
    class Mock_ofstream
    {
        public:
            std::stringstream StringStream;
            
            CO_OVERRIDE_MEMBER_METHOD_CTOR(OverrideInstance, Mock_ofstream, const ghc::filesystem::path&)
            CO_OVERRIDE_MEMBER_METHOD(OverrideInstance, bool, is_open, ())
            CO_OVERRIDE_MEMBER_METHOD(OverrideInstance, void, close, ())
            
            template<typename T>
            friend Mock_ofstream& operator<<(Mock_ofstream&, T const&);
    };
    
    template<typename T>
    class Mock_hash
    {
        static_assert(std::is_same<T, std::string>::value, "We are only mocking std string");
        public:
            CO_OVERRIDE_MEMBER_METHOD(OverrideInstance, std::size_t, operator(), (T))
    };
    
    template<typename T>
    inline Mock_ofstream& operator<<(Mock_ofstream& mockStream, T const& value)
    {
        mockStream.StringStream << value;
        return mockStream;
    }
    
    inline Mock_ofstream& operator<<(Mock_ofstream& mockStream, std::ostream& (*pf)(std::ostream&))
    {
        mockStream.StringStream << pf;
        return mockStream;
    }

    inline bool Mock_getline(Mock_ifstream& stream, std::string& line)
    {
        CO_OVERRIDE_IMPL(OverrideInstance, bool, (stream, line));
        return false;
    }
}

#define exists Mock_exists
#define create_directories Mock_create_directories
#define last_write_time Mock_last_write_time
#define ifstream Mock_ifstream
#define ofstream Mock_ofstream
#define hash Mock_hash
#define getline Mock_getline

#if INTERNAL_RUNCPP2_UNDEF_MOCKS
    #undef exists
    #undef create_directories
    #undef last_write_time
    #undef ifstream
    #undef ofstream
    #undef hash
    #undef getline
#endif

#endif 
