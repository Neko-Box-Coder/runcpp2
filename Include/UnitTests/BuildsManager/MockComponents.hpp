#ifndef RUNCPP2_UNIT_TESTS_BUILDS_MANAGER_MOCK_COMPONENTS_HPP
#define RUNCPP2_UNIT_TESTS_BUILDS_MANAGER_MOCK_COMPONENTS_HPP

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
                            bool,
                            Mock_remove_all,
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
            CO_OVERRIDE_MEMBER_METHOD(OverrideInstance, std::string, rdbuf, ())
            CO_OVERRIDE_MEMBER_METHOD(OverrideInstance, bool, is_open, ())
            CO_OVERRIDE_MEMBER_METHOD(OverrideInstance, void, close, ())
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
        //CO_OVERRIDE_IMPL(FreeFunctionOverrideInstance, Mock_ofstream&, (mockStream, value));
        mockStream.StringStream << value;
        return mockStream;
    }
    
    inline Mock_ofstream& operator<<(Mock_ofstream& mockStream, std::ostream& (*pf)(std::ostream&))
    {
        //NOTE: This is called when std::endl is passed to the << operator.
        //      Unfortunately function pointer is not tested/implemented yet for CppOverride
        //      void* will do for now
        //void* dummyPointer = nullptr;
        //CO_OVERRIDE_IMPL(FreeFunctionOverrideInstance, Mock_ofstream&, (mockStream, dummyPointer));
        
        mockStream.StringStream << pf;
        return mockStream;
    }
}

#define exists Mock_exists
#define create_directories Mock_create_directories
#define remove_all Mock_remove_all
#define ifstream Mock_ifstream
#define ofstream Mock_ofstream
#define hash Mock_hash

#if INTERNAL_RUNCPP2_UNDEF_MOCKS
    #undef exists
    #undef create_directories
    #undef remove_all
    #undef ifstream
    #undef ofstream
    #undef hash
#endif

#endif
