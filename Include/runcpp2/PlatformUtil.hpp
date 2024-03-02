#ifndef RUNCPP2_PLATFORM_UTIL_HPP
#define RUNCPP2_PLATFORM_UTIL_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace runcpp2
{

namespace Internal
{
    char GetAltFileSystemSeparator();
    
    char GetFileSystemSeparator();

    std::string ProcessPath(const std::string& path);

    std::vector<std::string> GetPlatformNames();
}

}

#endif