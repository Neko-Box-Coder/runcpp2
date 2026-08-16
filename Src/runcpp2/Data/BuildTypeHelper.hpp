#ifndef RUNCPP2_DATA_BUILD_TYPE_HELPER_HPP
#define RUNCPP2_DATA_BUILD_TYPE_HELPER_HPP

#include "runcpp2/Data/BuildType.hpp"
#include "runcpp2/Data/FilesTypesInfo.hpp"
#include "runcpp2/Data/Profile.hpp"
#include "runcpp2/Data/FileProperties.hpp"
#include "runcpp2/PlatformUtil.hpp"

#include "ghc/filesystem.hpp"
#include "ssLogger/ssLog.hpp"

#include <string>
#include <vector>

namespace
{
    using namespace runcpp2::Data;
    
    std::vector<const FileProperties*> 
    GetOutputFileProperties(const FilesTypesInfo& filesTypes, BuildType buildType)
    {
        std::vector<const FileProperties*> properties;
        
        static_assert(static_cast<int>(BuildType::COUNT) == 6, "Update This");
        switch(buildType)
        {
            case BuildType::INTERNAL_EXECUTABLE_EXECUTABLE:
                properties.push_back(&filesTypes.ExecutableFile);
                break;
            case BuildType::INTERNAL_EXECUTABLE_SHARED:
            case BuildType::SHARED:
                properties.push_back(&filesTypes.SharedLibraryFile);
                properties.push_back(&filesTypes.SharedLinkFile);
                break;
            case BuildType::STATIC:
                properties.push_back(&filesTypes.StaticLinkFile);
                break;
            case BuildType::OBJECTS:
                properties.push_back(&filesTypes.ObjectLinkFile);
                break;
        }
        
        properties.push_back(&filesTypes.DebugSymbolFile);
        return properties;
    }
}

namespace runcpp2
{
namespace Data
{
namespace BuildTypeHelper
{
    inline bool NeedsLinking(BuildType buildType)
    {
        return buildType != BuildType::OBJECTS;
    }

    inline bool GetPossibleOutputPaths( const ghc::filesystem::path& buildDir,
                                        const std::string& scriptName,
                                        const Profile& profile,
                                        const BuildType buildType,
                                        std::vector<ghc::filesystem::path>& outPaths,
                                        std::vector<bool>& outIsRunnable)
    {
        outPaths.clear();
        outIsRunnable.clear();
        
        //Get all relevant file properties
        std::vector<const FileProperties*> fileProperties = 
            GetOutputFileProperties(profile.FilesTypes, buildType);
        
        //Generate paths for each file type
        for(const FileProperties* fileTypeInfo : fileProperties)
        {
            if(!fileTypeInfo)
            {
                ssLOG_ERROR("fileProperties should not contain nullptr");
                return false;
            }
            
            const std::string* targetExt = runcpp2::GetValueFromPlatformMap(fileTypeInfo->Extension);
            const std::string* targetPrefix = runcpp2::GetValueFromPlatformMap(fileTypeInfo->Prefix);

            if(targetExt == nullptr || targetPrefix == nullptr)
                continue;

            outPaths.push_back(buildDir / (*targetPrefix + scriptName + *targetExt));
            
            //Only ExecutableFile or SharedLibraryFile are runnable for non-direct executables
            outIsRunnable.push_back(fileTypeInfo == &profile.FilesTypes.SharedLibraryFile ||
                                    fileTypeInfo == &profile.FilesTypes.ExecutableFile);
        }

        return !outPaths.empty();
    }
}
}
}

#endif
