#include "runcpp2/DependenciesHelper.hpp"
#include "ghc/filesystem.hpp"
#include "runcpp2/PlatformUtil.hpp"
#include "ssLogger/ssLog.hpp"

namespace
{
    bool PopulateLocalDependencies( const std::vector<runcpp2::Data::DependencyInfo*>& dependencies,
                                    const std::vector<std::string>& dependenciesCopiesPaths,
                                    const std::vector<std::string>& dependenciesSourcesPaths,
                                    const ghc::filesystem::path& buildDir,
                                    std::vector<bool>& outPrePopulated)
    {
        std::error_code _;
        for(int i = 0; i < dependencies.size(); ++i)
        {
            if(ghc::filesystem::exists(dependenciesCopiesPaths.at(i), _))
            {
                if(!ghc::filesystem::is_directory(dependenciesCopiesPaths.at(i), _))
                {
                    ssLOG_ERROR("Dependency path is a file: " << dependenciesCopiesPaths.at(i));
                    ssLOG_ERROR("It should be a folder instead");
                    return false;
                }
                outPrePopulated.push_back(true);
            }
            else
            {
                static_assert((int)runcpp2::Data::DependencySourceType::COUNT == 2, "");
                
                switch(dependencies.at(i)->Source.Type)
                {
                    case runcpp2::Data::DependencySourceType::GIT:
                    {
                        std::string gitCloneCommand = 
                            "git clone " + dependencies.at(i)->Source.Value;
                        
                        ssLOG_INFO( "Running git clone command: " << gitCloneCommand << " in " << 
                                    buildDir.string());
                        
                        int returnCode = 0;
                        std::string output;
                        if(!runcpp2::RunCommandAndGetOutput(gitCloneCommand, 
                                                            output, 
                                                            returnCode,
                                                            buildDir.string()))
                        {
                            ssLOG_ERROR("Failed to run git clone with result: " << returnCode);
                            ssLOG_ERROR("Output: \n" << output);
                            return false;
                        }
                        else
                            ssLOG_INFO("Output: \n" << output);
                        
                        break;
                    }
                    
                    case runcpp2::Data::DependencySourceType::LOCAL:
                    {
                        std::string sourcePath = dependenciesSourcesPaths.at(i);
                        std::string destinationPath = dependenciesCopiesPaths.at(i);
                        
                        //Copy the folder
                        ghc::filesystem::copy(destinationPath, sourcePath, _);
                        break;
                    }
                    
                    case runcpp2::Data::DependencySourceType::COUNT:
                        return false;
                }
                
                outPrePopulated.push_back(false);
            }
        }
        
        return true;
    }
    
    bool PopulateAbsoluteIncludePaths(  std::vector<runcpp2::Data::DependencyInfo*>& dependencies,
                                        const std::vector<std::string>& dependenciesCopiesPaths)
    {
        //Append absolute include paths from relative include paths
        for(int i = 0; i < dependencies.size(); ++i)
        {
            dependencies.at(i)->AbsoluteIncludePaths.clear();
            for(int j = 0; j < dependencies.at(i)->IncludePaths.size(); ++j)
            {
                if(ghc::filesystem::path(dependencies.at(i)->IncludePaths.at(j)).is_absolute())
                {
                    ssLOG_ERROR("Dependency include path cannot be absolute: " << 
                                dependencies.at(i)->IncludePaths.at(j));
                    
                    return false;
                }
                
                dependencies.at(i) 
                            ->AbsoluteIncludePaths
                            .push_back(ghc::filesystem::absolute(   dependenciesCopiesPaths.at(i) + "/" + 
                                                                    dependencies.at(i)->IncludePaths.at(j)).string());
            
                ssLOG_DEBUG("Include path added: " << dependencies.at(i)->AbsoluteIncludePaths.back());
            }
        }
        
        return true;
    }

