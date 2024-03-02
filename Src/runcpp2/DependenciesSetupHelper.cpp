#include "runcpp2/DependenciesSetupHelper.hpp"
#include "System2.h"
#include "ghc/filesystem.hpp"
#include "runcpp2/PlatformUtil.hpp"
#include "ssLogger/ssLog.hpp"

bool runcpp2::GetDependenciesPaths( const std::vector<DependencyInfo>& dependencies,
                                    std::vector<std::string>& copiesPaths,
                                    std::vector<std::string>& sourcesPaths,
                                    std::string runcpp2ScriptDir,
                                    std::string scriptDir)
{
    std::vector<std::string> platformNames = Internal::GetPlatformNames();
    
    for(int i = 0; i < dependencies.size(); ++i)
    {
        for(int j = 0; j < platformNames.size(); ++j)
        {
            if( dependencies.at(i).Platforms.find(platformNames.at(j)) == 
                dependencies.at(i).Platforms.end())
            {
                copiesPaths.push_back("");
                sourcesPaths.push_back("");
                continue;
            }
        }
        
        const DependencySource& currentSource = dependencies.at(i).Source;
        
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
                        currentSource.Value.substr( lastSlashFoundIndex + 1, 
                                                    lastDotGitFoundIndex - lastSlashFoundIndex - 1);
                    
                    copiesPaths.push_back(runcpp2ScriptDir + "/" + gitRepoName);
                    sourcesPaths.push_back("");
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
                copiesPaths.push_back(runcpp2ScriptDir + "/" + localDepDirectoryName);
                
                if(ghc::filesystem::path(curPath).is_relative())
                    sourcesPaths.push_back(scriptDir + "/" + currentSource.Value);
                else
                    sourcesPaths.push_back(currentSource.Value);
                
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
        bool platformFound = false;
        for(int j = 0; j < platformNames.size(); ++j)
        {
            if( dependencies.at(i).Platforms.find(platformNames.at(j)) != 
                dependencies.at(i).Platforms.end())
            {
                platformFound = true;
                break;
            }
        }
        
        if(!platformFound)
            continue;
        
        if(ghc::filesystem::exists(dependenciesCopiesPaths.at(i), _))
        {
            if(!ghc::filesystem::is_directory(dependenciesCopiesPaths.at(i), _))
            {
                ssLOG_ERROR("Dependency path is a file: " << dependenciesCopiesPaths.at(i));
                return false;
            }
        }
        else
        {
            static_assert((int)DependencySourceType::COUNT == 2, "");
            
            switch(dependencies.at(i).Source.Type)
            {
                case DependencySourceType::GIT:
                {
                    std::string processedRuncpp2ScriptDir = Internal::ProcessPath(runcpp2ScriptDir);
                    std::string gitCloneCommand = "cd " + processedRuncpp2ScriptDir;
                    gitCloneCommand += " && git clone " + dependencies.at(i).Source.Value;
                    
                    System2CommandInfo gitCommandInfo;
                    SYSTEM2_RESULT result = System2Run(gitCloneCommand.c_str(), &gitCommandInfo);
                    
                    if(result != SYSTEM2_RESULT_SUCCESS)
                    {
                        ssLOG_ERROR("Failed to run git clone with result: " << result);
                        return false;
                    }
                    
                    std::string output;
                    
                    while(true)
                    {
                        char outputBuffer[1024] = {0};
                        uint32_t bytesRead = 0;
                        
                        result = System2ReadFromOutput(&gitCommandInfo, outputBuffer, 1023, &bytesRead);
                        
                        switch(result)
                        {
                            case SYSTEM2_RESULT_SUCCESS:
                                output.append(outputBuffer, bytesRead);
                                break;
                            case SYSTEM2_RESULT_READ_NOT_FINISHED:
                                output.append(outputBuffer, bytesRead);
                                break;
                            default:
                                ssLOG_ERROR("Failed to read git clone output with result: " << result);
                                return false;
                        }
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
                    std::string sourcePath = dependenciesSourcesPaths.at(i);
                    std::string destinationPath = dependenciesCopiesPaths.at(i);
                    
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

bool runcpp2::RunDependenciesSetupSteps(const ProfileName& profileName,
                                        const std::vector<DependencyInfo>& dependencies,
                                        const std::vector<std::string>& dependenciesCopiesPaths)
{
    std::vector<std::string> platformNames = Internal::GetPlatformNames();
    for(int i = 0; i < dependencies.size(); ++i)
    {
        int platformFoundIndex = -1;
        
        for(int j = 0; j < platformNames.size(); ++j)
        {
            if( dependencies.at(i).Platforms.find(platformNames.at(j)) != 
                dependencies.at(i).Platforms.end())
            {
                platformFoundIndex = j;
                break;
            }
        }
        
        if(platformFoundIndex == -1 || dependencies.at(i).Setup.empty())
            continue;
        
        
        PlatformName chosenPlatformName;
        
        for(int j = 0; j < platformNames.size(); ++j)
        {
            if( dependencies.at(i).Platforms.find(platformNames.at(j)) == 
                dependencies.at(i).Platforms.end())
            {
                continue;
            }
            
            const DependencySetup& dependencySetup = dependencies   .at(i)
                                                                    .Setup
                                                                    .find(platformNames.at(j))
                                                                    ->second;
            
            if(dependencySetup.SetupSteps.find(profileName) == dependencySetup.SetupSteps.end())
            {
                ssLOG_ERROR("Dependency " << dependencies.at(i).Name << " failed to find setup " 
                            "with profile " << profileName);
                
                return false;
            }
            
            chosenPlatformName = platformNames.at(j);
            break;
        }
        
        if(chosenPlatformName.empty())
        {
            ssLOG_ERROR("Dependency " << dependencies.at(i).Name << " failed to find setup " 
                        "with current platform");

            return false;
        }
        
        //Run the setup command
        const DependencySetup& dependencySetup = dependencies   .at(i)
                                                                .Setup
                                                                .find(chosenPlatformName)
                                                                ->second;
        
        const std::vector<std::string>& setupCommands = dependencySetup.SetupSteps.at(profileName);
        
        for(int k = 0; k < setupCommands.size(); ++k)
        {
            std::string processedDependencyPath = Internal::ProcessPath(dependenciesCopiesPaths.at(i));
            
            std::string setupCommand = "cd " + processedDependencyPath;
            setupCommand += " && " + setupCommands.at(k);
            
            System2CommandInfo setupCommandInfo;
            SYSTEM2_RESULT result = System2Run(setupCommand.c_str(), &setupCommandInfo);
            
            if(result != SYSTEM2_RESULT_SUCCESS)
            {
                ssLOG_ERROR("Failed to run setup command with result: " << result);
                return false;
            }
            
            std::string output;
            
            while(true)
            {
                char outputBuffer[1024] = {0};
                uint32_t bytesRead = 0;
                //TODO: Use do while loop
                result = System2ReadFromOutput(&setupCommandInfo, outputBuffer, 1023, &bytesRead);
                if(bytesRead >= 1024 || outputBuffer[1023] != '\0')
                {
                    ssLOG_ERROR("outputBuffer has overflowed");
                    return false;
                }
                
                switch(result)
                {
                    case SYSTEM2_RESULT_SUCCESS:
                        output.append(outputBuffer, bytesRead);
                        break;
                    case SYSTEM2_RESULT_READ_NOT_FINISHED:
                        output.append(outputBuffer, bytesRead);
                        break;
                    default:
                        ssLOG_ERROR("Failed to read git clone output with result: " << result);
                        return false;
                }
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
                                        const ScriptInfo& scriptInfo,
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
            if(!ghc::filesystem::remove_all(outDependenciesLocalCopiesPaths.at(i), _))
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
    
    //Run setup steps
    if(!RunDependenciesSetupSteps(  profileName,
                                    scriptInfo.Dependencies, 
                                    outDependenciesLocalCopiesPaths))
    {
        return false;
    }

    return true;
}