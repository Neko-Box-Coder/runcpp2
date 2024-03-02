#include "runcpp2/runcpp2.hpp"

#include "runcpp2/ParseUtil.hpp"
#include "runcpp2/PlatformUtil.hpp"
#include "System2.h"
#include "runcpp2/StringUtil.hpp"
#include "ssLogger/ssLog.hpp"
#include "cfgpath.h"
#include "yaml-cpp/yaml.h"
#include "ghc/filesystem.hpp"

#include <fstream>

extern const uint8_t DefaultCompilerProfiles[];
extern const size_t DefaultCompilerProfiles_size;
extern const uint8_t DefaultScriptDependencies[];
extern const size_t DefaultScriptDependencies_size;

bool runcpp2::ParseCompilerProfiles(const std::string& compilerProfilesString, 
                                    std::vector<CompilerProfile>& outProfiles,
                                    std::string& outPreferredProfile)
{
    YAML::Node compilersProfileYAML;
    
    try
    {
        compilersProfileYAML = YAML::Load(compilerProfilesString);
    }
    catch(...)
    {
        return false;
    }

    YAML::Node compilerProfilesNode = compilersProfileYAML["CompilerProfiles"];
    if(!compilerProfilesNode || compilerProfilesNode.Type() != YAML::NodeType::Sequence)
    {
        ssLOG_ERROR("CompilerProfiles is invalid");
        return false;
    }
    
    if(compilerProfilesNode.size() == 0)
    {
        ssLOG_ERROR("No compiler profiles found");
        return false;
    }
    
    for(int i = 0; i < compilerProfilesNode.size(); ++i)
    {
        YAML::Node currentCompilerProfileNode = compilerProfilesNode[i];
        
        outProfiles.push_back({});
        if(!outProfiles.back().ParseYAML_Node(currentCompilerProfileNode))
        {
            outProfiles.erase(outProfiles.end() - 1);
            ssLOG_ERROR("Failed to parse compiler profile at index " << i);
            return false;
        }
    }
    
    if( compilersProfileYAML["PreferredProfile"] && 
        compilersProfileYAML["PreferredProfile"].Type() == YAML::NodeType::Scalar)
    {
        outPreferredProfile = compilersProfileYAML["PreferredProfile"].as<std::string>();
        if(outPreferredProfile.empty())
        {
            outPreferredProfile = outProfiles.at(0).Name;
            ssLOG_WARNING("PreferredProfile is empty. Using the first profile name");
        }
    }
    
    return true;
}

//TODO(NOW): Replace all command path slashes with platform slashes

bool runcpp2::ReadUserConfig(   std::vector<CompilerProfile>& outProfiles, 
                                std::string& outPreferredProfile)
{
    //Check if user config exists
    char configDirC_Str[MAX_PATH] = {0};
    
    get_user_config_folder(configDirC_Str, 512, "runcpp2");
    
    if(strlen(configDirC_Str) == 0)
    {
        ssLOG_ERROR("Failed to retrieve user config path");
        return false;
    }
    
    std::string configDir = std::string(configDirC_Str);
    
    std::string compilerConfigFilePaths[2] = 
    {
        configDir + "/CompilerProfiles.yaml", 
        configDir + "/CompilerProfiles.yml"
    };
    int foundConfigFilePathIndex = -1;
    
    bool writeDefaultProfiles = false;
    
    //config directory is created by get_user_config_folder if it doesn't exist
    {
        for(int i = 0; i < sizeof(compilerConfigFilePaths) / sizeof(std::string); ++i)
        {
            //Check if the config file exists
            if(ghc::filesystem::exists(compilerConfigFilePaths[i]))
            {
                foundConfigFilePathIndex = i;
                writeDefaultProfiles = false;
                break;
            }
        }
        
        writeDefaultProfiles = true;
    }
    
    //Create default compiler profiles
    if(writeDefaultProfiles)
    {
        //Create default compiler profiles
        std::ofstream configFile(compilerConfigFilePaths[0], std::ios::binary);
        if(!configFile)
        {
            ssLOG_ERROR("Failed to create default config file: " << compilerConfigFilePaths[0]);
            return false;
        }
        configFile.write((const char*)DefaultCompilerProfiles, DefaultCompilerProfiles_size);
        configFile.close();
        foundConfigFilePathIndex = 0;
    }
    
    //Read compiler profiles
    std::string compilerConfigContent;
    {
        std::ifstream compilerConfigFilePath(compilerConfigFilePaths[foundConfigFilePathIndex]);
        if(!compilerConfigFilePath)
        {
            ssLOG_ERROR("Failed to open config file: " << 
                        compilerConfigFilePaths[foundConfigFilePathIndex]);
            
            return false;
        }
        std::stringstream buffer;
        buffer << compilerConfigFilePath.rdbuf();
        compilerConfigContent = buffer.str();
    }
    
    if(!ParseCompilerProfiles(compilerConfigContent, outProfiles, outPreferredProfile))
    {
        ssLOG_ERROR("Failed to parse config file: " << 
                    compilerConfigFilePaths[foundConfigFilePathIndex]);
        
        return false;
    }

    return true;
}

