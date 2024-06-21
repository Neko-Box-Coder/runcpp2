#include "runcpp2/DependenciesSetupHelper.hpp"
#include "System2.h"
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
            if(!runcpp2::IsDependencyAvailableForThisPlatform(dependencies[i]))
                continue;
            
            const runcpp2::Data::DependencySource& currentSource = dependencies[i].Source;
            
            static_assert((int)runcpp2::Data::DependencySourceType::COUNT == 2, "");
            
            switch(currentSource.Type)
            {
                case runcpp2::Data::DependencySourceType::GIT:
                {
                    size_t lastSlashFoundIndex = currentSource.Value.find_last_of("/");
                    size_t lastDotGitFoundIndex = currentSource.Value.find_last_of(".git");
                    
                    if(lastSlashFoundIndex == std::string::npos || 
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
            if(!runcpp2::IsDependencyAvailableForThisPlatform(dependencies[i]))
                continue;
            
            if(ghc::filesystem::exists(dependenciesCopiesPaths[i], _))
            {
                if(!ghc::filesystem::is_directory(dependenciesCopiesPaths[i], _))
                {
                    ssLOG_ERROR("Dependency path is a file: " << dependenciesCopiesPaths[i]);
                    return false;
                }
            }
            else
            {
                static_assert((int)runcpp2::Data::DependencySourceType::COUNT == 2, "");
                
                switch(dependencies[i].Source.Type)
                {
                    case runcpp2::Data::DependencySourceType::GIT:
                    {
                        std::string processedRuncpp2ScriptDir = runcpp2::ProcessPath(runcpp2ScriptDir);
                        std::string gitCloneCommand = "cd " + processedRuncpp2ScriptDir;
                        gitCloneCommand += " && git clone " + dependencies[i].Source.Value;
                        
                        System2CommandInfo gitCommandInfo = {};
                        gitCommandInfo.RedirectOutput = true;
                        SYSTEM2_RESULT result = System2Run(gitCloneCommand.c_str(), &gitCommandInfo);
                        
                        if(result != SYSTEM2_RESULT_SUCCESS)
                        {
                            ssLOG_ERROR("Failed to run git clone with result: " << result);
                            return false;
                        }
                        
                        std::string output;
                        
                        do
                        {
                            char outputBuffer[1024] = {0};
                            uint32_t bytesRead = 0;
                            
                            result = System2ReadFromOutput( &gitCommandInfo, 
                                                            outputBuffer, 
                                                            1023, 
                                                            &bytesRead);
                            
                            output.append(outputBuffer, bytesRead);
                        }
                        while(result == SYSTEM2_RESULT_READ_NOT_FINISHED);
                        
                        if(result != SYSTEM2_RESULT_SUCCESS)
                        {
                            ssLOG_ERROR("Failed to read git clone output with result: " << result);
                            return false;
                        }
                        
                        int returnCode = 0;
                        result = System2GetCommandReturnValueSync(&gitCommandInfo, &returnCode);
                        
                        if(result != SYSTEM2_RESULT_SUCCESS)
                        {
                            ssLOG_ERROR("Failed to get git clone return value with result: " << result);
                            ssLOG_ERROR("Output: " << output);
                            return false;
                        }
                        break;
                    }
                    
                    case runcpp2::Data::DependencySourceType::LOCAL:
                    {
                        std::string sourcePath = dependenciesSourcesPaths[i];
                        std::string destinationPath = dependenciesCopiesPaths[i];
                        
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
            if(!runcpp2::IsDependencyAvailableForThisPlatform(dependencies[i]))
                continue;
            
            dependencies[i].AbsoluteIncludePaths.clear();
            for(int j = 0; j < dependencies[i].IncludePaths.size(); ++j)
            {
                if(ghc::filesystem::path(dependencies[i].IncludePaths[j]).is_absolute())
                {
                    ssLOG_ERROR("Dependency include path cannot be absolute: " << 
                                dependencies[i].IncludePaths[j]);
                    
                    return false;
                }
                
                dependencies[i] .AbsoluteIncludePaths
                                .push_back(ghc::filesystem::absolute(   dependenciesCopiesPaths[i] + "/" + 
                                                                        dependencies[i].IncludePaths[j]).string());
            
                ssLOG_DEBUG("Include path added: " << dependencies[i].AbsoluteIncludePaths.back());
            }
        }
        
        return true;
    }

    bool RunDependenciesSetupSteps( const ProfileName& profileName,
                                    std::vector<runcpp2::Data::DependencyInfo>& dependencies,
                                    const std::vector<std::string>& dependenciesCopiesPaths)
    {
        for(int i = 0; i < dependencies.size(); ++i)
        {
            if( !runcpp2::IsDependencyAvailableForThisPlatform(dependencies[i]) || 
                dependencies[i].Setup.empty())
            {
                continue;
            }
            
            //Find the platform name we use for setup
            PlatformName chosenPlatformName;
            
            if(!runcpp2::HasValueFromPlatformMap(dependencies[i].Setup))
            {
                ssLOG_ERROR("Dependency " << dependencies[i].Name << " failed to find setup " 
                            "with current platform");
            }
            
            const runcpp2::Data::DependencySetup& dependencySetup = 
                *runcpp2::GetValueFromPlatformMap(dependencies[i].Setup);
            
            if( dependencySetup.SetupSteps.find(profileName) == dependencySetup.SetupSteps.end())
            {
                ssLOG_ERROR("Dependency " << dependencies[i].Name << " failed to find setup " 
                            "with profile " << profileName);
                    
                return false;
            }
            
            //Run the setup command
            const std::vector<std::string>& setupCommands = 
                dependencySetup.SetupSteps.at(profileName);
            
            for(int k = 0; k < setupCommands.size(); ++k)
            {
                std::string processedDependencyPath = 
                    runcpp2::ProcessPath(dependenciesCopiesPaths[i]);
                
                std::string setupCommand = "cd " + processedDependencyPath;
                setupCommand += " && " + setupCommands[k];
                
                System2CommandInfo setupCommandInfo = {};
                setupCommandInfo.RedirectOutput = true;
                SYSTEM2_RESULT result = System2Run(setupCommand.c_str(), &setupCommandInfo);
                
                ssLOG_INFO("Running setup command: " << setupCommand);
                
                if(result != SYSTEM2_RESULT_SUCCESS)
                {
                    ssLOG_ERROR("Failed to run setup command with result: " << result);
                    return false;
                }
                
                std::string output;
                
                do
                {
                    char outputBuffer[1024] = {0};
                    uint32_t bytesRead = 0;
                    
                    result = System2ReadFromOutput( &setupCommandInfo, 
                                                    outputBuffer, 
                                                    1023, 
                                                    &bytesRead);
                    
                    output.append(outputBuffer, bytesRead);
                }
                while(result == SYSTEM2_RESULT_READ_NOT_FINISHED);
                
                if(result != SYSTEM2_RESULT_SUCCESS)
                {
                    ssLOG_ERROR("Failed to read git clone output with result: " << result);
                    return false;
                }
                
                int returnCode = 0;
                result = System2GetCommandReturnValueSync(&setupCommandInfo, &returnCode);
                
                if(result != SYSTEM2_RESULT_SUCCESS)
                {
                    ssLOG_ERROR("Failed to get setup command return value with result: " << result);
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
                if(!runcpp2::HasValueFromPlatformMap(profile.StaticLinkFile.Extension))
                {
                    ssLOG_ERROR("Failed to find static library extensions for dependency " << 
                                dependencyInfo.Name);
                    
                    return false;
                }
                
                outExtensionsToCopy.push_back(
                    *runcpp2::GetValueFromPlatformMap(profile.StaticLinkFile.Extension));
                
                break;
            }
            case runcpp2::Data::DependencyLibraryType::SHARED:
            {
                if( !runcpp2::HasValueFromPlatformMap(profile.SharedLinkFile.Extension) ||
                    !runcpp2::HasValueFromPlatformMap(profile.SharedLibraryFile.Extension))
                {
                    ssLOG_ERROR("Failed to find shared library extensions for dependency " << 
                                dependencyInfo.Name);
                    
                    return false;
                }
                
                outExtensionsToCopy.push_back(
                    *runcpp2::GetValueFromPlatformMap(profile.SharedLinkFile.Extension));
                
                if( *runcpp2::GetValueFromPlatformMap(profile.SharedLinkFile.Extension) != 
                    *runcpp2::GetValueFromPlatformMap(profile.SharedLibraryFile.Extension) ||
                    *runcpp2::GetValueFromPlatformMap(profile.SharedLinkFile.Prefix) !=
                    *runcpp2::GetValueFromPlatformMap(profile.SharedLibraryFile.Prefix))
                {
                    outExtensionsToCopy.push_back(
                        *runcpp2::GetValueFromPlatformMap(profile.SharedLibraryFile.Extension));
                }
                
                break;
            }
            case runcpp2::Data::DependencyLibraryType::OBJECT:
            {
                if(!runcpp2::HasValueFromPlatformMap(profile.ObjectLinkFile.Extension))
                {
                    ssLOG_ERROR("Failed to find shared library extensions for dependency " << 
                                dependencyInfo.Name);
                    
                    return false;
                }
                
                outExtensionsToCopy.push_back(
                    *runcpp2::GetValueFromPlatformMap(profile.ObjectLinkFile.Extension));
                
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
        if( dependency.Platforms.find(platformNames[i]) != 
            dependency.Platforms.end())
        {
            return true;
        }
    }
    
    return false;
}

bool runcpp2::SetupScriptDependencies(  const ProfileName& profileName,
                                        const std::string& scriptPath, 
                                        Data::ScriptInfo& scriptInfo,
                                        bool resetDependencies,
                                        std::vector<std::string>& outDependenciesLocalCopiesPaths,
                                        std::vector<std::string>& outDependenciesSourcePaths)
{
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
        
        for(int i = 0; i < scriptInfo.Dependencies.size(); ++i)
        {
            //Remove the directory
            if(!ghc::filesystem::remove_all(outDependenciesLocalCopiesPaths[i], _))
            {
                ssLOG_ERROR("Failed to reset dependency directory: " << 
                            outDependenciesLocalCopiesPaths[i]);
                
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
    
    if(!PopulateAbsoluteIncludePaths(   scriptInfo.Dependencies, 
                                        outDependenciesLocalCopiesPaths))
    {
        return false;
    }
    
    //Run setup steps
    if(!RunDependenciesSetupSteps(  profileName,
                                    scriptInfo.Dependencies, 
                                    outDependenciesLocalCopiesPaths))
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
    
    if(scriptInfo.Dependencies.size() != dependenciesCopiesPaths.size())
    {
        ssLOG_ERROR("The amount of dependencies do not match the amount of dependencies copies paths");
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
                if( profile.DebugSymbolFile.Extension.find(platformNames.at(j)) == 
                    profile.DebugSymbolFile.Extension.end())
                {
                    continue;
                }
                
                extensionsToCopy.push_back(profile.DebugSymbolFile.Extension.at(platformNames.at(j)));
                break;
            }
        }
        
        if(scriptInfo.Dependencies.at(i).LibraryType == Data::DependencyLibraryType::HEADER)
            return true;
        
        const Data::DependencyLinkProperty& searchProperty = scriptInfo .Dependencies
                                                                        .at(i)
                                                                        .LinkProperties
                                                                        .at(profile.Name);
        
        //Get the Search path and search library name
        if( scriptInfo.Dependencies.at(i).LinkProperties.find(profile.Name) == 
            scriptInfo.Dependencies.at(i).LinkProperties.end())
        {
            ssLOG_ERROR("Search properties for dependency " << scriptInfo.Dependencies.at(i).Name <<
                        " is missing profile " << profile.Name);
            
            return false;
        }
        
        //Copy the files with extensions that contains the search name
        for(int j = 0; j < searchProperty.SearchLibraryNames.size(); ++j)
        {
            for(int k = 0; k < searchProperty.SearchDirectories.size(); ++k)
            {
                std::string currentSearchLibraryName = searchProperty.SearchLibraryNames.at(j);
                std::string currentSearchDirectory = searchProperty.SearchDirectories.at(k);
            
                ssLOG_DEBUG("currentSearchDirectory: " << currentSearchDirectory);
                ssLOG_DEBUG("currentSearchLibraryName: " << currentSearchLibraryName);
            
                if(!ghc::filesystem::path(currentSearchDirectory).is_absolute())
                    currentSearchDirectory = dependenciesCopiesPaths[i] + "/" + currentSearchDirectory;
            
                std::error_code _;
                if( !ghc::filesystem::exists(currentSearchDirectory, _) || 
                    !ghc::filesystem::is_directory(currentSearchDirectory, _))
                {
                    ssLOG_ERROR("Invalid search path: " << currentSearchDirectory);
                    return false;
                }
            
                for(auto it : ghc::filesystem::directory_iterator(currentSearchDirectory, _))
                {
                    if(it.is_directory())
                        continue;
                    
                    std::string currentFileName = it.path().stem().string();
                    std::string currentExtension = it.path().extension().string();
                    
                    ssLOG_DEBUG("currentFileName: " << currentFileName);
                    
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
                    
                    outCopiedBinariesPaths.push_back(currentFileName + currentExtension);
                }
            }
        }
    }
    
    return true;
}