#include "runcpp2/Data/BuildTypeHelper.hpp"
#include "runcpp2/PlatformUtil.hpp"

namespace runcpp2
{
    const Data::FileProperties* 
    Data::BuildTypeHelper::GetOutputFileProperties( const FilesTypesInfo& filesTypes,
                                                    BuildType buildType,
                                                    bool asExecutable)
    {
        //For executable build type with executable flag, 
        //  return null as it uses platform-specific handling
        if(buildType == BuildType::EXECUTABLE && asExecutable)
            return nullptr;

        switch(buildType)
        {
            case BuildType::EXECUTABLE:
            case BuildType::SHARED:
                return &filesTypes.SharedLibraryFile;
            case BuildType::STATIC:
                return &filesTypes.StaticLinkFile;
            case BuildType::OBJECTS:
                return &filesTypes.ObjectLinkFile;
            default:
                return nullptr;
        }
    }

    bool Data::BuildTypeHelper::NeedsLinking(BuildType buildType)
    {
        return buildType != BuildType::OBJECTS;
    }

    bool Data::BuildTypeHelper::GetOutputPath(const ghc::filesystem::path& buildDir,
                                              const std::string& scriptName,
                                              const Profile& profile,
                                              BuildType buildType,
                                              bool asExecutable,
                                              ghc::filesystem::path& outPath)
    {
        //For executable build type with executable flag, use platform-specific extension
        if(buildType == BuildType::EXECUTABLE && asExecutable)
        {
            #ifdef _WIN32
                outPath = buildDir / (scriptName + ".exe");
            #else
                outPath = buildDir / scriptName;
            #endif
            return true;
        }

        const FileProperties* fileTypeInfo = 
            GetOutputFileProperties(profile.FilesTypes, buildType, asExecutable);
        if(!fileTypeInfo)
            return false;

        const std::string* targetExt = runcpp2::GetValueFromPlatformMap(fileTypeInfo->Extension);
        const std::string* targetPrefix = runcpp2::GetValueFromPlatformMap(fileTypeInfo->Prefix);

        if(targetExt == nullptr || targetPrefix == nullptr)
            return false;

        outPath = buildDir / (*targetPrefix + scriptName + *targetExt);
        return true;
    }
} 