bool runcpp2::ParseScriptInfo(  const std::string& scriptInfo, 
                                ScriptInfo& outScriptInfo)
{
    if(scriptInfo.empty())
        return true;

    YAML::Node scriptYAML;
    
    try
    {
        scriptYAML = YAML::Load(scriptInfo);
    }
    catch(...)
    {
        return false;
    }
    
    if(outScriptInfo.ParseYAML_Node(scriptYAML))
    {
        outScriptInfo.Populated = true;
        return true;
    }
    else
        return false;
}

bool runcpp2::CreateRuncpp2ScriptDirectory(const std::string& scriptPath)
{
    std::string scriptDirectory = ghc::filesystem::path(scriptPath).parent_path().string();
    
    //Create the runcpp2 directory
    std::string runcpp2Dir = scriptDirectory + "/.runcpp2";
    
    if(!ghc::filesystem::exists(runcpp2Dir))
    {
        std::error_code _;
        if(!ghc::filesystem::create_directory(runcpp2Dir, _))
        {
            ssLOG_ERROR("Failed to create runcpp2 directory");
            return false;
        }
    }
    
    return true;
}

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

bool runcpp2::CopyDependenciesBinaries( const std::string& scriptPath, 
                                        const ScriptInfo& scriptInfo,
                                        const std::vector<std::string>& dependenciesCopiesPaths,
                                        const CompilerProfile& profile)
{
    std::string scriptDirectory = ghc::filesystem::path(scriptPath).parent_path().string();
    std::string scriptName = ghc::filesystem::path(scriptPath).stem().string();
    std::string runcpp2ScriptDir = scriptDirectory + "/.runcpp2";
    std::vector<std::string> platformNames = Internal::GetPlatformNames();
    
    if(scriptInfo.Dependencies.size() != dependenciesCopiesPaths.size())
    {
        ssLOG_ERROR("The amount of dependencies do not match the amount of dependencies copies paths");
        return false;
    }
    
    for(int i = 0; i < scriptInfo.Dependencies.size(); ++i)
    {
        std::string foundPlatformName;
        
        for(int j = 0; j < platformNames.size(); ++j)
        {
            if( scriptInfo.Dependencies.at(i).Platforms.find(platformNames.at(j)) == 
                scriptInfo.Dependencies.at(i).Platforms.end())
            {
                continue;
            }
            
            foundPlatformName = platformNames.at(j);
        }
        
        if(foundPlatformName.empty())
        {
            ssLOG_ERROR("Failed to find setup for current platform for dependency: " << 
                        scriptInfo.Dependencies.at(i).Name);

            return false;
        }
        
        std::vector<std::string> extensionsToCopy;
        static_assert((int)DependencyLibraryType::COUNT == 4, "");
        switch(scriptInfo.Dependencies.at(i).LibraryType)
        {
            case DependencyLibraryType::STATIC:
            {
                for(int j = 0; j < platformNames.size(); ++j)
                {
                    if( profile.StaticLibraryExtensions.find(platformNames.at(j)) == 
                        profile.StaticLibraryExtensions.end())
                    {
                        if(j == platformNames.size() - 1)
                        {
                            ssLOG_ERROR("Failed to find static library extensions for dependency " << 
                                        scriptInfo.Dependencies.at(i).Name);
                            
                            return false;
                        }
                        
                        continue;
                    }
                    
                    extensionsToCopy = profile.StaticLibraryExtensions.at(platformNames.at(j));
                    break;
                }
                
                break;
            }
            case DependencyLibraryType::SHARED:
            {
                for(int j = 0; j < platformNames.size(); ++j)
                {
                    if( profile.SharedLibraryExtensions.find(platformNames.at(j)) == 
                        profile.SharedLibraryExtensions.end())
                    {
                        if(j == platformNames.size() - 1)
                        {
                            ssLOG_ERROR("Failed to find shared library extensions for dependency " << 
                                        scriptInfo.Dependencies.at(i).Name);
                            
                            return false;
                        }
                        
                        continue;
                    }
                    
                    extensionsToCopy = profile.SharedLibraryExtensions.at(platformNames.at(j));
                    break;
                }
                
                break;
            }
            case DependencyLibraryType::OBJECT:
            {
                for(int j = 0; j < platformNames.size(); ++j)
                {
                    if( profile.ObjectFileExtensions.find(platformNames.at(j)) == 
                        profile.ObjectFileExtensions.end())
                    {
                        if(j == platformNames.size() - 1)
                        {
                            ssLOG_ERROR("Failed to find shared library extensions for dependency " << 
                                        scriptInfo.Dependencies.at(i).Name);
                            
                            return false;
                        }
                        
                        continue;
                    }
                    
                    extensionsToCopy.push_back(profile.ObjectFileExtensions.at(platformNames.at(j)));
                    break;
                }
                
                break;
            }
            case DependencyLibraryType::HEADER:
                break;
            default:
                ssLOG_ERROR("Invalid library type: " << (int)scriptInfo.Dependencies.at(i).LibraryType);
                return false;
        }
    
        for(int j = 0; j < platformNames.size(); ++j)
        {
            if( profile.DebugSymbolFileExtensions.find(platformNames.at(j)) == 
                profile.DebugSymbolFileExtensions.end())
            {
                continue;
            }
            
            const std::vector<std::string>& debugSymbolExtensions = profile .DebugSymbolFileExtensions
                                                                            .at(platformNames.at(j));
            
            extensionsToCopy.insert(extensionsToCopy.end(), 
                                    debugSymbolExtensions.begin(), 
                                    debugSymbolExtensions.end());

            break;
        }
        
        if(scriptInfo.Dependencies.at(i).LibraryType == DependencyLibraryType::HEADER)
            return true;
        
        //Get the Search path and search library name
        if( scriptInfo.Dependencies.at(i).SearchProperties.find(profile.Name) == 
            scriptInfo.Dependencies.at(i).SearchProperties.end())
        {
            ssLOG_ERROR("Search properties for dependency " << scriptInfo.Dependencies.at(i).Name <<
                        " is missing profile " << profile.Name);
            
            return false;
        }
        
        //Copy the files with extensions that contains the search name
        std::string searchLibraryName = scriptInfo  .Dependencies
                                                    .at(i)
                                                    .SearchProperties
                                                    .at(profile.Name).SearchLibraryName;
    
        std::string searchPath = scriptInfo .Dependencies
                                            .at(i)
                                            .SearchProperties
                                            .at(profile.Name).SearchPath;
    
        if(!ghc::filesystem::path(searchPath).is_absolute())
            searchPath = scriptDirectory + "/" + searchPath;
    
        std::error_code _;
        if(!ghc::filesystem::exists(searchPath, _) || !ghc::filesystem::is_directory(searchPath, _))
        {
            ssLOG_ERROR("Invalid search path: " << searchPath);
            return false;
        }
    
        for(auto it :  ghc::filesystem::directory_iterator(searchPath, _))
        {
            if(it.is_directory())
                continue;
            
            std::string currentFileName = it.path().stem().string();
            std::string currentExtension = it.path().extension().string();
            
            //TODO: Make it not case sensitive?
            bool nameMatched = false;
            if(currentFileName.find(searchLibraryName) != std::string::npos)
                nameMatched = true;
            
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
            
            if(!ghc::filesystem::copy_file( it.path(), 
                                            runcpp2ScriptDir, 
                                            ghc::filesystem::copy_options::overwrite_existing,  
                                            _))
            {
                ssLOG_ERROR("Failed to copy file from " << it.path().string() << 
                            " to " << runcpp2ScriptDir);

                return false;
            }
        }
    }
    
    return true;
}