    bool RunDependenciesSteps(  const runcpp2::Data::Profile& profile,
                                const std::unordered_map<   PlatformName, 
                                                            runcpp2::Data::DependencyCommands> steps,
                                const std::string& dependenciesCopiedDirectory,
                                bool required)
    {
        ssLOG_FUNC_DEBUG();
        
        if(steps.empty())
            return true;
        
        //Find the platform name we use for setup
        PlatformName chosenPlatformName;
        if(!runcpp2::HasValueFromPlatformMap(steps))
        {
            if(required)
            {
                ssLOG_ERROR("Failed to find steps for current platform");
                return false;
            }
            else
                return true;
        }
        
        const runcpp2::Data::DependencyCommands& dependencySteps = 
            *runcpp2::GetValueFromPlatformMap(steps);
        
        std::string profileNameToUse;
        std::vector<std::string> currentProfileNames;
        profile.GetNames(currentProfileNames);
        
        for(int i = 0; i < currentProfileNames.size(); ++i)
        {
            if(dependencySteps.CommandSteps.count(currentProfileNames.at(i)) > 0)
            {
                profileNameToUse = currentProfileNames.at(i);
                break;
            }
        }
        
        if(profileNameToUse.empty())
        {
            ssLOG_ERROR("Failed to find steps for profile " << profile.Name << 
                        " for current platform");
            return false;
        }
        
        //Run the commands
        const std::vector<std::string>& commands = 
            dependencySteps.CommandSteps.at(profileNameToUse);
        
        for(int k = 0; k < commands.size(); ++k)
        {
            std::string processedDependencyPath = runcpp2::ProcessPath(dependenciesCopiedDirectory);
            ssLOG_INFO( "Running command: " << commands.at(k) << " in " << 
                        processedDependencyPath);
            
            int returnCode = 0;
            std::string output;
            if(!runcpp2::RunCommandAndGetOutput(commands.at(k), 
                                                output, 
                                                returnCode, 
                                                processedDependencyPath))
            {
                ssLOG_ERROR("Failed to run command with result: " << returnCode);
                ssLOG_ERROR("Output: \n" << output);
                return false;
            }
            else
                ssLOG_INFO("Output: \n" << output);
        }
        
        return true;
    }

    bool GetDependencyBinariesExtensionsToCopy( const runcpp2::Data::DependencyInfo& dependencyInfo,
                                                const runcpp2::Data::Profile& profile,
                                                std::vector<std::string>& outExtensionsToCopy)
    {
        static_assert((int)runcpp2::Data::DependencyLibraryType::COUNT == 4, "");
        switch(dependencyInfo.LibraryType)
        {
            case runcpp2::Data::DependencyLibraryType::STATIC:
            {
                if(!runcpp2::HasValueFromPlatformMap(profile.FilesTypes.StaticLinkFile.Extension))
                {
                    ssLOG_ERROR("Failed to find static library extensions for dependency " << 
                                dependencyInfo.Name);
                    
                    return false;
                }
                
                outExtensionsToCopy.push_back(
                    *runcpp2::GetValueFromPlatformMap(profile   .FilesTypes
                                                                .StaticLinkFile
                                                                .Extension));
                
                break;
            }
            case runcpp2::Data::DependencyLibraryType::SHARED:
            {
                if( !runcpp2::HasValueFromPlatformMap(profile.FilesTypes.SharedLinkFile.Extension) ||
                    !runcpp2::HasValueFromPlatformMap(profile.FilesTypes.SharedLibraryFile.Extension))
                {
                    ssLOG_ERROR("Failed to find shared library extensions for dependency " << 
                                dependencyInfo.Name);
                    
                    return false;
                }
                
                outExtensionsToCopy.push_back(
                    *runcpp2::GetValueFromPlatformMap(profile.FilesTypes.SharedLinkFile.Extension));
                
                if( *runcpp2::GetValueFromPlatformMap(profile.FilesTypes.SharedLinkFile.Extension) != 
                    *runcpp2::GetValueFromPlatformMap(profile.FilesTypes.SharedLibraryFile.Extension))
                {
                    outExtensionsToCopy.push_back(
                        *runcpp2::GetValueFromPlatformMap(profile   .FilesTypes
                                                                    .SharedLibraryFile
                                                                    .Extension));
                }
               
                break;
            }
            case runcpp2::Data::DependencyLibraryType::OBJECT:
            {
                if(!runcpp2::HasValueFromPlatformMap(profile.FilesTypes.ObjectLinkFile.Extension))
                {
                    ssLOG_ERROR("Failed to find shared library extensions for dependency " << 
                                dependencyInfo.Name);
                    
                    return false;
                }
                
                outExtensionsToCopy.push_back(
                    *runcpp2::GetValueFromPlatformMap(profile.FilesTypes.ObjectLinkFile.Extension));
                
                break;
            }
            case runcpp2::Data::DependencyLibraryType::HEADER:
                break;
            default:
                ssLOG_ERROR("Invalid library type: " << (int)dependencyInfo.LibraryType);
                return false;
        }
        
        return true;
    }
}

