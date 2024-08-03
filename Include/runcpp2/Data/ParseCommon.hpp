#ifndef RUNCPP2_DATA_PARSE_COMMON_HPP
#define RUNCPP2_DATA_PARSE_COMMON_HPP

#include <string>

using PlatformName = std::string;;
using ProfileName = std::string;

#define INTERNAL_RUNCPP2_SAFE_START() \
try {

#define INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(return_val) \
}\
catch(const std::exception& ex)\
{\
    ssLOG_ERROR("Exception caught: " << ex.what());\
    return return_val;\
}

#define INTERNAL_RUNCPP2_SAFE_CATCH_ACTION(action) \
}\
catch(const std::exception& ex)\
{\
    ssLOG_ERROR("Exception caught: " << ex.what());\
    action\
}\
catch(...)\
{\
    ssLOG_ERROR("Unknown Exception caught");\
    action\
}

#define INTERNAL_RUNCPP2_BIT_CONTANTS(a, b) ((a & b) == b)

#endif