bool runcpp2::CompileAndLinkScript( const std::string& scriptPath, 
                                    const ScriptInfo& scriptInfo,
                                    const CompilerProfile& profile)
{
    std::string scriptDirectory = ghc::filesystem::path(scriptPath).parent_path().string();
    std::string scriptName = ghc::filesystem::path(scriptPath).stem().string();
    std::string runcpp2ScriptDir = Internal::ProcessPath(scriptDirectory + "/.runcpp2");
    std::vector<std::string> platformNames = Internal::GetPlatformNames();
    
    std::string compileCommand =    profile.Compiler.Executable + " " + 
                                    profile.Compiler.CompileArgs;

    //Replace for {CompileFlags}
    const std::string compileFlagSubstitution = "{CompileFlags}";
    std::size_t foundIndex = compileCommand.find(compileFlagSubstitution);
    
    std::string compileArgs = profile.Compiler.DefaultCompileFlags;
    
    if(foundIndex == std::string::npos)
    {
        ssLOG_ERROR("'{CompileFlags}' missing in CompileArgs");
        return false;
    }
    
    if(scriptInfo.OverrideCompileFlags.find(profile.Name) != scriptInfo.OverrideCompileFlags.end())
    {
        std::vector<std::string> compileArgsToRemove; 
        Internal::SplitString(  scriptInfo.OverrideCompileFlags.at(profile.Name).Remove, 
                                " ", 
                                compileArgsToRemove);
        
        for(int i = 0; i < compileArgsToRemove.size(); ++i)
        {
            std::size_t foundIndex = compileArgs.find(compileArgsToRemove.at(i));
            
            if(foundIndex != std::string::npos)
            {
                compileArgs.erase(foundIndex, compileArgsToRemove.at(i).size() + 1);
                continue;
            }
            
            ssLOG_WARNING("Compile flag to remove not found: " << compileArgsToRemove.at(i));
        }
        
        Internal::TrimRight(compileArgs);
        compileArgs += " " + scriptInfo.OverrideCompileFlags.at(profile.Name).Append;
    }
    
    compileCommand.replace( foundIndex, 
                            compileFlagSubstitution.size(), 
                            compileArgs);
    
    //Replace {InputFile}
    const std::string inputFileSubstitution = "{InputFile}";
    foundIndex = compileCommand.find(inputFileSubstitution);
    if(foundIndex == std::string::npos)
    {
        ssLOG_ERROR("'{InputFile}' missing in CompileArgs");
        return false;
    }
    compileCommand.replace(foundIndex, inputFileSubstitution.size(), scriptPath);
    
    //Replace {ObjectFile}
    const std::string objectFileSubstitution = "{ObjectFile}";
    std::string objectFileExt; 
    
    for(int i = 0; i < platformNames.size(); ++i)
    {
        if(profile.ObjectFileExtensions.find(platformNames.at(i)) != profile.ObjectFileExtensions.end())
        {
            objectFileExt = profile.ObjectFileExtensions.at(platformNames.at(i));
            break;
        }
    }
    
    foundIndex = compileCommand.find(objectFileSubstitution);
    if(foundIndex == std::string::npos)
    {
        ssLOG_ERROR("'{ObjectFile}' missing in CompileArgs");
        return false;
    }
    
    std::string objectFileName = Internal::ProcessPath( runcpp2ScriptDir + "/" + 
                                                        scriptName + "." + objectFileExt);
    
    compileCommand.replace(foundIndex, objectFileSubstitution.size(), objectFileName);
    
    //Compile the script
    ssLOG_INFO("running compile command: " << compileCommand);
    
    System2CommandInfo compileCommandInfo;
    SYSTEM2_RESULT result = System2Run(compileCommand.c_str(), &compileCommandInfo);
    
    if(result != SYSTEM2_RESULT_SUCCESS)
    {
        ssLOG_ERROR("System2Run failed with result: " << result);
        return false;
    }
    
    std::vector<char> output;
    
    do
    {
        uint32_t byteRead = 0;
        output.resize(output.size() + 4096);
        
        result = System2ReadFromOutput( &compileCommandInfo, 
                                        output.data() + output.size() - 4096, 
                                        4096 - 1, 
                                        &byteRead);

        output.resize(output.size() - 4096+ byteRead + 1);
        output.back() = '\0';
    }
    while(result == SYSTEM2_RESULT_READ_NOT_FINISHED);
    
    if(result != SYSTEM2_RESULT_SUCCESS)
    {
        ssLOG_ERROR("Failed to read from output with result: " << result);
        return false;
    }
    
    ssLOG_DEBUG("Compile Output: \n" << output.data());
    
    int statusCode = 0;
    result = System2GetCommandReturnValueSync(&compileCommandInfo, &statusCode);
    
    if(result != SYSTEM2_RESULT_SUCCESS)
    {
        ssLOG_ERROR("System2GetCommandReturnValueSync failed with result: " << result);
        return false;
    }
    
    if(statusCode != 0)
    {
        ssLOG_ERROR("Compile command returned with non-zero status code: " << statusCode);
        return false;
    }
    
    //Link the script to the dependencies
    std::string linkCommand = profile.Linker.Executable + " ";
    std::string currentOutputPart = profile.Linker.LinkerArgs.OutputPart;
    const std::string linkFlagsSubstitution = "{LinkFlags}";
    const std::string outputFileSubstitution = "{OutputFile}";
    
    std::string linkFlags = profile.Linker.DefaultLinkFlags;
    std::string outputName = scriptName;
    
    if(scriptInfo.OverrideLinkFlags.find(profile.Name) != scriptInfo.OverrideLinkFlags.end())
    {
        std::vector<std::string> linkFlagsToRemove;
        
        Internal::SplitString(  scriptInfo.OverrideLinkFlags.at(profile.Name).Remove, 
                                " ", 
                                linkFlagsToRemove);
        
        for(int i = 0; i < linkFlagsToRemove.size(); ++i)
        {
            std::size_t foundIndex = linkFlags.find(linkFlagsToRemove.at(i));
            
            if(foundIndex != std::string::npos)
            {
                linkFlags.erase(foundIndex, linkFlagsToRemove.at(i).size() + 1);
                continue;
            }
            
            ssLOG_WARNING("Link flag to remove not found: " << linkFlagsToRemove.at(i));
        }
        
        Internal::TrimRight(linkFlags);
        linkFlags += " " + scriptInfo.OverrideLinkFlags.at(profile.Name).Append;
    }
    
    foundIndex = currentOutputPart.find(linkFlagsSubstitution);
    if(foundIndex == std::string::npos)
    {
        ssLOG_ERROR("'{LinkFlags}' missing in LinkerArgs");
        return false;
    }
    
    currentOutputPart.replace(foundIndex, linkFlagsSubstitution.size(), linkFlags);
    
    foundIndex = currentOutputPart.find(outputFileSubstitution);
    if(foundIndex == std::string::npos)
    {
        ssLOG_ERROR("'{ProgramName}' missing in LinkerArgs");
        return false;
    }
    
    currentOutputPart.replace(foundIndex, outputFileSubstitution.size(), outputName);
    
    foundIndex = currentOutputPart.find(objectFileSubstitution);
    if(foundIndex == std::string::npos)
    {
        ssLOG_ERROR("'{ObjectFile}' missing in LinkerArgs");
        return false;
    }
    
    currentOutputPart.replace(foundIndex, objectFileSubstitution.size(), objectFileName);
    Internal::Trim(currentOutputPart);
    linkCommand += currentOutputPart;
    
    const std::string dependencySubstitution = "{DependencyName}";
    const std::string dependencyPart = profile.Linker.LinkerArgs.DependenciesPart;
    
    foundIndex = dependencyPart.find(dependencySubstitution);
    if(foundIndex == std::string::npos)
    {
        ssLOG_ERROR("'{DependencyName}' missing in LinkerArgs");
        return false;
    }
    
    for(int i = 0; i < scriptInfo.Dependencies.size(); ++i)
    {
        if(scriptInfo.Dependencies.at(i).LibraryType == DependencyLibraryType::HEADER)
            continue;
        
        std::string dependencyName = scriptInfo .Dependencies
                                                .at(i)
                                                .SearchProperties
                                                .at(profile.Name)
                                                .SearchLibraryName;
        
        std::string currentDependencyPart = dependencyPart;
        currentDependencyPart.replace(foundIndex, dependencySubstitution.size(), dependencyName);
        
        if(i == 0)
            linkCommand += " ";
        
        linkCommand += currentDependencyPart;
    }
    
    linkCommand = "cd " + runcpp2ScriptDir + " && " + linkCommand;
    
    System2CommandInfo linkCommandInfo;
    result = System2Run(linkCommand.c_str(), &linkCommandInfo);
    
    ssLOG_INFO("running link command: " << linkCommand);
    
    if(result != SYSTEM2_RESULT_SUCCESS)
    {
        ssLOG_ERROR("System2Run failed with result: " << result);
        return false;
    }
    
    output.clear();
    do
    {
        uint32_t byteRead = 0;
        
        output.resize(output.size() + 4096);
        
        result = System2ReadFromOutput( &linkCommandInfo, 
                                        output.data() + output.size() - 4096, 
                                        4096 - 1, 
                                        &byteRead);
        
        output.resize(output.size() - 4096 + byteRead + 1);
        output.back() = '\0';
    }
    while(result == SYSTEM2_RESULT_READ_NOT_FINISHED);
    
    if(result != SYSTEM2_RESULT_SUCCESS)
    {
        ssLOG_ERROR("Failed to read from output with result: " << result);
        return false;
    }
    
    ssLOG_DEBUG("Link Output: \n" << output.data());
    
    statusCode = 0;
    result = System2GetCommandReturnValueSync(&linkCommandInfo, &statusCode);
    
    if(result != SYSTEM2_RESULT_SUCCESS)
    {
        ssLOG_ERROR("System2GetCommandReturnValueSync failed with result: " << result);
        return false;
    }
    
    if(statusCode != 0)
    {
        ssLOG_ERROR("Link command returned with non-zero status code: " << statusCode);
        return false;
    }
    
    
    return true;
}

