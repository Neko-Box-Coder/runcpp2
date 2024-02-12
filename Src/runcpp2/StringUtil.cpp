#include "runcpp2/StringUtil.hpp"

namespace runcpp2
{
    void TrimLeft(std::string& str)
    {
        //Remove spaces from left
        while(!str.empty() && str.at(0) == ' ')
            str.erase(0, 1);
        
        //Remove tabs from left
        while(!str.empty() && str.at(0) == '\t')
            str.erase(0, 1);
    }

    void TrimRight(std::string& str)
    {
        //Remove spaces from right
        while(!str.empty() && str.at(str.size() - 1) == ' ')
            str.erase(str.size() - 1, 1);
        
        //Remove tabs from right
        while(!str.empty() && str.at(str.size() - 1) == '\t')
            str.erase(str.size() - 1, 1);
    }

    //Trim string from both sides
    void Trim(std::string& str)
    {
        //Remove spaces from left
        TrimLeft(str);
        
        //Remove spaces from right
        TrimRight(str);
    }
}
