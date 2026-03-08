#ifndef RUNCPP2_DEFER_UTIL_HPP
#define RUNCPP2_DEFER_UTIL_HPP

//NOTE: Improvised from https://stackoverflow.com/a/42060129
namespace runcpp2
{
    struct DeferDummy {};
    template <class T> struct DeferObj { T f; ~DeferObj() { f(); } };
    template <class T> DeferObj<T> operator*(DeferDummy, T f) { return {f}; }
    #define INTERNAL_DEFER_(LINE) zz_defer##LINE
    #define INTERNAL_DEFER__(LINE) INTERNAL_DEFER_(LINE)
    #define DEFER auto INTERNAL_DEFER__(__COUNTER__) = runcpp2::DeferDummy{} * [&]()
}

#endif
