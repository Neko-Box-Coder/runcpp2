#include "runcpp2/StringUtil.hpp"

namespace runcpp2
{
    void Internal::TrimLeft(std::string& str)
    {
        //Remove spaces from left
        while(!str.empty() && str.at(0) == ' ')
            str.erase(0, 1);
        
        //Remove tabs from left
        while(!str.empty() && str.at(0) == '\t')
            str.erase(0, 1);
    }

    void Internal::TrimRight(std::string& str)
    {
        //Remove spaces from right
        while(!str.empty() && str.at(str.size() - 1) == ' ')
            str.erase(str.size() - 1, 1);
        
        //Remove tabs from right
        while(!str.empty() && str.at(str.size() - 1) == '\t')
            str.erase(str.size() - 1, 1);
    }

    //Trim string from both sides
    void Internal::Trim(std::string& str)
    {
        //Remove spaces from left
        TrimLeft(str);
        
        //Remove spaces from right
        TrimRight(str);
    }
    
    void Internal::SplitString( const std::string& stringToSplit, 
                                const std::string& splitter, 
                                std::vector<std::string>& outStrings)
    {
        if(splitter.empty())
        {
            outStrings.push_back(stringToSplit);
            return;
        }
        
        std::string curOutString;
        std::vector<std::string> tempStringsToCheckForSplit;
        
        for(int i = 0; i < stringToSplit.size(); ++i)
        {
            curOutString += stringToSplit.at(i);
            
            //Check existing string matches
            for(int j = 0; j < tempStringsToCheckForSplit.size(); ++j)
            {
                if( tempStringsToCheckForSplit.at(j).size() >= splitter.size() - 1 || 
                    stringToSplit.at(i) != splitter.at(tempStringsToCheckForSplit.at(j).size()))
                {
                    tempStringsToCheckForSplit.erase(tempStringsToCheckForSplit.begin() + j);
                    --j;
                    continue;
                }
                else
                {
                    tempStringsToCheckForSplit.at(j) += stringToSplit.at(i);
                    
                    //If there's a match, clear record
                    if(tempStringsToCheckForSplit.size() == splitter.size())
                    {
                        curOutString.erase(curOutString.size() - splitter.size());
                        outStrings.push_back(curOutString);
                        curOutString.clear();
                        tempStringsToCheckForSplit.clear();
                        break;
                    }
                }
            }
            
            //If the current character fits the first character of splitter, add it to the record
            if(stringToSplit.at(i) == splitter.front())
            {
                //Check special case if the splitter is only 1 character
                if(splitter.size() == 1)
                {
                    curOutString.erase(curOutString.size() - splitter.size());
                    outStrings.push_back(curOutString);
                    curOutString.clear();
                }
                //Otherwise push to string matches
                else
                {
                    tempStringsToCheckForSplit.push_back(std::string());
                    tempStringsToCheckForSplit.back() += stringToSplit.at(i);
                    
                }
            }
        }
        
        //Push the remaining string
        if(!curOutString.empty())
            outStrings.push_back(curOutString);
    }
}
