#ifndef RUNCPP2_UNIT_TESTS_CONFIG_PARSING_MOCK_COMPONENTS_HPP
#define RUNCPP2_UNIT_TESTS_CONFIG_PARSING_MOCK_COMPONENTS_HPP

#if !defined(NOMINMAX)
    #define NOMINMAX 1
#endif
#include "ghc/filesystem.hpp"
#include "CppOverride.hpp"

#include <sstream>
#include <type_traits>
#include <vector>
#include <utility>

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
                            Mock_is_directory, 
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
            CO_INSERT_MEMBER_METHOD_DTOR(OverrideInstance, Mock_ifstream)
            CO_INSERT_MEMBER_METHOD(OverrideInstance, bool, operator!, ())
            CO_INSERT_MEMBER_METHOD(OverrideInstance, std::string, rdbuf, ())
            CO_INSERT_METHOD(   OverrideInstance, 
                                Mock_ifstream&, 
                                operator<<, 
                                (Mock_ifstream&, T const&), 
                                template<typename T> friend)
    };
    
    CO_FORWARD_TEMPLATE_TYPE(std, vector);
    CO_FORWARD_TYPE(std, stringstream);
    CO_FORWARD_TYPE(std, string);
    CO_FORWARD_TEMPLATE_TYPE(std, stack);
    CO_FORWARD_TYPE(std, error_code);
    CO_FORWARD_TYPE(std, exception);
    CO_FORWARD_TEMPLATE_TYPE(std, unordered_map);
    CO_FORWARD_TYPE(std, ofstream);
    CO_FORWARD_TYPE(std, ios);
    CO_FORWARD_TYPE(std, ios_base);
    
    template<typename T>
    inline std::string to_string(T val)
    {
        return std::to_string(val);
    }
    
    namespace
    {
        auto& cout = std::cout;
    }
    
    template< class CharT, class Traits >
    std::basic_ostream<CharT, Traits>& endl( std::basic_ostream<CharT, Traits>& os )
    {
        return std::endl(os);
    }
    
    inline int stoi(const std::string& str, std::size_t* pos = nullptr, int base = 10)
    {
        return std::stoi(str, pos, base);
    }
    
    template<class T>
    typename std::remove_reference<T>::type&& move( T&& t ) noexcept
    {
        return std::move(t);
    }
}


#define exists Mock_exists
#define is_directory Mock_is_directory
#define ifstream Mock_ifstream
#define std Mock_std

#if INTERNAL_RUNCPP2_UNDEF_MOCKS
    #undef exists
    #undef is_directory
    #undef ifstream
    #undef std
#endif

#endif 
