#include "runcpp2/DependenciesHelper.hpp"
#include "runcpp2/PlatformUtil.hpp"
#include "runcpp2/StringUtil.hpp"
#include "runcpp2/ParseUtil.hpp"

#include "ghc/filesystem.hpp"
#include "ssLogger/ssLog.hpp"

#include <unordered_set>
#include <future>
#include <chrono>

namespace
{
    bool GetDependencyPath( const runcpp2::Data::DependencyInfo& dependency,
                            const ghc::filesystem::path& scriptPath,
                            const ghc::filesystem::path& buildDir,
                            ghc::filesystem::path& outCopyPath,
                            ghc::filesystem::path& outSourcePath)
    {
        ssLOG_FUNC_INFO();

        ghc::filesystem::path scriptDirectory = scriptPath.parent_path();
        
        const runcpp2::Data::DependencySource& currentSource = dependency.Source;

        if(mpark::get_if<runcpp2::Data::GitSource>(&currentSource.Source))
        {
            const runcpp2::Data::GitSource* git = 
                mpark::get_if<runcpp2::Data::GitSource>(&currentSource.Source);
            
            size_t lastSlashFoundIndex = git->URL.find_last_of("/");
            size_t lastDotGitFoundIndex = git->URL.find_last_of(".git");
            
            if( lastSlashFoundIndex == std::string::npos || 
                lastDotGitFoundIndex == std::string::npos ||
                lastDotGitFoundIndex < lastSlashFoundIndex)
            {
                ssLOG_ERROR("Invalid git url: " << git->URL);
                return false;
            }
            else
            {
                std::string gitRepoName = 
                                    //+1 for / to not include it
                    git->URL.substr(lastSlashFoundIndex + 1, 
                                    //-1 for slash
                                    lastDotGitFoundIndex - 1 - 
                                    //-(size - 1) for .git
                                    (std::string(".git").size() - 1) -
                                    lastSlashFoundIndex);
                
                outCopyPath = (buildDir / gitRepoName);
                outSourcePath.clear();
            }
        }
        else if(mpark::get_if<runcpp2::Data::LocalSource>(&currentSource.Source))
        {
            const runcpp2::Data::LocalSource* local = 
                mpark::get_if<runcpp2::Data::LocalSource>(&currentSource.Source);
            
            std::string localDepDirectoryName;
            std::string curPath = local->Path;
            
            if(curPath.back() == '/')
                curPath.pop_back();
            
            localDepDirectoryName = ghc::filesystem::path(curPath).filename().string();
            
            if(ghc::filesystem::path(curPath).is_relative())
                outSourcePath = (scriptDirectory / local->Path);
            else
                outSourcePath = (local->Path);

            if(!ghc::filesystem::is_directory(outSourcePath))
            {
                ssLOG_ERROR("Local dependency path is not a directory: " << outSourcePath);
                return false;
            }

            if(currentSource.ImportPath.empty())
                outCopyPath = (buildDir / localDepDirectoryName);
            else
                outCopyPath = outSourcePath;
        }
        
        return true;
    }
    
