#ifndef RUNCPP2_DATA_BUILD_TYPE_HELPER_HPP
#define RUNCPP2_DATA_BUILD_TYPE_HELPER_HPP

#include "runcpp2/Data/BuildType.hpp"
#include "runcpp2/Data/FilesTypesInfo.hpp"
#include "runcpp2/Data/Profile.hpp"
#include "ghc/filesystem.hpp"

namespace runcpp2
{
    namespace Data
    {
        namespace BuildTypeHelper
        {
            const FileProperties* GetOutputFileProperties(  const FilesTypesInfo& filesTypes,
                                                            BuildType buildType,
                                                            bool asExecutable);
            
            bool NeedsLinking(BuildType buildType);
            
            bool GetOutputPath( const ghc::filesystem::path& buildDir,
                                const std::string& scriptName,
                                const Profile& profile,
                                BuildType buildType,
                                bool asExecutable,
                                ghc::filesystem::path& outPath);
        }
    }
}

#endif 
