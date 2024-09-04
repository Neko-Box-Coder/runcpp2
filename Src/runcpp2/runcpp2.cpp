#include "runcpp2/runcpp2.hpp"

#include "runcpp2/ProfileHelper.hpp"
#include "runcpp2/CompilingLinking.hpp"
#include "runcpp2/ConfigParsing.hpp"
#include "runcpp2/DependenciesHelper.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "runcpp2/PlatformUtil.hpp"
#include "runcpp2/BuildsManager.hpp"
#include "System2.h"
#include "ssLogger/ssLog.hpp"
#include "ghc/filesystem.hpp"
#include "dylib.hpp"

#include <fstream>
#include <chrono>

extern const uint8_t DefaultScriptInfo[];
extern const size_t DefaultScriptInfo_size;

namespace
{
    bool CreateLocalBuildDirectory( const std::string& scriptPath, 
                                    ghc::filesystem::path& outBuildPath)
    {
        std::string scriptDirectory = ghc::filesystem::path(scriptPath).parent_path().string();
        
        //Create the runcpp2 directory
        std::string runcpp2Dir = scriptDirectory + "/.runcpp2";
        
        std::error_code e;
        if(!ghc::filesystem::exists(runcpp2Dir, e))
        {
            if(!ghc::filesystem::create_directory(runcpp2Dir, e))
            {
                ssLOG_ERROR("Failed to create runcpp2 directory");
                return false;
            }
        }
        
        //Using builds manager in local builds directory instead
        {
            runcpp2::BuildsManager buildsManager(runcpp2Dir);
            if(!buildsManager.Initialize())
            {
                ssLOG_FATAL("Failed to initialize builds manager");
                return false;
            }
            
            bool writeMapping = false;
            if(!buildsManager.HasBuildMapping(scriptPath))
                writeMapping = true;
            
            if(buildsManager.GetBuildMapping(scriptPath, outBuildPath))
            {
                if(writeMapping && !buildsManager.SaveBuildsMappings())
                {
                    ssLOG_FATAL("Failed to save builds mappings");
                    ssLOG_FATAL("Failed to create local build directory for: " << 
                                scriptPath);
                    return false;
                }
            }
        }
        return true;
    }

    bool RunCompiledScript( const ghc::filesystem::path& executable,
                            const std::string& scriptPath,
                            const std::vector<std::string>& runArgs)
    {
        INTERNAL_RUNCPP2_SAFE_START();
        ssLOG_FUNC_DEBUG();
        
        std::error_code _;
        std::string interpretedRunPath = runcpp2::ProcessPath(scriptPath);
        std::vector<const char*> args = { interpretedRunPath.c_str() };
        
        if(!runArgs.empty())
        {
            for(int i = 0; i < runArgs.size(); ++i)
                args.push_back(runArgs[i].c_str());
        }
        
        //TODO: Don't redirect output
        System2CommandInfo runCommandInfo = {};
        runCommandInfo.RedirectOutput = true;
        
        SYSTEM2_RESULT result = System2RunSubprocess(   executable.c_str(),
                                                        args.data(),
                                                        args.size(),
                                                        &runCommandInfo);
        
        ssLOG_INFO("Running: " << executable.string());
        for(int i = 0; i < runArgs.size(); ++i)
            ssLOG_INFO("-   " << runArgs[i]);
        
        if(result != SYSTEM2_RESULT_SUCCESS)
        {
            ssLOG_ERROR("System2Run failed with result: " << result);
            return false;
        }
        
        int statusCode = 0;
        do
        {
            uint32_t byteRead = 0;
            const int bufferSize = 32;
            char output[bufferSize] = {0};
            
            result = System2ReadFromOutput( &runCommandInfo, 
                                            output, 
                                            bufferSize - 1, 
                                            &byteRead);

            output[byteRead] = '\0';
            
            //Log the output and continue fetching output
            if(result == SYSTEM2_RESULT_READ_NOT_FINISHED)
            {
                std::cout << output << std::flush;
                continue;
            }
            
            //If we have finished reading the output, check if the command has finished
            if(result == SYSTEM2_RESULT_SUCCESS)
            {
                std::cout << output << std::flush;
                
                result = System2GetCommandReturnValueAsync(&runCommandInfo, &statusCode);
                
                //If the command is not finished, continue reading output
                if(result == SYSTEM2_RESULT_COMMAND_NOT_FINISHED)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    continue;
                }
                else
                    break;
            }
            else
            {
                ssLOG_ERROR("Failed to read from output with result: " << result);
                return false;
            }
        }
        while(true);
        
