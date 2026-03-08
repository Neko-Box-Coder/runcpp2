#ifndef RUNCPP2_DATA_SUBMODULE_INIT_TYPE_HPP
#define RUNCPP2_DATA_SUBMODULE_INIT_TYPE_HPP

#include <string>

namespace runcpp2
{
    namespace Data
    {
        enum class SubmoduleInitType
        {
            NONE,
            SHALLOW,
            FULL,
            COUNT
        };
        
        inline std::string SubmoduleInitTypeToString(SubmoduleInitType submoduleInitType)
        {
            static_assert(  static_cast<int>(SubmoduleInitType::COUNT) == 3, 
                            "Add new type to be processed");
            
            switch(submoduleInitType)
            {
                case SubmoduleInitType::NONE:
                    return "None";
                
                case SubmoduleInitType::SHALLOW:
                    return "Shallow";
                
                case SubmoduleInitType::FULL:
                    return "Full";
                
                case SubmoduleInitType::COUNT:
                    return "Count";
                
                default:
                    return "";
            }
        }

        inline SubmoduleInitType StringToSubmoduleInitType(const std::string& submoduleInitTypeStr)
        {
            if(submoduleInitTypeStr == "None")
                return SubmoduleInitType::NONE;
            else if(submoduleInitTypeStr == "Shallow")
                return SubmoduleInitType::SHALLOW;
            else if(submoduleInitTypeStr == "Full")
                return SubmoduleInitType::FULL;
            
            return SubmoduleInitType::COUNT;
        }
    }
}


#endif