bool runcpp2::IsProfileAvailableOnSystem(const CompilerProfile& profile)
{
    //Check compiler
    std::string command = profile.Compiler.Executable + " -v";
    
    System2CommandInfo compilerCommandInfo;
    SYSTEM2_RESULT sys2Result = System2Run(command.c_str(), &compilerCommandInfo);
    
    if(sys2Result != SYSTEM2_RESULT_SUCCESS)
    {
        ssLOG_ERROR("System2Run failed with result: " << sys2Result);
        return false;
    }
    
    int returnCode = 0;
    sys2Result = System2GetCommandReturnValueSync(&compilerCommandInfo, &returnCode);
    if(sys2Result != SYSTEM2_RESULT_SUCCESS)
    {
        ssLOG_ERROR("System2GetCommandReturnValueSync failed with result: " << sys2Result);
        return false;
    }
    
    if(returnCode != 0)
        return false;
    
    //Check linker
    command = profile.Linker.Executable + " -v";
    
    System2CommandInfo linkerCommandInfo;
    sys2Result = System2Run(command.c_str(), &linkerCommandInfo);
    
    if(sys2Result != SYSTEM2_RESULT_SUCCESS)
    {
        ssLOG_ERROR("System2Run failed with result: " << sys2Result);
        return false;
    }
    
    returnCode = 0;
    sys2Result = System2GetCommandReturnValueSync(&linkerCommandInfo, &returnCode);
    if(sys2Result != SYSTEM2_RESULT_SUCCESS)
    {
        ssLOG_ERROR("System2GetCommandReturnValueSync failed with result: " << sys2Result);
        return false;
    }
    
    if(returnCode != 0)
        return false;

    return true;
}