        if(result != SYSTEM2_RESULT_SUCCESS)
        {
            ssLOG_ERROR("System2GetCommandReturnValueASync failed with result: " << result);
            return false;
        }
        
        if(statusCode != 0)
        {
            ssLOG_ERROR("Run command returned with non-zero status code: " << statusCode);
            return false;
        }
        
        return true;
        INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
    }
    
    int RunCompiledSharedLib(   const std::string& scriptPath,
                                const ghc::filesystem::path& compiledSharedLibPath,
                                const std::vector<std::string>& runArgs)
    {
        INTERNAL_RUNCPP2_SAFE_START();
        ssLOG_FUNC_DEBUG();
        
        std::error_code _;
        if(!ghc::filesystem::exists(compiledSharedLibPath, _))
        {
            ssLOG_ERROR("Failed to find shared library: " << compiledSharedLibPath.string());
            return -1;
        }
        
        //Load it
        std::unique_ptr<dylib> sharedLib;
        
        try
        {
             sharedLib = std::unique_ptr<dylib>(new dylib(  compiledSharedLibPath.string(), 
                                                            dylib::no_filename_decorations));
        }
        catch(std::exception& e)
        {
            ssLOG_ERROR("Failed to load shared library " << compiledSharedLibPath.string() << 
                        " with exception: ");
            
            ssLOG_ERROR(e.what());
            return -1;
        }
        
        //Get main as entry point
        if(sharedLib->has_symbol("main") == false)
        {
            ssLOG_ERROR("The shared library does not have a main function");
            return -1;
        }
        
        int (*scriptFullMain)(int, char**) = nullptr;
        int (*scriptMain)() = nullptr;
        
        try
        {
            scriptFullMain = sharedLib->get_function<int(int, char**)>("main");
        }
        catch(const dylib::exception& ex)
        {
            ssLOG_DEBUG("Failed to get full main function from shared library: " << ex.what());
        }
        catch(...)
        {
            ssLOG_ERROR("Failed to get entry point function");
            return -1;
        }
        
        if(scriptFullMain == nullptr)
        {
            try
            {
                scriptMain = sharedLib->get_function<int()>("_main");
            }
            catch(const dylib::exception& ex)
            {
                ssLOG_DEBUG("Failed to get main function from shared library: " << ex.what());
            }
            catch(...)
            {
                ssLOG_ERROR("Failed to get entry point function");
                return -1;
            }
        }
        
        if(scriptMain == nullptr && scriptFullMain == nullptr)
        {
            ssLOG_ERROR("Failed to load function");
            return -1;
        }
        
        //Run the entry point
        int result = 0;
        try
        {
            if(scriptFullMain != nullptr)
            {
                std::vector<std::string> runArgsCopy = runArgs;
                runArgsCopy.insert(runArgsCopy.begin(), scriptPath);
                
                std::vector<char*> runArgsCStr(runArgsCopy.size());
                for(int i = 0; i < runArgsCopy.size(); ++i)
                    runArgsCStr[i] = &runArgsCopy[i][0];
                
                result = scriptFullMain(runArgsCStr.size(), runArgsCStr.data());
            }
            else if(scriptMain != nullptr)
                result = scriptMain();
        }
        catch(std::exception& e)
        {
            ssLOG_ERROR("Failed to run script main with exception: " << e.what());
            return -1;
        }
        
        return result;

        INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(-1);
    }
    
    bool HasCompiledCache(  const std::unordered_map<   runcpp2::CmdOptions, 
                                                        std::string>& currentOptions,
                            const std::string& absoluteScriptPath,
                            const std::string& exeExt,
                            const ghc::filesystem::path& buildDir,
                            const std::vector<runcpp2::Data::Profile>& profiles,
                            int profileIndex)
    {
        ssLOG_FUNC_DEBUG();
        
        std::string scriptName = ghc::filesystem::path(absoluteScriptPath).stem().string();
        std::error_code _;
        ghc::filesystem::file_time_type lastScriptWriteTime = 
            ghc::filesystem::last_write_time(absoluteScriptPath, _);
        
        if(currentOptions.find(runcpp2::CmdOptions::RESET_CACHE) != currentOptions.end())
            return false;
        
        //If we are compiling to an executable
        if(currentOptions.find(runcpp2::CmdOptions::EXECUTABLE) != currentOptions.end())
        {
            ghc::filesystem::path exeToCopy = buildDir / ".runcpp2" / scriptName;
            exeToCopy.concat(exeExt);
            
            ssLOG_INFO("Trying to use cache: " << exeToCopy.string());
            
            //If the executable already exists, check if it's newer than the script
            if(ghc::filesystem::exists(exeToCopy, _) && ghc::filesystem::file_size(exeToCopy, _) > 0)
            {
                ghc::filesystem::file_time_type lastExecutableWriteTime = 
                    ghc::filesystem::last_write_time(exeToCopy, _);
                
                if(lastExecutableWriteTime < lastScriptWriteTime)
                {
                    ssLOG_INFO("Compiled file is older than the source file");
                    return false;
                }
                else
                    return true;
            }
        }
        //If we are compiling to a shared library
        else
        {
            //Check if there's any existing shared library build that is newer than the script
            const std::string* targetSharedLibExt = 
                runcpp2::GetValueFromPlatformMap(profiles.at(profileIndex)  .FilesTypes
                                                                            .SharedLibraryFile
                                                                            .Extension);
            
            const std::string* targetSharedLibPrefix =
                runcpp2::GetValueFromPlatformMap(profiles.at(profileIndex)  .FilesTypes
                                                                            .SharedLibraryFile
                                                                            .Prefix);
            
            if(targetSharedLibExt == nullptr || targetSharedLibPrefix == nullptr)
            {
                ssLOG_ERROR("Shared library extension or prefix not found in compiler profile");
                return false;
            }
            
            ghc::filesystem::path sharedLibBuild = buildDir / ".runcpp2" / *targetSharedLibPrefix;
            sharedLibBuild.concat(scriptName).concat(*targetSharedLibExt);
            
            ssLOG_INFO("Trying to use cache: " << sharedLibBuild.string());
            
            if( ghc::filesystem::exists(sharedLibBuild, _) && 
                ghc::filesystem::file_size(sharedLibBuild, _) > 0)
            {
                ghc::filesystem::file_time_type lastSharedLibWriteTime = 
                    ghc::filesystem::last_write_time(sharedLibBuild, _);
                
                if(lastSharedLibWriteTime < lastScriptWriteTime)
                    ssLOG_INFO("Compiled file is older than the source file");
                else
                    return true;
            }
        }
        
        return false;
    }
}

