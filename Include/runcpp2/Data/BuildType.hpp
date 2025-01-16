#ifndef RUNCPP2_DATA_BUILD_TYPE_HPP
#define RUNCPP2_DATA_BUILD_TYPE_HPP

#include <string>

namespace runcpp2
{
    namespace Data
    {
        enum class BuildType
        {
            EXECUTABLE,
            STATIC,
            SHARED,
            OBJECTS,
            COUNT
        };

        inline std::string BuildTypeToString(BuildType buildType)
        {
            static_assert(static_cast<int>(BuildType::COUNT) == 4, "Add new type to be processed");
            
            switch(buildType)
            {
                case BuildType::EXECUTABLE:
                    return "Executable";
                
                case BuildType::STATIC:
                    return "Static";
                
                case BuildType::SHARED:
                    return "Shared";
                
                case BuildType::OBJECTS:
                    return "Objects";
                
                default:
                    return "";
            }
        }

        inline BuildType StringToBuildType(const std::string& buildTypeStr)
        {
            if(buildTypeStr == "Executable")
                return BuildType::EXECUTABLE;
            else if(buildTypeStr == "Static")
                return BuildType::STATIC;
            else if(buildTypeStr == "Shared")
                return BuildType::SHARED;
            else if(buildTypeStr == "Objects")
                return BuildType::OBJECTS;
            
            return BuildType::COUNT;
        }
    }
} 

#endif