bool runcpp2::GetDependenciesPaths( const std::vector<Data::DependencyInfo*>& availableDependencies,
                                    std::vector<std::string>& outCopiesPaths,
                                    std::vector<std::string>& outSourcesPaths,
                                    const ghc::filesystem::path& scriptPath,
                                    const ghc::filesystem::path& buildDir)
{
    ssLOG_FUNC_DEBUG();

    ghc::filesystem::path scriptDirectory = scriptPath.parent_path();
    
    for(int i = 0; i < availableDependencies.size(); ++i)
    {
        const Data::DependencySource& currentSource = availableDependencies.at(i)->Source;
        
        static_assert((int)Data::DependencySourceType::COUNT == 2, "");
        
        switch(currentSource.Type)
        {
            case Data::DependencySourceType::GIT:
            {
                size_t lastSlashFoundIndex = currentSource.Value.find_last_of("/");
                size_t lastDotGitFoundIndex = currentSource.Value.find_last_of(".git");
                
                if( lastSlashFoundIndex == std::string::npos || 
                    lastDotGitFoundIndex == std::string::npos ||
                    lastDotGitFoundIndex < lastSlashFoundIndex)
                {
                    ssLOG_ERROR("Invalid git url: " << currentSource.Value);
                    return false;
                }
                else
                {
                    std::string gitRepoName = 
                                                    //+1 for / to not include it
                        currentSource.Value.substr( lastSlashFoundIndex + 1, 
                                                    //-1 for slash
                                                    lastDotGitFoundIndex - 1 -
                                                    //-(size - 1) for .git
                                                    (std::string(".git").size() - 1) -
                                                    lastSlashFoundIndex);
                    
                    outCopiesPaths.push_back(buildDir / gitRepoName);
                    outSourcesPaths.push_back("");
                }
                break;
            }
            
            case Data::DependencySourceType::LOCAL:
            {
                std::string localDepDirectoryName;
                std::string curPath = currentSource.Value;
                
                if(curPath.back() == '/')
                    curPath.pop_back();
                
                localDepDirectoryName = ghc::filesystem::path(curPath).filename().string();
                outCopiesPaths.push_back(buildDir / localDepDirectoryName);
                
                if(ghc::filesystem::path(curPath).is_relative())
                    outSourcesPaths.push_back(scriptDirectory / currentSource.Value);
                else
                    outSourcesPaths.push_back(currentSource.Value);
                
                break;
            }
            
            case Data::DependencySourceType::COUNT:
                return false;
        }
    }
    
    return true;
}

bool runcpp2::IsDependencyAvailableForThisPlatform(const Data::DependencyInfo& dependency)
{
    std::vector<std::string> platformNames = GetPlatformNames();
    
    for(int i = 0; i < platformNames.size(); ++i)
    {
        if( dependency.Platforms.find(platformNames.at(i)) != 
            dependency.Platforms.end())
        {
            return true;
        }
    }
    
    return false;
}

bool runcpp2::CleanupDependencies(  const runcpp2::Data::Profile& profile,
                                    const Data::ScriptInfo& scriptInfo,
                                    const std::vector<Data::DependencyInfo*>& availableDependencies,
                                    const std::vector<std::string>& dependenciesLocalCopiesPaths)
{
    ssLOG_FUNC_DEBUG();
    
    //If the script info is not populated (i.e. empty script info), don't do anything
    if(!scriptInfo.Populated)
        return true;

    assert(availableDependencies.size() == dependenciesLocalCopiesPaths.size());
    for(int i = 0; i < availableDependencies.size(); ++i)
    {
        std::error_code e;
        ssLOG_INFO("Running cleanup commands for " << availableDependencies.at(i)->Name);
        
        if(!RunDependenciesSteps(   profile, 
                                    availableDependencies.at(i)->Cleanup, 
                                    dependenciesLocalCopiesPaths.at(i),
                                    false))
        {
            ssLOG_ERROR("Failed to cleanup dependency " << availableDependencies.at(i)->Name);
            return false;
        }
        
        //Remove the directory
        if( ghc::filesystem::exists(dependenciesLocalCopiesPaths.at(i), e) &&
            !ghc::filesystem::remove_all(dependenciesLocalCopiesPaths.at(i), e))
        {
            ssLOG_ERROR("Failed to reset dependency directory: " << 
                        dependenciesLocalCopiesPaths.at(i));
            
            return false;
        }
    }
    
    return true;
}

