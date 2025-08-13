#ifndef RUNCPP2_UNIT_TESTS_BUILDS_MANAGER_MOCK_COMPONENTS_HPP
#define RUNCPP2_UNIT_TESTS_BUILDS_MANAGER_MOCK_COMPONENTS_HPP

#if !defined(NOMINMAX)
    #define NOMINMAX 1
#endif
#include "ghc/filesystem.hpp"

#include "CppOverride.hpp"

#include <sstream>
#include <type_traits>
#include <string>
#include <unordered_map>

extern CO_DECLARE_INSTANCE(OverrideInstance);

namespace ghc
{
    namespace filesystem
    {
        CO_INSERT_METHOD(   OverrideInstance, 
                            bool, 
                            Mock_exists, 
                            (const ghc::filesystem::path&, std::error_code&),
                            /* no prepend */,
                            noexcept)
        
        CO_INSERT_METHOD(   OverrideInstance,
                            bool,
                            Mock_create_directories,
                            (const ghc::filesystem::path&, std::error_code&),
                            /* no prepend */,
                            noexcept)
        
        CO_INSERT_METHOD(   OverrideInstance,
                            bool,
                            Mock_remove_all,
                            (const ghc::filesystem::path&, std::error_code&),
                            /* no prepend */,
                            noexcept)
    }
}

namespace Mock_std
{
    class Mock_ifstream
    {
        public:
            CO_INSERT_MEMBER_METHOD_CTOR(OverrideInstance, Mock_ifstream, const ghc::filesystem::path&)
            CO_INSERT_MEMBER_METHOD(OverrideInstance, std::string, rdbuf, ())
            CO_INSERT_MEMBER_METHOD(OverrideInstance, bool, is_open, ())
            CO_INSERT_MEMBER_METHOD(OverrideInstance, void, close, ())
    };
    
    class Mock_ofstream
    {
        public:
            std::stringstream StringStream;
            
            CO_INSERT_MEMBER_METHOD_CTOR(OverrideInstance, Mock_ofstream, const ghc::filesystem::path&)
            
            CO_INSERT_MEMBER_METHOD(OverrideInstance, bool, is_open, ())
            CO_INSERT_MEMBER_METHOD(OverrideInstance, void, close, ())
            
            template<typename T>
            friend Mock_ofstream& operator<<(Mock_ofstream&, T const&);
    };
    
    template<typename T>
    class Mock_hash
    {
        static_assert(std::is_same<T, std::string>::value, "We are only mocking std string");
        public:
            CO_INSERT_MEMBER_METHOD(OverrideInstance, std::size_t, operator(), (T))
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
    
    CO_FORWARD_TYPE(std, string);
    CO_FORWARD_TYPE(std, stringstream);
    CO_FORWARD_TYPE(std, istringstream);
    CO_FORWARD_TYPE(std, error_code);
    CO_FORWARD_TYPE(std, size_t);
    CO_FORWARD_TEMPLATE_TYPE(std, unordered_map);
    
    namespace
    {
        auto& cout = std::cout;
    }
    
    template< class CharT, class Traits >
    std::basic_ostream<CharT, Traits>& endl( std::basic_ostream<CharT, Traits>& os )
    {
        return std::endl(os);
    }
    
    template<typename T>
    inline std::string to_string(T val)
    {
        return std::to_string(val);
    }
    
    template<typename T>
    std::istream& getline(std::istream& is, T& val)
    {
        return std::getline(is, val);
    }
}

#define exists Mock_exists
#define create_directories Mock_create_directories
#define remove_all Mock_remove_all
#define ifstream Mock_ifstream
#define ofstream Mock_ofstream
#define hash Mock_hash
#define std Mock_std

#if INTERNAL_RUNCPP2_UNDEF_MOCKS
    #undef exists
    #undef create_directories
    #undef remove_all
    #undef ifstream
    #undef ofstream
    #undef hash
    #undef std
#endif

#endif