bool runcpp2::IsProfileValidForScript(  const CompilerProfile& profile, 
                                        const ScriptInfo& scriptInfo, 
                                        const std::string& scriptPath)
{
    std::string scriptExtension = ghc::filesystem::path(scriptPath).extension().string();
    
    if(profile.FileExtensions.find(scriptExtension.substr(1)) == profile.FileExtensions.end())
        return false;
    
    if(!scriptInfo.Language.empty())
    {
        if(profile.Languages.find(scriptInfo.Language) == profile.Languages.end())
            return false;
    }
    
    if(!scriptInfo.RequiredProfiles.empty())
    {
        std::vector<PlatformName> platformNames = Internal::GetPlatformNames();
        
        for(int i = 0; i < platformNames.size(); ++i)
        {
            if(scriptInfo.RequiredProfiles.find(platformNames.at(i)) == scriptInfo.RequiredProfiles.end())
                continue;
            
            const std::vector<ProfileName> allowedProfileNames = scriptInfo .RequiredProfiles
                                                                            .at(platformNames.at(i));

            for(int j = 0; j < allowedProfileNames.size(); ++j)
            {
                if(allowedProfileNames.at(j) == profile.Name)
                    return true;
            }
            
            //If we went through all the specified profile names, exit
            return false;
        }
        
        //If we went through all the specified platform names for required profiles, exit
        return false;
    }
    
    return true;
}

