#ifndef RUNCPP2_STRING_UTIL_HPP
#define RUNCPP2_STRING_UTIL_HPP

#include <string>
#include <vector>

namespace runcpp2
{
    namespace Internal
    {
    
        void TrimLeft(std::string& str);
        void TrimRight(std::string& str);
        void Trim(std::string& str);
        
        void SplitString(   const std::string& stringToSplit, 
                            const std::string& splitter, 
                            std::vector<std::string>& outStrings);
    }
    
}

#endif