void runcpp2::GetDefaultScriptInfo(std::string& scriptInfo)
{
    scriptInfo = std::string(reinterpret_cast<const char*>(DefaultScriptInfo), DefaultScriptInfo_size);
}

int runcpp2::RunScript( const std::string& scriptPath, 
                        const std::vector<Data::Profile>& profiles,
                        const std::string& configPreferredProfile,
                        const std::unordered_map<CmdOptions, std::string> currentOptions,
                        const std::vector<std::string>& runArgs)
{
    INTERNAL_RUNCPP2_SAFE_START();
    
    ssLOG_FUNC_DEBUG();
    
    if(profiles.empty())
    {
        ssLOG_ERROR("No compiler profiles found");
        return -1;
    }
    
    //Check if input file exists
    {
        std::error_code _;
        
        if(!ghc::filesystem::exists(scriptPath, _))
        {
            ssLOG_ERROR("File does not exist: " << scriptPath);
            return -1;
        }
        
        if(ghc::filesystem::is_directory(scriptPath, _))
        {
            ssLOG_ERROR("The input file must not be a directory: " << scriptPath);
            return -1;
        }
    }
    
    //TODO: Use ghc::filesystem::path instead of string?
    std::string absoluteScriptPath = ghc::filesystem::absolute(scriptPath).string();
    std::string scriptDirectory = ghc::filesystem::path(absoluteScriptPath).parent_path().string();
    std::string scriptName = ghc::filesystem::path(absoluteScriptPath).stem().string();
    ghc::filesystem::path configDir = GetConfigFilePath();
    ghc::filesystem::path buildDir;
    
    //Parse and get the config directory
    {
        std::error_code e;
        if(ghc::filesystem::is_directory(configDir, e))
        {
            ssLOG_FATAL("Unexpected path for config file: " << configDir.string());
            return -1;
        }
        
        configDir = configDir.parent_path();
        if(!ghc::filesystem::is_directory(configDir, e))
        {
            ssLOG_FATAL("Unexpected path for config directory: " << configDir.string());
            return -1;
        }
    }
    
    //Create a class that manages build folder
    BuildsManager buildsManager(configDir);
    if(!buildsManager.Initialize())
    {
        ssLOG_FATAL("Failed to initialize builds manager");
        return -1;
    }
    
    int profileIndex = -1;

    ssLOG_DEBUG("scriptPath: " << scriptPath);
    ssLOG_DEBUG("absoluteScriptPath: " << absoluteScriptPath);
    ssLOG_DEBUG("scriptDirectory: " << scriptDirectory);
    ssLOG_DEBUG("scriptName: " << scriptName);
    ssLOG_DEBUG("is_directory: " << ghc::filesystem::is_directory(scriptDirectory));

    std::string exeExt = "";
    #ifdef _WIN32
        exeExt = ".exe";
    #endif

    //Parsing the script, setting up dependencies, compiling and linking
    {
        //Check if there's script info as yaml file instead
        std::error_code e;
        std::string parsableInfo;
        std::ifstream inputFile;
        std::string dedicatedYamlLoc = ProcessPath(scriptDirectory + "/" + scriptName + ".yaml");
        if(ghc::filesystem::exists(dedicatedYamlLoc, e))
        {
            inputFile.open(dedicatedYamlLoc);
            std::stringstream buffer;
            buffer << inputFile.rdbuf();
            parsableInfo = buffer.str();
        }
        else
        {
            inputFile.open(absoluteScriptPath);
            
            if (!inputFile)
            {
                ssLOG_ERROR("Failed to open file: " << absoluteScriptPath);
                return -1;
            }

            std::stringstream buffer;
            buffer << inputFile.rdbuf();
            std::string source(buffer.str());
            
            if(!GetParsableInfo(source, parsableInfo))
            {
                ssLOG_ERROR("An error has been encountered when parsing info: " << 
                            absoluteScriptPath);
                return -1;
            }
        }
        
        //Try to parse the runcpp2 info
        Data::ScriptInfo scriptInfo;
        if(!ParseScriptInfo(parsableInfo, scriptInfo))
        {
            ssLOG_ERROR("Failed to parse info");
            ssLOG_ERROR("Content trying to parse: " << "\n" << parsableInfo);
            return -1;
        }
        
        if(!parsableInfo.empty())
        {
            ssLOG_INFO("Parsed script info YAML:");
            ssLOG_INFO(scriptInfo.ToString(""));
        }

        //Create build directory
        {
            const bool localBuildDir = currentOptions.count(CmdOptions::LOCAL) > 0;
            bool createdBuildDir = false;
            if(localBuildDir)
            {
                if(CreateLocalBuildDirectory(absoluteScriptPath, buildDir))
                    createdBuildDir = true;
            }
            else
            {
                bool writeMapping = false;
                if(!buildsManager.HasBuildMapping(absoluteScriptPath))
                    writeMapping = true;
                
                if(buildsManager.GetBuildMapping(absoluteScriptPath, buildDir))
                {
                    if(writeMapping && !buildsManager.SaveBuildsMappings())
                        ssLOG_FATAL("Failed to save builds mappings");
                    else
                        createdBuildDir = true;
                }
            }
            
            if(!createdBuildDir)
            {
                ssLOG_FATAL("Failed to create local build directory for: " << absoluteScriptPath);
                return -1;
            }
        }

        profileIndex = GetPreferredProfileIndex(absoluteScriptPath, 
                                                scriptInfo, 
                                                profiles, 
                                                configPreferredProfile);

        if(profileIndex == -1)
        {
            ssLOG_ERROR("Failed to find a profile to run");
            return -1;
        }
        
        std::vector<std::string> copiedBinariesPaths;
        
        //Process Dependencies
        std::vector<Data::DependencyInfo*> availableDependencies;
        {
            for(int i = 0; i < scriptInfo.Dependencies.size(); ++i)
            {
                if(IsDependencyAvailableForThisPlatform(scriptInfo.Dependencies.at(i)))
                    availableDependencies.push_back(&scriptInfo.Dependencies.at(i));
            }
            
            std::vector<std::string> dependenciesLocalCopiesPaths;
            std::vector<std::string> dependenciesSourcePaths;
            if(!GetDependenciesPaths(   availableDependencies,
                                        dependenciesLocalCopiesPaths,
                                        dependenciesSourcePaths,
                                        absoluteScriptPath,
                                        buildDir))
            {
                ssLOG_ERROR("Failed to get dependencies paths");
                return -1;
            }
            
            if( currentOptions.count(CmdOptions::RESET_CACHE) > 0 ||
                currentOptions.count(CmdOptions::REMOVE_DEPENDENCIES) > 0)
            {
                if(!CleanupDependencies(profiles.at(profileIndex),
                                        scriptInfo,
                                        availableDependencies,
                                        dependenciesLocalCopiesPaths))
                {
                    ssLOG_ERROR("Failed to cleanup dependencies");
                    return -1;
                }
            }
            
            if(currentOptions.count(CmdOptions::REMOVE_DEPENDENCIES) > 0)
            {
                ssLOG_LINE("Removed script dependencies");
                return 0;
            }
            
            if(!SetupDependencies(  profiles.at(profileIndex), 
                                    buildDir,
                                    scriptInfo, 
                                    availableDependencies,
                                    dependenciesLocalCopiesPaths,
                                    dependenciesSourcePaths))
            {
                ssLOG_ERROR("Failed to setup script dependencies");
                return -1;
            }
            
            if(!BuildDependencies(  profiles.at(profileIndex),
                                    scriptInfo,
                                    availableDependencies, 
                                    dependenciesLocalCopiesPaths))
            {
                ssLOG_ERROR("Failed to build script dependencies");
                return -1;
            }

            if(!CopyDependenciesBinaries(   buildDir, 
                                            availableDependencies,
                                            dependenciesLocalCopiesPaths,
                                            profiles.at(profileIndex),
                                            copiedBinariesPaths))
            {
                ssLOG_ERROR("Failed to copy dependencies binaries");
                return -1;
            }
        }
        
        //Check if we have already compiled before.
        if(!HasCompiledCache(   currentOptions, 
                                absoluteScriptPath, 
                                exeExt, 
                                buildDir,
                                profiles, 
                                profileIndex))
        {
            if(!CompileAndLinkScript(   buildDir,
                                        absoluteScriptPath, 
                                        scriptInfo,
                                        availableDependencies,
                                        profiles.at(profileIndex),
                                        copiedBinariesPaths,
                                        (currentOptions.count(CmdOptions::EXECUTABLE) > 0),
                                        exeExt))
            {
                ssLOG_ERROR("Failed to compile or link script");
                return -1;
            }
        }
    }

    //Run the compiled file at script directory
    {
        ssLOG_INFO("Running script...");
        
        std::error_code _;
        ghc::filesystem::path target = buildDir;
        
        const std::string* targetSharedLibExt = 
            runcpp2::GetValueFromPlatformMap(profiles.at(profileIndex)  .FilesTypes
                                                                        .SharedLibraryFile
                                                                        .Extension);
        
        const std::string* targetSharedLibPrefix =
            runcpp2::GetValueFromPlatformMap(profiles.at(profileIndex)  .FilesTypes
                                                                        .SharedLibraryFile
                                                                        .Prefix);
        
        if(currentOptions.find(CmdOptions::EXECUTABLE) != currentOptions.end())
            target = (target / scriptName).concat(exeExt);
        else
        {
            if(targetSharedLibExt == nullptr || targetSharedLibPrefix == nullptr)
            {
                ssLOG_ERROR("Shared library extension or prefix not found in compiler profile");
                return -1;
            }

            target = (target / *targetSharedLibPrefix)  .concat(scriptName)
                                                        .concat(*targetSharedLibExt);
        }
        
        if(!ghc::filesystem::exists(target, _))
        {
            ssLOG_ERROR("Failed to find the compiled file: " << target.string());
            return -1;
        }
        
        if(currentOptions.count(CmdOptions::EXECUTABLE) > 0)
        {
            //Running the script
            if(!RunCompiledScript(target, absoluteScriptPath, runArgs))
            {
                ssLOG_ERROR("Failed to run script");
                return false;
            }
            
            return 0;
        }
        //Load the shared library and run it
        else
        {
            int result = RunCompiledSharedLib(  absoluteScriptPath, 
                                                target, 
                                                runArgs);
        
            return result;
        }
    }

    return -1;
    
    INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(-1);
}