std::vector<ProfileName> runcpp2::GetAvailableProfiles( const std::vector<CompilerProfile>& profiles,
                                                        const ScriptInfo& scriptInfo,
                                                        const std::string& scriptPath)
{
    //Check which compiler is available
    std::vector<ProfileName> availableProfiles;
    
    for(int i = 0; i < profiles.size(); ++i)
    {
        if(IsProfileAvailableOnSystem(profiles.at(i)) && IsProfileValidForScript(   profiles.at(i), 
                                                                                    scriptInfo, 
                                                                                    scriptPath))
        {
            availableProfiles.push_back(profiles.at(i).Name);
        }
    }
    
    return availableProfiles;
}


int runcpp2::GetPreferredProfileIndex(  const std::string& scriptPath, 
                                        const ScriptInfo& scriptInfo,
                                        const std::vector<CompilerProfile>& profiles, 
                                        const std::string& configPreferredProfile)
{
    std::vector<ProfileName> availableProfiles = GetAvailableProfiles(  profiles, 
                                                                        scriptInfo, 
                                                                        scriptPath);
    
    if(availableProfiles.empty())
    {
        ssLOG_ERROR("No compilers/linkers found");
        return -1;
    }
    
    int firstAvailableProfileIndex = -1;
    
    if(!configPreferredProfile.empty())
    {
        for(int i = 0; i < profiles.size(); ++i)
        {
            bool available = false;
            for(int j = 0; j < availableProfiles.size(); ++j)
            {
                if(availableProfiles.at(j) == profiles.at(i).Name)
                {
                    available = true;
                    break;
                }
            }
            
            if(!available)
                continue;
            
            if(firstAvailableProfileIndex == -1)
                firstAvailableProfileIndex = i;
            
            if(profiles.at(i).Name == configPreferredProfile)
                return i;
        }
    }
    
    return firstAvailableProfileIndex;
}