bool runcpp2::SetupDependenciesIfNeeded(const runcpp2::Data::Profile& profile,
                                        const ghc::filesystem::path& buildDir,
                                        const Data::ScriptInfo& scriptInfo,
                                        std::vector<Data::DependencyInfo*>& availableDependencies,
                                        const std::vector<std::string>& dependenciesLocalCopiesPaths,
                                        const std::vector<std::string>& dependenciesSourcePaths)
{
    ssLOG_FUNC_DEBUG();

    //If the script info is not populated (i.e. empty script info), don't do anything
    if(!scriptInfo.Populated)
        return true;

    std::vector<bool> prePolulatedDependencies;
    
    //Clone/copy the dependencies if needed
    if(!PopulateLocalDependencies(  availableDependencies, 
                                    dependenciesLocalCopiesPaths, 
                                    dependenciesSourcePaths, 
                                    buildDir,
                                    prePolulatedDependencies))
    {
        return false;
    }
    
    if(!PopulateAbsoluteIncludePaths(availableDependencies, dependenciesLocalCopiesPaths))
    {
        return false;
    }
    
    //Run setup steps
    for(int i = 0; i < availableDependencies.size(); ++i)
    {
        //Don't run setup if the dependency is already setup in previous runs
        if(prePolulatedDependencies.at(i))
        {
            ssLOG_INFO("Skip running setup commands for " << availableDependencies.at(i)->Name);
            continue;
        }
        
        ssLOG_INFO("Running setup commands for " << availableDependencies.at(i)->Name);
        if(!RunDependenciesSteps(   profile, 
                                    availableDependencies.at(i)->Setup, 
                                    dependenciesLocalCopiesPaths.at(i),
                                    true))
        {
            ssLOG_ERROR("Failed to setup dependency " << availableDependencies.at(i)->Name);
            return false;
        }
    }

    return true;
}

bool runcpp2::BuildDependencies(const runcpp2::Data::Profile& profile,
                                const Data::ScriptInfo& scriptInfo,
                                const std::vector<Data::DependencyInfo*>& availableDependencies,
                                const std::vector<std::string>& dependenciesLocalCopiesPaths)
{
    ssLOG_FUNC_DEBUG();

    //If the script info is not populated (i.e. empty script info), don't do anything
    if(!scriptInfo.Populated)
        return true;

    //Run build steps
    for(int i = 0; i < availableDependencies.size(); ++i)
    {
        ssLOG_INFO("Running build commands for " << availableDependencies.at(i)->Name);
        
        if(!RunDependenciesSteps(   profile, 
                                    availableDependencies.at(i)->Build, 
                                    dependenciesLocalCopiesPaths.at(i),
                                    true))
        {
            ssLOG_ERROR("Failed to build dependency " << availableDependencies.at(i)->Name);
            return false;
        }
    }

    return true;
}