    bool PopulateLocalDependency(   const runcpp2::Data::DependencyInfo& dependency,
                                    const ghc::filesystem::path& copyPath,
                                    const ghc::filesystem::path& sourcePath,
                                    const ghc::filesystem::path& buildDir,
                                    bool& outPrePopulated)
    {
        ssLOG_FUNC_DEBUG();
        
        std::error_code e;
        
        if(ghc::filesystem::exists(copyPath, e))
        {
            if(!ghc::filesystem::is_directory(copyPath, e))
            {
                ssLOG_ERROR("Dependency path is a file: " << copyPath.string());
                ssLOG_ERROR("It should be a folder instead");
                return false;
            }
            outPrePopulated = true;
            return true;
        }
        else
        {
            if(mpark::get_if<runcpp2::Data::GitSource>(&(dependency.Source.Source)))
            {
                const runcpp2::Data::GitSource* git = 
                    mpark::get_if<runcpp2::Data::GitSource>(&(dependency.Source.Source));
                
                std::string submoduleString;
                static_assert(  static_cast<int>(runcpp2::Data::SubmoduleInitType::COUNT) == 3, 
                                "Add new type to be processed");
                switch(git->CurrentSubmoduleInitType)
                {
                    case runcpp2::Data::SubmoduleInitType::NONE:
                        break;
                    case runcpp2::Data::SubmoduleInitType::SHALLOW:
                        submoduleString = "--recursive --shallow-submodules ";
                        break;
                    case runcpp2::Data::SubmoduleInitType::FULL:
                        submoduleString = "--recursive ";
                        break;
                    default:
                    {
                        ssLOG_ERROR("Invalid git submodule init type: " << 
                                    static_cast<int>(git->CurrentSubmoduleInitType));
                        return false;
                    }
                }
                
                std::string gitCloneCommand =
                    std::string("git clone ") + 
                    submoduleString + 
                    (git->FullHistory ? "" : "--depth 1 ") + 
                    (
                        git->Branch.empty() ? 
                        std::string("") : 
                        std::string("--branch ") + git->Branch + " "
                    ) +
                    git->URL;
                
                ssLOG_INFO("Running git clone command: " << gitCloneCommand << " in " << 
                            buildDir.string());
                
                int returnCode = 0;
                std::string output;
                if(!runcpp2::RunCommand(gitCloneCommand, 
                                        false,
                                        buildDir.string(),
                                        output, 
                                        returnCode))
                {
                    ssLOG_ERROR("Failed to run git clone with result: " << returnCode);
                    ssLOG_ERROR("Was trying to run: " << gitCloneCommand);
                    //ssLOG_ERROR("Output: \n" << output);
                    return false;
                }
                //else
                //    ssLOG_INFO("Output: \n" << output);
            }
            else if(mpark::get_if<runcpp2::Data::LocalSource>(&(dependency.Source.Source)))
            {
                //Copy/link local directory if it doesn't have any import path
                if( dependency.Source.ImportPath.empty() &&
                    !runcpp2::SyncLocalDependency(dependency, sourcePath, copyPath))
                {
                        return false;
                }
            }
            
            outPrePopulated = false;
            return true;
        }
    }
    
