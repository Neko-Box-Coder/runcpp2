#include "runcpp2/Data/BuildTypeHelper.hpp"
#include "runcpp2/PlatformUtil.hpp"

#include "ssLogger/ssLog.hpp"

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
                //NOTE: Currently this is a special case and handled outside
                //TODO: Handle it as config
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
    bool Data::BuildTypeHelper::NeedsLinking(BuildType buildType)
    {
        return buildType != BuildType::OBJECTS;
    }

    bool Data::BuildTypeHelper::GetPossibleOutputPaths( const ghc::filesystem::path& buildDir,
                                                        const std::string& scriptName,
                                                        const Profile& profile,
                                                        const BuildType buildType,
                                                        std::vector<ghc::filesystem::path>& outPaths,
                                                        std::vector<bool>& outIsRunnable)
    {
        outPaths.clear();
        outIsRunnable.clear();
        
        //For executable build type with executable flag, use platform-specific extension
        if(buildType == BuildType::INTERNAL_EXECUTABLE_EXECUTABLE)
        {
            #ifdef _WIN32
                outPaths.push_back(buildDir / (scriptName + ".exe"));
            #else
                outPaths.push_back(buildDir / scriptName);
            #endif
            outIsRunnable.push_back(true);
        }

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

            if(targetExt->empty() && targetPrefix->empty())
                continue;

            outPaths.push_back(buildDir / (*targetPrefix + scriptName + *targetExt));
            
            //Only SharedLibraryFile is runnable for non-direct executables
            outIsRunnable.push_back(fileTypeInfo == &profile.FilesTypes.SharedLibraryFile);
        }

        return !outPaths.empty();
    }
} 