bool runcpp2::RunScript(const std::string& scriptPath, 
                        const std::vector<CompilerProfile>& profiles,
                        const std::string& configPreferredProfile,
                        const std::string& runArgs)
{
    if(profiles.empty())
    {
        ssLOG_ERROR("No compiler profiles found");
        return false;
    }
    
    //Check if input file exists
    {
        std::error_code _;
        
        if(!ghc::filesystem::exists(scriptPath, _))
        {
            ssLOG_ERROR("Failed to check if file exists: " << scriptPath);
            return false;
        }
        
        if(ghc::filesystem::is_directory(scriptPath, _))
        {
            ssLOG_ERROR("The input file must not be a directory: " << scriptPath);
            return false;
        }
    }
    
    std::string absoluteScriptPath = ghc::filesystem::absolute(scriptPath).string();
    std::string scriptDirectory = ghc::filesystem::path(absoluteScriptPath).parent_path().string();
    std::string scriptName = ghc::filesystem::path(absoluteScriptPath).stem().string();

    ssLOG_DEBUG("scriptPath: " << scriptPath);
    ssLOG_DEBUG("absoluteScriptPath: " << absoluteScriptPath);
    ssLOG_DEBUG("scriptDirectory: " << scriptDirectory);
    ssLOG_DEBUG("scriptName: " << scriptName);
    
    ssLOG_DEBUG("is_directory: " << ghc::filesystem::is_directory(scriptDirectory));
    

    //Read from a c/cpp file
    //TODO: Check is it c or cpp

    std::ifstream inputFile(absoluteScriptPath);
    if (!inputFile)
    {
        ssLOG_ERROR("Failed to open file: " << absoluteScriptPath);
        return false;
    }

    std::stringstream buffer;
    buffer << inputFile.rdbuf();
    std::string source(buffer.str());

    std::string parsableInfo;
    if(!runcpp2::Internal::GetParsableInfo(source, parsableInfo))
    {
        ssLOG_ERROR("An error has been encountered when parsing info: " << absoluteScriptPath);
        return false;
    }
    
    //TODO: Check if there's script info as yaml file instead

    //Try to parse the runcpp2 info
    ScriptInfo scriptInfo;
    if(!runcpp2::ParseScriptInfo(parsableInfo, scriptInfo))
    {
        ssLOG_ERROR("Failed to parse info");
        ssLOG_ERROR("Content trying to parse: " << "\n" << parsableInfo);
        return false;
    }
    
    if(!parsableInfo.empty())
    {
        ssLOG_LINE("\n" << scriptInfo.ToString(""));
    }

    if(!CreateRuncpp2ScriptDirectory(absoluteScriptPath))
    {
        ssLOG_ERROR("Failed to create runcpp2 script directory: " << absoluteScriptPath);
        return false;
    }

    int profileIndex = GetPreferredProfileIndex(absoluteScriptPath, 
                                                scriptInfo, 
                                                profiles, 
                                                configPreferredProfile);

    if(profileIndex == -1)
    {
        ssLOG_ERROR("Failed to find a profile to run");
        return false;
    }

    //TODO(NOW): Pass reset dependencies from commandline arg
    std::vector<std::string> dependenciesLocalCopiesPaths;
    std::vector<std::string> dependenciesSourcePaths;
    
    if(!SetupScriptDependencies(profiles.at(profileIndex).Name, 
                                absoluteScriptPath, 
                                scriptInfo, 
                                false,
                                dependenciesLocalCopiesPaths,
                                dependenciesSourcePaths))
    {
        ssLOG_ERROR("Failed to setup script dependencies");
        return false;
    }

    if(!CopyDependenciesBinaries(   absoluteScriptPath, 
                                    scriptInfo,
                                    dependenciesLocalCopiesPaths,
                                    profiles.at(profileIndex)))
    {
        ssLOG_ERROR("Failed to copy dependencies binaries");
        return false;
    }

    if(!CompileAndLinkScript(   absoluteScriptPath, 
                                scriptInfo,
                                profiles.at(profileIndex)))
    {
        ssLOG_ERROR("Failed to compile or link script");
        return false;
    }

    //Run the script
    std::string exeExt = "";
    #ifdef _WIN32
        exeExt = ".exe";
    #endif
    
    std::string exeToCopy = scriptDirectory + "/.runcpp2/" + scriptName + exeExt;
    
    if(!ghc::filesystem::exists(exeToCopy))
    {
        ssLOG_ERROR("Failed to find the compiled file: " << exeToCopy);
        return false;
    }
    
    std::error_code _;
    if(!ghc::filesystem::copy_file(exeToCopy, scriptDirectory + "/" + scriptName + exeExt, _))
    {
        ssLOG_ERROR("Failed to copy file from " << exeToCopy << " to " << scriptDirectory);
        ssLOG_ERROR("Error code: " << _.message());
        return false;
    }

    std::string runCommand = Internal::ProcessPath( "cd " + scriptDirectory + 
                                                    " && ./" + scriptName + exeExt);
    
    if(!runArgs.empty())
        runCommand += " " + runArgs;
    
    ssLOG_INFO("Running: " << runCommand);
    
    System2CommandInfo runCommandInfo;
    SYSTEM2_RESULT result = System2Run(runCommand.c_str(), &runCommandInfo);
    
    if(result != SYSTEM2_RESULT_SUCCESS)
    {
        ssLOG_ERROR("System2Run failed with result: " << result);
        ghc::filesystem::remove(scriptDirectory + "/" + std::string(scriptName + exeExt), _);
        return false;
    }
    
    std::vector<char> output;
    do
    {
        uint32_t byteRead = 0;
        output.resize(output.size() + 4096);
        
        result = System2ReadFromOutput( &runCommandInfo, 
                                        output.data() + output.size() - 4096, 
                                        4096 - 1, 
                                        &byteRead);

        output.resize(output.size() - 4096 + byteRead + 1);
        output.back() = '\0';
    }
    while(result == SYSTEM2_RESULT_READ_NOT_FINISHED);
    
    if(result != SYSTEM2_RESULT_SUCCESS)
    {
        ssLOG_ERROR("Failed to read from output with result: " << result);
        ghc::filesystem::remove(scriptDirectory + "/" + std::string(scriptName + exeExt), _);
        return false;
    }
    
    ssLOG_INFO("Run Output: \n" << output.data());
    
    int statusCode = 0;
    result = System2GetCommandReturnValueSync(&runCommandInfo, &statusCode);
    
    if(result != SYSTEM2_RESULT_SUCCESS)
    {
        ssLOG_ERROR("System2GetCommandReturnValueSync failed with result: " << result);
        ghc::filesystem::remove(scriptDirectory + "/" + std::string(scriptName + exeExt), _);
        return false;
    }
    
    if(statusCode != 0)
    {
        ssLOG_ERROR("Run command returned with non-zero status code: " << statusCode);
        ghc::filesystem::remove(scriptDirectory + "/" + std::string(scriptName + exeExt), _);
        return false;
    }
    
    ghc::filesystem::remove(scriptDirectory + "/" + std::string(scriptName + exeExt), _);
    return true;
}

