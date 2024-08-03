#include "runcpp2/DependenciesSetupHelper.hpp"
#include "ghc/filesystem.hpp"
#include "runcpp2/PlatformUtil.hpp"
#include "ssLogger/ssLog.hpp"

namespace
{
    bool GetDependenciesPaths(  const std::vector<runcpp2::Data::DependencyInfo>& dependencies,
                                std::vector<std::string>& outCopiesPaths,
                                std::vector<std::string>& outSourcesPaths,
                                std::string runcpp2ScriptDir,
                                std::string scriptDir)
    {
        for(int i = 0; i < dependencies.size(); ++i)
        {
            if(!runcpp2::IsDependencyAvailableForThisPlatform(dependencies.at(i)))
                continue;
            
            const runcpp2::Data::DependencySource& currentSource = dependencies.at(i).Source;
            
            static_assert((int)runcpp2::Data::DependencySourceType::COUNT == 2, "");
            
            switch(currentSource.Type)
            {
                case runcpp2::Data::DependencySourceType::GIT:
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
                        
                        outCopiesPaths.push_back(runcpp2ScriptDir + "/" + gitRepoName);
                        outSourcesPaths.push_back("");
                    }
                    break;
                }
                
                case runcpp2::Data::DependencySourceType::LOCAL:
                {
                    std::string localDepDirectoryName;
                    std::string curPath = currentSource.Value;
                    
                    if(curPath.back() == '/')
                        curPath.pop_back();
                    
                    localDepDirectoryName = ghc::filesystem::path(curPath).filename().string();
                    outCopiesPaths.push_back(runcpp2ScriptDir + "/" + localDepDirectoryName);
                    
                    if(ghc::filesystem::path(curPath).is_relative())
                        outSourcesPaths.push_back(scriptDir + "/" + currentSource.Value);
                    else
                        outSourcesPaths.push_back(currentSource.Value);
                    
                    break;
                }
                
                case runcpp2::Data::DependencySourceType::COUNT:
                    return false;
            }
        }
        
        return true;
    }
    
    bool PopulateLocalDependencies( const std::vector<runcpp2::Data::DependencyInfo>& dependencies,
                                    const std::vector<std::string>& dependenciesCopiesPaths,
                                    const std::vector<std::string>& dependenciesSourcesPaths,
                                    const std::string runcpp2ScriptDir)
    {
        std::error_code _;
        for(int i = 0; i < dependencies.size(); ++i)
        {
            if(!runcpp2::IsDependencyAvailableForThisPlatform(dependencies.at(i)))
                continue;
            
            if(ghc::filesystem::exists(dependenciesCopiesPaths.at(i), _))
            {
                if(!ghc::filesystem::is_directory(dependenciesCopiesPaths.at(i), _))
                {
                    ssLOG_ERROR("Dependency path is a file: " << dependenciesCopiesPaths.at(i));
                    ssLOG_ERROR("It should be a folder instead");
                    return false;
                }
            }
            else
            {
                static_assert((int)runcpp2::Data::DependencySourceType::COUNT == 2, "");
                
                switch(dependencies.at(i).Source.Type)
                {
                    case runcpp2::Data::DependencySourceType::GIT:
                    {
                        std::string processedRuncpp2ScriptDir = runcpp2::ProcessPath(runcpp2ScriptDir);
                        std::string gitCloneCommand = "git clone " + dependencies.at(i).Source.Value;
                        
                        ssLOG_INFO( "Running git clone command: " << gitCloneCommand << " in " << 
                                    processedRuncpp2ScriptDir);
                        
                        int returnCode = 0;
                        std::string output;
                        if(!runcpp2::RunCommandAndGetOutput(gitCloneCommand, 
                                                            output, 
                                                            returnCode,
                                                            processedRuncpp2ScriptDir))
                        {
                            ssLOG_ERROR("Failed to run git clone with result: " << returnCode);
                            ssLOG_ERROR("Output: " << output);
                            return false;
                        }
                        
                        ssLOG_INFO("Output: " << output);
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
            }
        }
        
        return true;
    }
    
    bool PopulateAbsoluteIncludePaths(  std::vector<runcpp2::Data::DependencyInfo>& dependencies,
                                        const std::vector<std::string>& dependenciesCopiesPaths)
    {
        //Append absolute include paths from relative include paths
        for(int i = 0; i < dependencies.size(); ++i)
        {
            if(!runcpp2::IsDependencyAvailableForThisPlatform(dependencies.at(i)))
                continue;
            
            dependencies.at(i).AbsoluteIncludePaths.clear();
            for(int j = 0; j < dependencies.at(i).IncludePaths.size(); ++j)
            {
                if(ghc::filesystem::path(dependencies.at(i).IncludePaths.at(j)).is_absolute())
                {
                    ssLOG_ERROR("Dependency include path cannot be absolute: " << 
                                dependencies.at(i).IncludePaths.at(j));
                    
                    return false;
                }
                
                dependencies.at(i) 
                            .AbsoluteIncludePaths
                            .push_back(ghc::filesystem::absolute(   dependenciesCopiesPaths.at(i) + "/" + 
                                                                    dependencies.at(i).IncludePaths.at(j)).string());
            
                ssLOG_DEBUG("Include path added: " << dependencies.at(i).AbsoluteIncludePaths.back());
            }
        }
        
        return true;
    }

    bool RunDependenciesSetupSteps( const runcpp2::Data::Profile& profile,
                                    std::vector<runcpp2::Data::DependencyInfo>& dependencies,
                                    const std::vector<std::string>& dependenciesCopiesPaths)
    {
        for(int i = 0; i < dependencies.size(); ++i)
        {
            if( !runcpp2::IsDependencyAvailableForThisPlatform(dependencies.at(i)) || 
                dependencies.at(i).Setup.empty())
            {
                continue;
            }
            
            //Find the platform name we use for setup
            PlatformName chosenPlatformName;
            if(!runcpp2::HasValueFromPlatformMap(dependencies.at(i).Setup))
            {
                ssLOG_WARNING(  "Dependency " << dependencies.at(i).Name << " failed to find setup " 
                                "with current platform");
                continue;
            }
            
            const runcpp2::Data::DependencyCommands& dependencySetup = 
                *runcpp2::GetValueFromPlatformMap(dependencies.at(i).Setup);
            
            std::string profileNameToUse;
            //Check profile name is available first
            if(dependencySetup.CommandSteps.count(profile.Name) > 0)
                profileNameToUse = profile.Name;
            else
            {
                //If not check for name alias
                for(const auto& alias : profile.NameAliases)
                {
                    if(dependencySetup.CommandSteps.count(alias) > 0)
                    {
                        profileNameToUse = alias;
                        break;
                    }
                }
                
                //Otherwise check for "All"
                if(dependencySetup.CommandSteps.count("All") > 0)
                {
                    profileNameToUse = "All";
                    break;
                }
                
                if(profileNameToUse.empty())
                {
                    ssLOG_ERROR("Dependency " << dependencies.at(i).Name << " failed to find setup " 
                                "with profile " << profile.Name);
                        
                    return false;
                }
            }
            
            //Run the setup command
            const std::vector<std::string>& setupCommands = 
                dependencySetup.CommandSteps.at(profileNameToUse);
            
            for(int k = 0; k < setupCommands.size(); ++k)
            {
                std::string processedDependencyPath = 
                    runcpp2::ProcessPath(dependenciesCopiesPaths.at(i));
                
                ssLOG_INFO( "Running setup command: " << setupCommands.at(k) << " in " << 
                            processedDependencyPath);
                
                int returnCode = 0;
                std::string output;
                if(!runcpp2::RunCommandAndGetOutput(setupCommands.at(k), 
                                                    output, 
                                                    returnCode, 
                                                    processedDependencyPath))
                {
                    ssLOG_ERROR("Failed to run setup command with result: " << returnCode);
                    ssLOG_ERROR("Output: " << output);
                    return false;
                }
                
                ssLOG_INFO("Output: " << output);
            }
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

bool runcpp2::SetupScriptDependencies(  const runcpp2::Data::Profile& profile,
                                        const std::string& scriptPath, 
                                        Data::ScriptInfo& scriptInfo,
                                        bool resetDependencies,
                                        std::vector<std::string>& outDependenciesLocalCopiesPaths,
                                        std::vector<std::string>& outDependenciesSourcePaths)
{
    //If the script info is not populated (i.e. empty script info), don't do anything
    if(!scriptInfo.Populated)
        return true;

    std::string scriptDirectory = ghc::filesystem::path(scriptPath).parent_path().string();
    std::string scriptName = ghc::filesystem::path(scriptPath).stem().string();
    std::string runcpp2ScriptDir = scriptDirectory + "/.runcpp2";
    
    //Get the dependencies paths
    if(!GetDependenciesPaths(   scriptInfo.Dependencies, 
                                outDependenciesLocalCopiesPaths, 
                                outDependenciesSourcePaths, 
                                runcpp2ScriptDir, 
                                scriptDirectory))
    {
        return false;
    }
    
    //Reset dependencies if needed
    if(resetDependencies)
    {
        std::error_code _;
        
        for(int i = 0; i < outDependenciesLocalCopiesPaths.size(); ++i)
        {
            //Remove the directory
            if( ghc::filesystem::exists(outDependenciesLocalCopiesPaths.at(i), _) &&
                !ghc::filesystem::remove_all(outDependenciesLocalCopiesPaths.at(i), _))
            {
                ssLOG_ERROR("Failed to reset dependency directory: " << 
                            outDependenciesLocalCopiesPaths.at(i));
                
                return false;
            }
        }
    }
    
    //Clone/copy the dependencies if needed
    if(!PopulateLocalDependencies(  scriptInfo.Dependencies, 
                                    outDependenciesLocalCopiesPaths, 
                                    outDependenciesSourcePaths, 
                                    runcpp2ScriptDir))
    {
        return false;
    }
    
    if(!PopulateAbsoluteIncludePaths(scriptInfo.Dependencies, outDependenciesLocalCopiesPaths))
    {
        return false;
    }
    
    //Run setup steps
    if(!RunDependenciesSetupSteps(profile, scriptInfo.Dependencies, outDependenciesLocalCopiesPaths))
    {
        return false;
    }

    return true;
}

bool runcpp2::CopyDependenciesBinaries( const std::string& scriptPath, 
                                        const Data::ScriptInfo& scriptInfo,
                                        const std::vector<std::string>& dependenciesCopiesPaths,
                                        const Data::Profile& profile,
                                        std::vector<std::string>& outCopiedBinariesPaths)
{
    std::string scriptDirectory = ghc::filesystem::path(scriptPath).parent_path().string();
    std::string scriptName = ghc::filesystem::path(scriptPath).stem().string();
    std::string runcpp2ScriptDir = scriptDirectory + "/.runcpp2";
    std::vector<std::string> platformNames = GetPlatformNames();
    
    int minimumDependenciesCopiesCount = 0;
    for(int i = 0; i < scriptInfo.Dependencies.size(); ++i)
    {
        if( runcpp2::IsDependencyAvailableForThisPlatform(scriptInfo.Dependencies.at(i)) &&
            scriptInfo.Dependencies.at(i).LibraryType != runcpp2::Data::DependencyLibraryType::HEADER)
        {
            ++minimumDependenciesCopiesCount;
        }
    }

    if(minimumDependenciesCopiesCount > dependenciesCopiesPaths.size())
    {
        ssLOG_ERROR("The amount of available dependencies do not match" <<
                    " the amount of dependencies copies paths");
        
        return false;
    }
    
    for(int i = 0; i < scriptInfo.Dependencies.size(); ++i)
    {
        std::vector<std::string> extensionsToCopy;
        
        //Get all the file extensions to copy
        {
            if(!GetDependencyBinariesExtensionsToCopy(  scriptInfo.Dependencies.at(i),
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
        
        if(scriptInfo.Dependencies.at(i).LibraryType == Data::DependencyLibraryType::HEADER)
            return true;
        
        //Get the Search path and search library name
        const std::unordered_map<   ProfileName, 
                                    Data::DependencyLinkProperty>& linkProperties = 
            scriptInfo.Dependencies.at(i).LinkProperties;
        
        //See if we can find the link properties with the profile name
        auto foundPropertyIt = linkProperties.find(profile.Name);
        if(foundPropertyIt == linkProperties.end())
        {
            //If not, try alias
            for(const auto& alias : profile.NameAliases)
            {
                foundPropertyIt = linkProperties.find(alias);
                if(foundPropertyIt != linkProperties.end())
                    break;
            }
            
            //If not, try All
            if(foundPropertyIt == linkProperties.end())
                foundPropertyIt = linkProperties.find("All");
        }
        if(foundPropertyIt == linkProperties.end())
        {
            ssLOG_ERROR("Search properties for dependency " << scriptInfo.Dependencies.at(i).Name <<
                        " is missing profile " << profile.Name);
            
            return false;
        }
        const Data::DependencyLinkProperty& searchProperty = foundPropertyIt->second;
        
        //Copy the files with extensions that contains the search name
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
            
                std::error_code _;
                if( !ghc::filesystem::exists(currentSearchDirectory, _) || 
                    !ghc::filesystem::is_directory(currentSearchDirectory, _))
                {
                    ssLOG_INFO("Invalid search path: " << currentSearchDirectory);
                    continue;
                }
            
                for(auto it : ghc::filesystem::directory_iterator(currentSearchDirectory, _))
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
                    
                    std::error_code copyErrorCode;
                    ghc::filesystem::copy(  it.path(), 
                                            runcpp2ScriptDir, 
                                            ghc::filesystem::copy_options::overwrite_existing,  
                                            copyErrorCode);
                    
                    if(copyErrorCode)
                    {
                        ssLOG_ERROR("Failed to copy file from " << it.path().string() << 
                                    " to " << runcpp2ScriptDir);

                        ssLOG_ERROR("Error: " << copyErrorCode.message());
                        return false;
                    }
                    
                    ssLOG_INFO("Copied " << it.path().string());
                    outCopiedBinariesPaths.push_back(it.path().string());
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