bool runcpp2::CopyDependenciesBinaries( const ghc::filesystem::path& buildDir,
                                        const std::vector<Data::DependencyInfo*>& 
                                            availableDependencies,
                                        const std::vector<std::string>& dependenciesCopiesPaths,
                                        const Data::Profile& profile,
                                        std::vector<std::string>& outCopiedBinariesPaths)
{
    std::vector<std::string> platformNames = GetPlatformNames();
    
    int minimumDependenciesCopiesCount = 0;
    for(int i = 0; i < availableDependencies.size(); ++i)
    {
        if(availableDependencies.at(i)->LibraryType != runcpp2::Data::DependencyLibraryType::HEADER)
            ++minimumDependenciesCopiesCount;
    }

    if(minimumDependenciesCopiesCount > dependenciesCopiesPaths.size())
    {
        ssLOG_ERROR("The amount of available dependencies do not match" <<
                    " the amount of dependencies copies paths");
        return false;
    }
    
    for(int i = 0; i < availableDependencies.size(); ++i)
    {
        std::vector<std::string> currentProfileNames;
        profile.GetNames(currentProfileNames);
        
        if(runcpp2::HasValueFromPlatformMap(availableDependencies.at(i)->FilesToCopy))
        {
            const runcpp2::Data::FilesToCopyInfo& filesToCopy = 
                *runcpp2::GetValueFromPlatformMap(availableDependencies.at(i)->FilesToCopy);
            
            std::string profileNameToUse;
            for(int j = 0; j < currentProfileNames.size(); ++j)
            {
                if( filesToCopy.ProfileFiles.find(currentProfileNames.at(j)) != 
                    filesToCopy.ProfileFiles.end())
                {
                    profileNameToUse = currentProfileNames.at(j);
                    break;
                }
            }
            
            if(!profileNameToUse.empty())
            {
                const std::vector<std::string>& filesToCopyForProfile = 
                    filesToCopy.ProfileFiles.at(profileNameToUse);
                
                for(int j = 0; j < filesToCopyForProfile.size(); ++j)
                {
                    ghc::filesystem::path srcPath = 
                        ghc::filesystem::path(dependenciesCopiesPaths.at(i)) / 
                        filesToCopyForProfile.at(j);
                    
                    ghc::filesystem::path destPath = 
                        buildDir / ghc::filesystem::path(filesToCopyForProfile.at(j)).filename();
                    
                    std::error_code e;
                    //TODO: Maybe we can check if destPath timestamp is newer and avoid copy?
                    if(ghc::filesystem::exists(srcPath, e))
                    {
                        ghc::filesystem::copy(  srcPath, 
                                                destPath, 
                                                ghc::filesystem::copy_options::overwrite_existing, 
                                                e);
                        
                        if(e)
                        {
                            ssLOG_ERROR("Failed to copy file from " << srcPath.string() << 
                                        " to " << destPath.string());
                            ssLOG_ERROR("Error: " << e.message());
                            return false;
                        }
                        
                        ssLOG_INFO("Copied from " << srcPath.string());
                        ssLOG_INFO("Copied to " << destPath.string());
                        outCopiedBinariesPaths.push_back(runcpp2::ProcessPath(destPath));
                    }
                    else
                        ssLOG_WARNING("File not found: " << srcPath.string());
                }
            }
        }

        std::vector<std::string> extensionsToCopy;
        
        //Get all the file extensions to copy
        {
            if(!GetDependencyBinariesExtensionsToCopy(  *availableDependencies.at(i),
                                                        profile,
                                                        extensionsToCopy))
            {
                return false;
            }
        
            for(int j = 0; j < platformNames.size(); ++j)
            {
                if( profile.FilesTypes.DebugSymbolFile.Extension.find(platformNames.at(j)) == 
                    profile.FilesTypes.DebugSymbolFile.Extension.end())
                {
                    continue;
                }
                
                extensionsToCopy.push_back(profile.FilesTypes.DebugSymbolFile.Extension.at(platformNames.at(j)));
                break;
            }
        }
        
        if(availableDependencies.at(i)->LibraryType == Data::DependencyLibraryType::HEADER)
            return true;
        
        //Get the Search path and search library name
        using PropertyMap = std::unordered_map<ProfileName, Data::DependencyLinkProperty>;
        const PropertyMap& linkProperties = availableDependencies.at(i)->LinkProperties;
        
        //See if we can find the link properties with the profile name
        auto foundPropertyIt = linkProperties.end();
        for(int j = 0; j < currentProfileNames.size(); ++j)
        {
            foundPropertyIt = linkProperties.find(currentProfileNames.at(j));
            if(foundPropertyIt != linkProperties.end())
                break;
        }
        
        if(foundPropertyIt == linkProperties.end())
        {
            ssLOG_ERROR("Search properties for dependency " << availableDependencies.at(i)->Name <<
                        " is missing profile " << profile.Name);
            
            return false;
        }
        const Data::DependencyLinkProperty& searchProperty = foundPropertyIt->second;
        
        //Copy the files with extensions that contains the search name if needed
        for(int j = 0; j < searchProperty.SearchLibraryNames.size(); ++j)
        {
            for(int k = 0; k < searchProperty.SearchDirectories.size(); ++k)
            {
                std::string currentSearchLibraryName = searchProperty.SearchLibraryNames.at(j);
                std::string currentSearchDirectory = searchProperty.SearchDirectories.at(k);
            
                if(!ghc::filesystem::path(currentSearchDirectory).is_absolute())
                {
                    currentSearchDirectory =    dependenciesCopiesPaths.at(i) + "/" + 
                                                currentSearchDirectory;
                }
            
                ssLOG_DEBUG("currentSearchDirectory: " << currentSearchDirectory);
                ssLOG_DEBUG("currentSearchLibraryName: " << currentSearchLibraryName);
            
                std::error_code e;
                if( !ghc::filesystem::exists(currentSearchDirectory, e) || 
                    !ghc::filesystem::is_directory(currentSearchDirectory, e))
                {
                    ssLOG_INFO("Invalid search path: " << currentSearchDirectory);
                    continue;
                }
            
                //Iterate each files in the directory we are searching
                for(auto it : ghc::filesystem::directory_iterator(currentSearchDirectory, e))
                {
                    if(it.is_directory())
                        continue;
                    
                    std::string currentFileName = it.path().stem().string();
                    std::string currentExtension = it.path().extension().string();
                    
                    ssLOG_DEBUG("currentFileName: " << currentFileName);
                    ssLOG_DEBUG("currentExtension: " << currentExtension);
                    
                    //TODO: Make it not case sensitive?
                    bool nameMatched = false;
                    if(currentFileName.find(currentSearchLibraryName) != std::string::npos)
                        nameMatched = true;
                    
                    for(int j = 0; j < searchProperty.ExcludeLibraryNames.size(); ++j)
                    {
                        std::string currentExcludeLibraryName = 
                            searchProperty.ExcludeLibraryNames.at(j);
                        
                        if(currentFileName.find(currentExcludeLibraryName) != std::string::npos)
                        {
                            nameMatched = false;
                            break;
                        }
                    }
                    
                    if(!nameMatched)
                        continue;
                    
                    bool extensionMatched = false;
                    
                    for(int j = 0; j < extensionsToCopy.size(); ++j)
                    {
                        if(currentExtension == extensionsToCopy.at(j))
                        {
                            extensionMatched = true;
                            break;
                        }
                    }
                    
                    if(!extensionMatched)
                        continue;
                    
                    //TODO: Group object files in folders to avoid name collision
                    ghc::filesystem::path copiedPath = buildDir / it.path().filename();
                    
                    //Check if we have previously copied the dependencies to the folder
                    if(ghc::filesystem::exists(copiedPath, e))
                    {
                        ghc::filesystem::file_time_type builtWriteTime = 
                            ghc::filesystem::last_write_time(it.path(), e);
                        
                        if(e)
                        {
                            ssLOG_ERROR("Failed to get write time of " << it.path().string());
                            ssLOG_ERROR("Error: " << e.message());
                            return false;
                        }
                        
                        ghc::filesystem::file_time_type copiedWriteTime = 
                            ghc::filesystem::last_write_time(copiedPath, e);
                        
                        if(e)
                        {
                            ssLOG_ERROR("Failed to get write time of " << copiedPath.string());
                            ssLOG_ERROR("Error: " << e.message());
                            return false;
                        }
                        
                        //If the copied write time is newer than the original one, that means 
                        //  the copy is up to date
                        if(builtWriteTime <= copiedWriteTime)
                        {
                            ssLOG_INFO(copiedPath.string() << " is up to date");
                            outCopiedBinariesPaths.push_back(runcpp2::ProcessPath(copiedPath));
                            continue;
                        }
                        else
                            ssLOG_INFO(copiedPath.string() << " is outdated");
                    }
                    
                    //Copy the dependency binary to our folder
                    std::error_code copyErrorCode;
                    ghc::filesystem::copy(  it.path(), 
                                            buildDir, 
                                            ghc::filesystem::copy_options::overwrite_existing,  
                                            copyErrorCode);
                    
                    if(copyErrorCode)
                    {
                        ssLOG_ERROR("Failed to copy file from " << it.path().string() << 
                                    " to " << buildDir.string());

                        ssLOG_ERROR("Error: " << copyErrorCode.message());
                        return false;
                    }
                    
                    
                    ssLOG_INFO("Copied from " << it.path().string());
                    ssLOG_INFO("Copied to " << copiedPath.string());
                    outCopiedBinariesPaths.push_back(runcpp2::ProcessPath(copiedPath));
                }
            }
        }
    }
    
    //Do a check to see if any dependencies are copied
    if(outCopiedBinariesPaths.size() < minimumDependenciesCopiesCount)
    {
        ssLOG_WARNING("outCopiedBinariesPaths.size() does not match minimumDependenciesCopiesCount");
        ssLOG_WARNING("outCopiedBinariesPaths are");
        
        for(int i = 0; i < outCopiedBinariesPaths.size(); ++i)
            ssLOG_WARNING("outCopiedBinariesPaths[" << i << "]: " << outCopiedBinariesPaths.at(i));
    }
    
    return true;
}