    bool PopulateLocalDependencies( const std::vector<runcpp2::Data::DependencyInfo*>& dependencies,
                                    const std::vector<std::string>& dependenciesCopiesPaths,
                                    const std::vector<std::string>& dependenciesSourcesPaths,
                                    const ghc::filesystem::path& buildDir,
                                    std::vector<bool>& outPrePopulated)
    {
        ssLOG_FUNC_INFO();
        
        outPrePopulated.resize(dependencies.size());
        
        for(int i = 0; i < dependencies.size(); ++i)
        {
            if(!dependencies.at(i)->Source.ImportPath.empty())
            {
                ssLOG_ERROR("Dependency import not resolved before populating.");
                return false;
            }
            
            bool prePopulated = false;
            if(!PopulateLocalDependency(*dependencies.at(i),
                                        ghc::filesystem::path(dependenciesCopiesPaths.at(i)),
                                        ghc::filesystem::path(dependenciesSourcesPaths.at(i)),
                                        buildDir,
                                        prePopulated))
            {
                return false;
            }
            
            outPrePopulated.at(i) = prePopulated;
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
                                                            runcpp2::Data::ProfilesCommands> steps,
                                const std::string& dependenciesCopiedDirectory,
                                bool required)
    {
        ssLOG_FUNC_INFO();
        
        if(steps.empty())
            return true;
        
        //Find the platform name we use for setup
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
        
        const runcpp2::Data::ProfilesCommands& dependencySteps = 
            *runcpp2::GetValueFromPlatformMap(steps);
        
        const std::vector<std::string>* commands = 
            runcpp2::GetValueFromProfileMap(profile, dependencySteps.CommandSteps);
            
        if(!commands)
        {
            ssLOG_ERROR("Failed to find steps for profile " << profile.Name << 
                        " for current platform");
            return false;
        }
        
        //Run the commands
        for(int k = 0; k < commands->size(); ++k)
        {
            std::string processedDependencyPath = runcpp2::ProcessPath(dependenciesCopiedDirectory);
            ssLOG_INFO( "Running command: " << commands->at(k) << " in " << 
                        processedDependencyPath);
            
            int returnCode = 0;
            std::string output;
            
            if(!runcpp2::RunCommand(commands->at(k), 
                                    true,
                                    processedDependencyPath,
                                    output, 
                                    returnCode))
            {
                ssLOG_ERROR("Failed to run command with result: " << returnCode);
                ssLOG_ERROR("Was trying to run: " << commands->at(k));
                ssLOG_ERROR("Output: \n" << output);
                return false;
            }
            else
                ssLOG_INFO("Output: \n" << output);
        }
        
        return true;
    }

    bool GetDependencyBinariesExtensionsToLink( const runcpp2::Data::DependencyInfo& dependencyInfo,
                                                const runcpp2::Data::Profile& profile,
                                                std::vector<std::string>& outExtensionsToLink)
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
                
                outExtensionsToLink.push_back(
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
                
                outExtensionsToLink.push_back(
                    *runcpp2::GetValueFromPlatformMap(profile.FilesTypes.SharedLinkFile.Extension));
                
                if( *runcpp2::GetValueFromPlatformMap(profile.FilesTypes.SharedLinkFile.Extension) != 
                    *runcpp2::GetValueFromPlatformMap(profile.FilesTypes.SharedLibraryFile.Extension))
                {
                    outExtensionsToLink.push_back(
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
                    ssLOG_ERROR("Failed to find object file extensions for dependency " << 
                                dependencyInfo.Name);
                    
                    return false;
                }
                
                outExtensionsToLink.push_back(
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

    ghc::filesystem::path ResolveSymlink(const ghc::filesystem::path& path, std::error_code& ec)
    {
        ghc::filesystem::path resolvedPath = ghc::filesystem::canonical(path, ec);
        if(ec)
            return path; // Return original path if canonical fails
        
        return resolvedPath;
    }
}

bool runcpp2::GetDependenciesPaths( const std::vector<Data::DependencyInfo*>& availableDependencies,
                                    std::vector<std::string>& outCopiesPaths,
                                    std::vector<std::string>& outSourcesPaths,
                                    const ghc::filesystem::path& scriptPath,
                                    const ghc::filesystem::path& buildDir)
{
    ssLOG_FUNC_INFO();

    for(int i = 0; i < availableDependencies.size(); ++i)
    {
        ghc::filesystem::path currentCopyPath;
        ghc::filesystem::path currentSourcePath;
        
        if(!GetDependencyPath(  *availableDependencies.at(i),
                                scriptPath,
                                buildDir,
                                currentCopyPath,
                                currentSourcePath))
        {
            return false;
        }

        outCopiesPaths.push_back(currentCopyPath.string());
        outSourcesPaths.push_back(currentSourcePath.string());
    }
    
    return true;
}

bool runcpp2::IsDependencyAvailableForThisPlatform(const Data::DependencyInfo& dependency)
{
    std::vector<std::string> platformNames = GetPlatformNames();
    
    for(int i = 0; i < platformNames.size(); ++i)
    {
        if(dependency.Platforms.find(platformNames.at(i)) != dependency.Platforms.end())
            return true;
    }
    
    return false;
}

bool runcpp2::CleanupDependencies(  const runcpp2::Data::Profile& profile,
                                    const Data::ScriptInfo& scriptInfo,
                                    const std::vector<Data::DependencyInfo*>& availableDependencies,
                                    const std::vector<std::string>& dependenciesLocalCopiesPaths,
                                    const std::string& dependenciesToReset)
{
    ssLOG_FUNC_DEBUG();
    
    //If the script info is not populated (i.e. empty script info), don't do anything
    if(!scriptInfo.Populated)
        return true;

    assert(availableDependencies.size() == dependenciesLocalCopiesPaths.size());
    
    //Split dependency names if not "all"
    std::unordered_set<std::string> dependencyNames;
    if(dependenciesToReset != "all")
    {
        std::string currentName;
        for(int i = 0; i < dependenciesToReset.size(); ++i)
        {
            if(dependenciesToReset[i] == ',' || i == dependenciesToReset.size() - 1)
            {
                if(dependenciesToReset[i] != ',')
                    currentName += dependenciesToReset[i];
                
                if(!currentName.empty())
                {
                    runcpp2::Trim(currentName);
                    //Convert to lowercase for case-insensitive comparison
                    for(int j = 0; j < currentName.size(); ++j)
                        currentName[j] = std::tolower(currentName[j]);
                    
                    dependencyNames.insert(currentName);
                    currentName.clear();
                }
            }
            else
                currentName += dependenciesToReset[i];
        }
    }

    for(int i = 0; i < availableDependencies.size(); ++i)
    {
        //Skip if not in the list of dependencies to reset
        if(!dependencyNames.empty())
        {
            //Convert dependency name to lowercase for comparison
            std::string depName = availableDependencies.at(i)->Name;
            for(int j = 0; j < depName.size(); ++j)
                depName[j] = std::tolower(depName[j]);
            
            if(dependencyNames.count(depName) == 0)
            {
                ssLOG_DEBUG(availableDependencies.at(i)->Name << " not in list to remove");
                continue;
            }
        }

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
        
        ssLOG_DEBUG(availableDependencies.at(i)->Name << " removed");
    }
    
    return true;
}

bool runcpp2::SetupDependenciesIfNeeded(const runcpp2::Data::Profile& profile,
                                        const ghc::filesystem::path& buildDir,
                                        const Data::ScriptInfo& scriptInfo,
                                        std::vector<Data::DependencyInfo*>& availableDependencies,
                                        const std::vector<std::string>& dependenciesLocalCopiesPaths,
                                        const std::vector<std::string>& dependenciesSourcePaths,
                                        const int maxThreads)
{
    ssLOG_FUNC_INFO();

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
        return false;
    
    std::vector<std::future<bool>> actions;
    
    //Cache logs for worker threads
    ssLOG_ENABLE_CACHE_OUTPUT_FOR_NEW_THREADS();
    int logLevel = ssLOG_GET_CURRENT_THREAD_TARGET_LEVEL();
    
    //Run setup steps
    for(int i = 0; i < availableDependencies.size(); ++i)
    {
        //Don't run setup if the dependency is already setup in previous runs
        if(prePolulatedDependencies.at(i))
        {
            ssLOG_INFO("Skip running setup commands for " << availableDependencies.at(i)->Name);
            continue;
        }
        
        actions.emplace_back
        (
            std::async
            (
                std::launch::async,
                [i, &profile, &availableDependencies, &dependenciesLocalCopiesPaths, logLevel]()
                {
                    ssLOG_SET_CURRENT_THREAD_TARGET_LEVEL(logLevel);
                    
                    ssLOG_INFO("Running setup commands for " << availableDependencies.at(i)->Name);
                    if(!RunDependenciesSteps(   profile, 
                                                availableDependencies.at(i)->Setup, 
                                                dependenciesLocalCopiesPaths.at(i),
                                                true))
                    {
                        ssLOG_ERROR("Failed to setup dependency " << 
                                    availableDependencies.at(i)->Name);
                        return false;
                    }
                    return true;
                }
            )
        );
        
        //Evaluate the setup results for each batch
        if(actions.size() >= maxThreads || i == availableDependencies.size() - 1)
        {
            std::chrono::system_clock::time_point deadline = 
                std::chrono::system_clock::now() + std::chrono::seconds(60);
            for(int j = 0; j < actions.size(); ++j)
            {
                if(!actions.at(j).valid())
                {
                    ssLOG_ERROR("Failed to construct actions for setup");
                    ssLOG_OUTPUT_ALL_CACHE_GROUPED();
                    return false;
                }
                
                std::future_status actionStatus = actions.at(j).wait_until(deadline);
                if(actionStatus == std::future_status::ready)
                {
                    if(!actions.at(j).get())
                    {
                        ssLOG_ERROR("Setup failed for dependencies");
                        ssLOG_OUTPUT_ALL_CACHE_GROUPED();
                        return false;
                    }
                }
                else
                {
                    ssLOG_ERROR("Dependencies setup timeout");
                    ssLOG_OUTPUT_ALL_CACHE_GROUPED();
                    return false;
                }
            }
            actions.clear();
        }
    }
    
    ssLOG_OUTPUT_ALL_CACHE_GROUPED();
    return true;
}

bool runcpp2::BuildDependencies(const runcpp2::Data::Profile& profile,
                                const Data::ScriptInfo& scriptInfo,
                                const std::vector<Data::DependencyInfo*>& availableDependencies,
                                const std::vector<std::string>& dependenciesLocalCopiesPaths,
                                const int maxThreads)
{
    ssLOG_FUNC_INFO();

    //If the script info is not populated (i.e. empty script info), don't do anything
    if(!scriptInfo.Populated)
        return true;
    
    std::vector<std::future<bool>> actions;
    
    //Cache logs for worker threads
    ssLOG_ENABLE_CACHE_OUTPUT_FOR_NEW_THREADS();
    int logLevel = ssLOG_GET_CURRENT_THREAD_TARGET_LEVEL();
    
    //Run build steps
    for(int i = 0; i < availableDependencies.size(); ++i)
    {
        ssLOG_INFO("Running build commands for " << availableDependencies.at(i)->Name);
        
        actions.emplace_back
        (
            std::async
            (
                std::launch::async,
                [i, &profile, &availableDependencies, &dependenciesLocalCopiesPaths, logLevel]()
                {
                    ssLOG_SET_CURRENT_THREAD_TARGET_LEVEL(logLevel);
                    
                    if(!RunDependenciesSteps(   profile, 
                                                availableDependencies.at(i)->Build, 
                                                dependenciesLocalCopiesPaths.at(i),
                                                true))
                    {
                        ssLOG_ERROR("Failed to build dependency " << availableDependencies.at(i)->Name);
                        return false;
                    }
                    return true;
                }
            )
        );
        
        //Evaluate the setup results for each batch
        if(actions.size() >= maxThreads || i == availableDependencies.size() - 1)
        {
            std::chrono::system_clock::time_point deadline = 
                std::chrono::system_clock::now() + std::chrono::seconds(60);
            for(int j = 0; j < actions.size(); ++j)
            {
                if(!actions.at(j).valid())
                {
                    ssLOG_ERROR("Failed to construct actions for building dependencies");
                    ssLOG_OUTPUT_ALL_CACHE_GROUPED();
                    return false;
                }
                
                std::future_status actionStatus = actions.at(j).wait_until(deadline);
                if(actionStatus == std::future_status::ready)
                {
                    if(!actions.at(j).get())
                    {
                        ssLOG_ERROR("Build failed for dependencies");
                        ssLOG_OUTPUT_ALL_CACHE_GROUPED();
                        return false;
                    }
                }
                else
                {
                    ssLOG_ERROR("Dependencies build timeout");
                    ssLOG_OUTPUT_ALL_CACHE_GROUPED();
                    return false;
                }
            }
            actions.clear();
        }
    }

    ssLOG_OUTPUT_ALL_CACHE_GROUPED();
    return true;
}

bool runcpp2::GatherDependenciesBinaries(   const std::vector<Data::DependencyInfo*>& 
                                                availableDependencies,
                                            const std::vector<std::string>& dependenciesCopiesPaths,
                                            const Data::Profile& profile,
                                            std::vector<std::string>& outBinariesPaths)
{
    ssLOG_FUNC_DEBUG();

    
    std::unordered_set<std::string> binariesPathsSet;
    for(int i = 0; i < outBinariesPaths.size(); ++i)
        binariesPathsSet.insert(outBinariesPaths[i]);
    
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
    
    int nonLinkFilesCount = 0;
    for(int i = 0; i < availableDependencies.size(); ++i)
    {
        ssLOG_INFO("Evaluating dependency " << availableDependencies.at(i)->Name);
        
        if(runcpp2::HasValueFromPlatformMap(availableDependencies.at(i)->FilesToCopy))
        {
            const runcpp2::Data::FilesToCopyInfo& filesToCopy = 
                *runcpp2::GetValueFromPlatformMap(availableDependencies.at(i)->FilesToCopy);
            
            const std::vector<std::string>* filesToGatherForProfile = 
                runcpp2::GetValueFromProfileMap(profile, filesToCopy.ProfileFiles);
            
            if(filesToGatherForProfile)
            {
                for(int j = 0; j < filesToGatherForProfile->size(); ++j)
                {
                    ghc::filesystem::path srcPath = 
                        ghc::filesystem::path(dependenciesCopiesPaths.at(i)) / 
                        filesToGatherForProfile->at(j);
                    
                    std::error_code e;
                    if(ghc::filesystem::exists(srcPath, e))
                    {
                        const std::string processedSrcPath = runcpp2::ProcessPath(srcPath);
                        outBinariesPaths.push_back(processedSrcPath);
                        binariesPathsSet.insert(processedSrcPath);
                        ++nonLinkFilesCount;
                        ssLOG_INFO("Added binary path: " << srcPath.string());
                    }
                    else
                        ssLOG_WARNING("File not found: " << srcPath.string());
                }
            }
        }

        std::vector<std::string> extensionsToLink;
        
        //Get all the file extensions to gather
        {
            if(!GetDependencyBinariesExtensionsToLink(  *availableDependencies.at(i),
                                                        profile,
                                                        extensionsToLink))
            {
                return false;
            }
            
            const std::string* debugSymbolExt = 
                runcpp2::GetValueFromPlatformMap(profile.FilesTypes.DebugSymbolFile.Extension);
                
            if(debugSymbolExt)
                extensionsToLink.push_back(*debugSymbolExt);
        }
        
        if(availableDependencies.at(i)->LibraryType == Data::DependencyLibraryType::HEADER)
            continue;
        
        //Get the Search path and search library name
        using PropertyMap = std::unordered_map<ProfileName, Data::DependencyLinkProperty>;
        const PropertyMap& linkProperties = availableDependencies.at(i)->LinkProperties;
        
        if(!runcpp2::HasValueFromPlatformMap(linkProperties))
        {
            ssLOG_ERROR("Link properties for dependency " << availableDependencies.at(i)->Name <<
                        " is missing for the current platform");
            return false;
        }

        const Data::DependencyLinkProperty& linkProperty = 
            *runcpp2::GetValueFromPlatformMap(linkProperties);

        const Data::ProfileLinkProperty* profileLinkProperty = 
            runcpp2::GetValueFromProfileMap(profile, linkProperty.ProfileProperties);
            
        if(!profileLinkProperty)
            continue;

        for(int searchLibIndex = 0; 
            searchLibIndex < profileLinkProperty->SearchLibraryNames.size(); 
            ++searchLibIndex)
        {
            for(int searchDirIndex = 0; 
                searchDirIndex < profileLinkProperty->SearchDirectories.size(); 
                ++searchDirIndex)
            {
                std::string currentSearchLibraryName = 
                    profileLinkProperty->SearchLibraryNames.at(searchLibIndex);
                std::string currentSearchDirectory = 
                    profileLinkProperty->SearchDirectories.at(searchDirIndex);

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
                    
                    std::string currentFileName = it.path().filename().string();
                    std::string currentExtension = runcpp2::GetFileExtensionWithoutVersion(it.path());
                    
                    ssLOG_DEBUG("currentFileName: " << currentFileName);
                    ssLOG_DEBUG("currentExtension: " << currentExtension);
                    
                    //TODO: Make it not case sensitive?
                    bool nameMatched = false;
                    if(currentFileName.find(currentSearchLibraryName) != std::string::npos)
                        nameMatched = true;
                    
                    for(int excludeIndex = 0; 
                        excludeIndex < profileLinkProperty->ExcludeLibraryNames.size(); 
                        ++excludeIndex)
                    {
                        std::string currentExcludeLibraryName = 
                            profileLinkProperty->ExcludeLibraryNames.at(excludeIndex);
                        
                        if(currentFileName.find(currentExcludeLibraryName) != std::string::npos)
                        {
                            nameMatched = false;
                            break;
                        }
                    }
                    
                    if(!nameMatched)
                        continue;
                    
                    bool extensionMatched = false;
                    for(int extIndex = 0; extIndex < extensionsToLink.size(); ++extIndex)
                    {
                        if(currentExtension == extensionsToLink.at(extIndex))
                        {
                            extensionMatched = true;
                            break;
                        }
                    }
                    
                    if(!extensionMatched)
                        continue;
                    
                    //Handle symlink
                    ghc::filesystem::path resolvedPath = it.path();
                    {
                        std::error_code symlink_ec;
                        resolvedPath = ResolveSymlink(resolvedPath, symlink_ec);
                        if(symlink_ec)
                        {
                            ssLOG_ERROR("Failed to resolve symlink: " << symlink_ec.message());
                            return false;
                        }
                    }
                    
                    const std::string processedPath = runcpp2::ProcessPath(it.path().string());
                    const std::string processedResolvedPath = 
                        runcpp2::ProcessPath(resolvedPath.string());
                    
                    if(binariesPathsSet.count(processedResolvedPath) == 0)
                    {
                        ssLOG_INFO("Linking " << processedPath);
                        outBinariesPaths.push_back(processedPath);
                        binariesPathsSet.insert(processedResolvedPath);
                    }
                }
            }
        }
    }
    
    //Do a check to see if any dependencies are copied
    if(outBinariesPaths.size() - nonLinkFilesCount < minimumDependenciesCopiesCount)
    {
        ssLOG_WARNING("We could missing some link files for dependencies");
        
        for(int i = 0; i < outBinariesPaths.size(); ++i)
            ssLOG_WARNING("outBinariesPaths[" << i << "]: " << outBinariesPaths.at(i));
    }
    
    return true;
}


bool runcpp2::HandleImport( Data::DependencyInfo& dependency,
                            const ghc::filesystem::path& basePath)
{
    ssLOG_FUNC_DEBUG();
    
    if(dependency.Source.ImportPath.empty())
        return true;

    const std::string fullPath = (basePath / dependency.Source.ImportPath).string();
    std::error_code ec;
    if(!ghc::filesystem::exists(fullPath, ec))
    {
        ssLOG_ERROR("Import file not found: " << fullPath);
        return false;
    }

    if(ghc::filesystem::is_directory(fullPath))
    {
        ssLOG_ERROR("Import path is a directory: " << fullPath);
        return false;
    }

    //Parse the YAML file
    ryml::ConstNodeRef resolvedTree;
    ryml::Tree importedTree;
    std::string content;
    {
        std::ifstream file(fullPath);
        if(!file.is_open())
        {
            ssLOG_ERROR("Failed to open import file: " << fullPath);
            return false;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        content = buffer.str();

        importedTree = ryml::parse_in_place(c4::to_substr(content));
        
        if(!runcpp2::ResolveYAML_Stream(importedTree, resolvedTree))
            return false;
    }

    //Store the imported sources as copies for traciblity if needed
    std::vector<std::shared_ptr<Data::DependencySource>> previouslyImportedSources;
    {
        std::shared_ptr<Data::DependencySource> currentImportSource = 
            std::make_shared<Data::DependencySource>(dependency.Source);

        previouslyImportedSources = dependency.Source.ImportedSources;
        
        dependency.Source.ImportedSources.clear();
        currentImportSource->ImportedSources.clear();
        
        previouslyImportedSources.push_back(currentImportSource);
    }
    
    //Reset the current dependency before we parse the import dependency
    dependency = Data::DependencyInfo();
    
    //Parse the imported dependency
    if(!dependency.ParseYAML_Node(resolvedTree))
    {
        ssLOG_ERROR("Failed to parse imported dependency: " << fullPath);
        
        //Print the list of imported sources
        for(int i = 0; i < previouslyImportedSources.size(); ++i)
        {
            ssLOG_ERROR("Imported source[" << i << "]: " << 
                        previouslyImportedSources.at(i)->ImportPath.string());
        }

        return false;
    }

    dependency.Source.ImportedSources = previouslyImportedSources;
    return true;
}

bool runcpp2::ResolveImports(   Data::ScriptInfo& scriptInfo,
                                const ghc::filesystem::path& scriptPath,
                                const ghc::filesystem::path& buildDir)
{
    ssLOG_FUNC_INFO();
    INTERNAL_RUNCPP2_SAFE_START();
    
    //For each dependency, check if import path exists
    for(int i = 0; i < scriptInfo.Dependencies.size(); ++i)
    {
        Data::DependencyInfo& dependency = scriptInfo.Dependencies.at(i);
        
        //Check if import path exists
        Data::DependencySource& source = dependency.Source;
        if(source.ImportPath.empty())
            continue;
        
        if(!source.ImportPath.is_relative())
        {
            ssLOG_ERROR("Import path is not relative: " << source.ImportPath.string());
            return false;
        }

        ghc::filesystem::path copyPath;
        ghc::filesystem::path sourcePath;
        
        if(!GetDependencyPath(dependency, scriptPath, buildDir, copyPath, sourcePath))
            return false;

        bool prePopulated = false;
        if(!PopulateLocalDependency(dependency, copyPath, sourcePath, buildDir, prePopulated))
            return false;
        
        //Parse the import file
        if(!HandleImport(dependency, copyPath))
            return false;

        //Check do we still have import path in the dependency. If so, we need to parse it again
        if(!dependency.Source.ImportPath.empty())
            --i;
    }

    return true;
    
    INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
}

bool runcpp2::SyncLocalDependency(  const Data::DependencyInfo& dependency,
                                    const ghc::filesystem::path& sourcePath,
                                    const ghc::filesystem::path& copyPath)
{
    ssLOG_FUNC_DEBUG();
    std::error_code ec;

    //Only sync if it's a local dependency
    const Data::LocalSource* local = mpark::get_if<Data::LocalSource>(&dependency.Source.Source);
    if(!local)
    {
        ssLOG_DEBUG("Not a local dependency, skipping sync");
        return true;
    }

    //Fail if source path doesn't exist
    if(!ghc::filesystem::exists(sourcePath, ec))
    {
        ssLOG_ERROR("Source path does not exist: " << sourcePath.string());
        return false;
    }

    //Create target directory if it doesn't exist
    if(!ghc::filesystem::exists(copyPath, ec))
    {
        if(!ghc::filesystem::create_directory(copyPath, ec))
        {
            ssLOG_ERROR("Failed to create directory " << copyPath.string() << ": " << ec.message());
            return false;
        }
    }

    //Get list of files in source
    std::unordered_set<std::string> sourceFiles;
    for(const ghc::filesystem::directory_entry& entry : 
        ghc::filesystem::directory_iterator(sourcePath, ec))
    {
        sourceFiles.insert(entry.path().filename().string());
    }

    //First pass: Check existing files in target
    for(const ghc::filesystem::directory_entry& entry : 
        ghc::filesystem::directory_iterator(copyPath, ec))
    {
        const ghc::filesystem::path& targetPath = entry.path();
        const ghc::filesystem::path& srcPath = sourcePath / targetPath.filename();
        bool needsUpdate = false;

        //Check if this is a symlink
        if(ghc::filesystem::is_symlink(targetPath, ec))
        {
            //Verify if symlink is valid
            if(!ghc::filesystem::exists(targetPath, ec))
            {
                ssLOG_DEBUG("Found invalid symlink, removing: " << targetPath.string());
                ghc::filesystem::remove(targetPath, ec);
                needsUpdate = true;
            }
        }

        //If file exists in source, check if it needs update
        if(ghc::filesystem::exists(srcPath, ec))
        {
            if(!needsUpdate)
            {
                ghc::filesystem::file_time_type srcTime = 
                    ghc::filesystem::last_write_time(srcPath, ec);
                ghc::filesystem::file_time_type dstTime = 
                    ghc::filesystem::last_write_time(targetPath, ec);
                needsUpdate = (srcTime > dstTime);
            }

            if(needsUpdate)
            {
                ssLOG_DEBUG("Updating: " << targetPath.string());
                ghc::filesystem::remove(targetPath, ec);
                
                switch(local->CopyMode)
                {
                    case Data::LocalCopyMode::Auto:
                        ghc::filesystem::create_symlink(srcPath, targetPath, ec);
                        if(ec)
                        {
                            ssLOG_DEBUG("Symlink failed: " << ec.message());
                            ec.clear();
                            ghc::filesystem::create_hard_link(srcPath, targetPath, ec);
                            if(ec)
                            {
                                ssLOG_DEBUG("Hardlink failed: " << ec.message());
                                ec.clear();
                                ghc::filesystem::copy(srcPath, targetPath, ec);
                            }
                        }
                        break;

                    case Data::LocalCopyMode::Symlink:
                        ghc::filesystem::create_symlink(srcPath, targetPath, ec);
                        break;

                    case Data::LocalCopyMode::Hardlink:
                        ghc::filesystem::create_hard_link(srcPath, targetPath, ec);
                        break;

                    case Data::LocalCopyMode::Copy:
                        ghc::filesystem::copy(srcPath, targetPath, ec);
                        break;
                }

                if(ec)
                {
                    ssLOG_ERROR("Failed to update target: " << ec.message());
                    return false;
                }
            }
            sourceFiles.erase(targetPath.filename().string());
        }
        else
        {
            //File no longer exists in source, remove it
            ssLOG_DEBUG("Removing file that no longer exists in source: " << targetPath.string());
            ghc::filesystem::remove(targetPath, ec);
        }
    }

    //Second pass: Add any new files from source
    for(const std::string& filename : sourceFiles)
    {
        const ghc::filesystem::path& srcPath = sourcePath / filename;
        const ghc::filesystem::path& targetPath = copyPath / filename;
        
        ssLOG_DEBUG("Adding new file: " << targetPath.string());
        
        switch(local->CopyMode)
        {
            case Data::LocalCopyMode::Auto:
                ghc::filesystem::create_symlink(srcPath, targetPath, ec);
                if(ec)
                {
                    ssLOG_DEBUG("Symlink failed: " << ec.message());
                    ec.clear();
                    ghc::filesystem::create_hard_link(srcPath, targetPath, ec);
                    if(ec)
                    {
                        ssLOG_DEBUG("Hardlink failed: " << ec.message());
                        ec.clear();
                        ghc::filesystem::copy(srcPath, targetPath, ec);
                    }
                }
                break;

            case Data::LocalCopyMode::Symlink:
                ghc::filesystem::create_symlink(srcPath, targetPath, ec);
                break;

            case Data::LocalCopyMode::Hardlink:
                ghc::filesystem::create_hard_link(srcPath, targetPath, ec);
                break;

            case Data::LocalCopyMode::Copy:
                ghc::filesystem::copy(srcPath, targetPath, ec);
                break;
        }

        if(ec)
        {
            ssLOG_ERROR("Failed to add new file: " << ec.message());
            return false;
        }
    }

    return true;
}

bool runcpp2::SyncLocalDependencies(const std::vector<Data::DependencyInfo*>& dependencies,
                                    const std::vector<std::string>& dependenciesSourcePaths,
                                    const std::vector<std::string>& dependenciesCopiesPaths)
{
    ssLOG_FUNC_DEBUG();
    for(size_t i = 0; i < dependencies.size(); ++i)
    {
        if(!SyncLocalDependency(*dependencies.at(i),
                                ghc::filesystem::path(dependenciesSourcePaths.at(i)),
                                ghc::filesystem::path(dependenciesCopiesPaths.at(i))))
        {
            return false;
        }
    }

    return true;
}

