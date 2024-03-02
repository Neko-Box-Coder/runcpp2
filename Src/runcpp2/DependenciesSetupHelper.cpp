#include "runcpp2/DependenciesSetupHelper.hpp"
#include "System2.h"
#include "ghc/filesystem.hpp"
#include "runcpp2/PlatformUtil.hpp"
#include "ssLogger/ssLog.hpp"

bool runcpp2::IsDependencyAvailableForThisPlatform(const DependencyInfo& dependency)
{
    std::vector<std::string> platformNames = Internal::GetPlatformNames();
    
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


bool runcpp2::GetDependenciesPaths( const std::vector<DependencyInfo>& dependencies,
                                    std::vector<std::string>& outCopiesPaths,
                                    std::vector<std::string>& outSourcesPaths,
                                    std::string runcpp2ScriptDir,
                                    std::string scriptDir)
{
    for(int i = 0; i < dependencies.size(); ++i)
    {
        if(!IsDependencyAvailableForThisPlatform(dependencies[i]))
            continue;
        
        const DependencySource& currentSource = dependencies[i].Source;
        
        static_assert((int)DependencySourceType::COUNT == 2, "");
        
        switch(currentSource.Type)
        {
            case DependencySourceType::GIT:
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
            
            case DependencySourceType::LOCAL:
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
            
            case DependencySourceType::COUNT:
                return false;
        }
    }
    
    return true;
}

bool runcpp2::PopulateLocalDependencies(const std::vector<DependencyInfo>& dependencies,
                                        const std::vector<std::string>& dependenciesCopiesPaths,
                                        const std::vector<std::string>& dependenciesSourcesPaths,
                                        const std::string runcpp2ScriptDir)
{
    std::vector<std::string> platformNames = Internal::GetPlatformNames();
    
    std::error_code _;
    for(int i = 0; i < dependencies.size(); ++i)
    {
        if(!IsDependencyAvailableForThisPlatform(dependencies[i]))
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
            static_assert((int)DependencySourceType::COUNT == 2, "");
            
            switch(dependencies[i].Source.Type)
            {
                case DependencySourceType::GIT:
                {
                    std::string processedRuncpp2ScriptDir = Internal::ProcessPath(runcpp2ScriptDir);
                    std::string gitCloneCommand = "cd " + processedRuncpp2ScriptDir;
                    gitCloneCommand += " && git clone " + dependencies[i].Source.Value;
                    
                    System2CommandInfo gitCommandInfo;
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
                        
                        result = System2ReadFromOutput(&gitCommandInfo, outputBuffer, 1023, &bytesRead);
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
                
                case DependencySourceType::LOCAL:
                {
                    std::string sourcePath = dependenciesSourcesPaths[i];
                    std::string destinationPath = dependenciesCopiesPaths[i];
                    
                    //Copy the folder
                    ghc::filesystem::copy(destinationPath, sourcePath, _);
                    break;
                }
                
                case DependencySourceType::COUNT:
                    return false;
            }
        }
    }
    
    return true;
}

bool runcpp2::PopulateAbsoluteIncludePaths( std::vector<DependencyInfo>& dependencies,
                                            const std::vector<std::string>& dependenciesCopiesPaths)
{
    //Append absolute include paths from relative include paths
    for(int i = 0; i < dependencies.size(); ++i)
    {
        if(!IsDependencyAvailableForThisPlatform(dependencies[i]))
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
                                                                    dependencies[i].IncludePaths[j]));
        
            ssLOG_DEBUG("Include path added: " << dependencies[i].AbsoluteIncludePaths.back());
        }
    }
    
    return true;
}

bool runcpp2::RunDependenciesSetupSteps(const ProfileName& profileName,
                                        std::vector<DependencyInfo>& dependencies,
                                        const std::vector<std::string>& dependenciesCopiesPaths)
{
    std::vector<std::string> platformNames = Internal::GetPlatformNames();
    for(int i = 0; i < dependencies.size(); ++i)
    {
        if(!IsDependencyAvailableForThisPlatform(dependencies[i]) || dependencies[i].Setup.empty())
            continue;
        
        //Find the platform name we use for setup
        PlatformName chosenPlatformName;
        for(int j = 0; j < platformNames.size(); ++j)
        {
            if(dependencies[i].Setup.find(platformNames[j]) == dependencies[i].Setup.end())
                continue;
            
            const DependencySetup& dependencySetup = dependencies[i].Setup
                                                                    .find(platformNames[j])
                                                                    ->second;
            
            
            if(dependencySetup.SetupSteps.find(profileName) == dependencySetup.SetupSteps.end())
            {
                ssLOG_ERROR("Dependency " << dependencies[i].Name << " failed to find setup " 
                            "with profile " << profileName);
                
                return false;
            }
            
            chosenPlatformName = platformNames[j];
            break;
        }
        
        if(chosenPlatformName.empty())
        {
            ssLOG_ERROR("Dependency " << dependencies[i].Name << " failed to find setup " 
                        "with current platform");

            return false;
        }
        
        //Run the setup command
        const DependencySetup& dependencySetup = dependencies   [i]
                                                                .Setup
                                                                .find(chosenPlatformName)
                                                                ->second;
        
        const std::vector<std::string>& setupCommands = dependencySetup.SetupSteps.at(profileName);
        
        for(int k = 0; k < setupCommands.size(); ++k)
        {
            std::string processedDependencyPath = Internal::ProcessPath(dependenciesCopiesPaths[i]);
            
            std::string setupCommand = "cd " + processedDependencyPath;
            setupCommand += " && " + setupCommands[k];
            
            System2CommandInfo setupCommandInfo;
            SYSTEM2_RESULT result = System2Run(setupCommand.c_str(), &setupCommandInfo);
            
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
                
                result = System2ReadFromOutput(&setupCommandInfo, outputBuffer, 1023, &bytesRead);
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
        }
    }
    
    return true;
}

bool runcpp2::SetupScriptDependencies(  const ProfileName& profileName,
                                        const std::string& scriptPath, 
                                        ScriptInfo& scriptInfo,
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
    
    if(!PopulateAbsoluteIncludePaths(scriptInfo.Dependencies, outDependenciesLocalCopiesPaths))
        return false;
    
    //Run setup steps
    if(!RunDependenciesSetupSteps(  profileName,
                                    scriptInfo.Dependencies, 
                                    outDependenciesLocalCopiesPaths))
    {
        return false;
    }

    return true;
}