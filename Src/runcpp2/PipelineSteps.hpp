#ifndef RUNCPP2_PIPELINE_STEPS_HPP
#define RUNCPP2_PIPELINE_STEPS_HPP

#include "runcpp2/Data/Profile.hpp"
#include "runcpp2/Data/PipelineResult.hpp"
#include "runcpp2/Data/ProfilesCommands.hpp"
#include "runcpp2/Data/ScriptInfo.hpp"
#include "runcpp2/Data/CmdOptions.hpp"
#include "runcpp2/Data/BuildTypeHelper.hpp"
#include "runcpp2/Data/BuildType.hpp"
#include "runcpp2/Data/DependencyInfo.hpp"
#include "runcpp2/Data/FileProperties.hpp"
#include "runcpp2/Data/FilesTypesInfo.hpp"
#include "runcpp2/Data/ParseCommon.hpp"
#include "runcpp2/Data/ProfilesFlagsOverride.hpp"
#include "runcpp2/Data/ProfilesProcessPaths.hpp"

#include "runcpp2/BuildsManager.hpp"
#include "runcpp2/IncludeManager.hpp"

#include "runcpp2/ConfigParsing.hpp"
#include "runcpp2/DependenciesHelper.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "runcpp2/PlatformUtil.hpp"

#if !defined(NOMINMAX)
    #define NOMINMAX 1
#endif

#include "ghc/filesystem.hpp"
#include "System2.h"
#include "ssLogger/ssLog.hpp"
#include "dylib.hpp"
#include "DSResult/DSResult.hpp"

#include <string>
#include <vector>
#include <unordered_map>
#include <queue>
#include <stddef.h>
#include <exception>
#include <fstream>
#include <memory>
#include <sstream>
#include <system_error>
#include <unordered_set>



namespace
{
    bool RunCompiledScript( const ghc::filesystem::path& executable,
                            const std::string& scriptPath,
                            const std::vector<std::string>& runArgs,
                            int& returnStatus)
    {
        INTERNAL_RUNCPP2_SAFE_START();
        ssLOG_FUNC_INFO();
        
        std::vector<const char*> args;
        for(size_t i = 0; i < runArgs.size(); ++i)
            args.push_back(runArgs[i].c_str());
        
        System2CommandInfo runCommandInfo = {};
        SYSTEM2_RESULT result = System2RunSubprocess(   executable.c_str(),
                                                        args.data(),
                                                        args.size(),
                                                        &runCommandInfo);
        
        ssLOG_INFO("Running: " << executable.string());
        for(size_t i = 0; i < runArgs.size(); ++i)
            ssLOG_INFO("-   " << runArgs[i]);
        
        if(result != SYSTEM2_RESULT_SUCCESS)
        {
            ssLOG_ERROR("System2Run failed with result: " << result);
            return false;
        }
        
        result = System2GetCommandReturnValueSync(&runCommandInfo, &returnStatus, false);
        if(result != SYSTEM2_RESULT_SUCCESS)
        {
            ssLOG_ERROR("System2GetCommandReturnValueSync failed with result: " << result);
            return false;
        }
        
        return true;
        INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
    }
    
    bool RunCompiledSharedLib(  const std::string& scriptPath,
                                const ghc::filesystem::path& compiledSharedLibPath,
                                const std::vector<std::string>& runArgs,
                                int& returnStatus)
    {
        INTERNAL_RUNCPP2_SAFE_START();
        ssLOG_FUNC_INFO();
        
        std::error_code _;
        if(!ghc::filesystem::exists(compiledSharedLibPath, _))
        {
            ssLOG_ERROR("Failed to find shared library: " << compiledSharedLibPath.string());
            return false;
        }
        
        //Load it
        std::unique_ptr<dylib> sharedLib;
        
        try
        {
             ssLOG_INFO("Trying to run shared library: " << compiledSharedLibPath.string());
             
             //TODO: We might want to use unicode instead for the path
             #if defined(_WIN32)
                std::string sharedLibDir = compiledSharedLibPath.parent_path().string();
                if(SetDllDirectoryA(sharedLibDir.c_str()) == FALSE)
                {
                    std::string lastError = runcpp2::GetWindowsError();
                    ssLOG_ERROR("Failed to set DLL directory: " << lastError);
                    return false;
                }
             #endif
             
             sharedLib = std::unique_ptr<dylib>(new dylib(  compiledSharedLibPath.string(), 
                                                            dylib::no_filename_decorations));
        }
        catch(std::exception& e)
        {
            ssLOG_ERROR("Failed to load shared library " << compiledSharedLibPath.string() << 
                        " with exception: ");
            
            ssLOG_ERROR(e.what());
            return false;
        }
        
        //Get main as entry point
        if(sharedLib->has_symbol("main") == false)
        {
            ssLOG_ERROR("The shared library does not have a main function");
            return false;
        }
        
        int (*scriptFullMain)(int, const char**) = nullptr;
        int (*scriptMain)() = nullptr;
        
        try
        {
            scriptFullMain = sharedLib->get_function<int(int, const char**)>("main");
        }
        catch(const dylib::exception& ex)
        {
            ssLOG_DEBUG("Failed to get full main function from shared library: " << ex.what());
        }
        catch(...)
        {
            ssLOG_ERROR("Failed to get entry point function");
            return false;
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
                return false;
            }
        }
        
        if(scriptMain == nullptr && scriptFullMain == nullptr)
        {
            ssLOG_ERROR("Failed to load function");
            return false;
        }
        
        //Run the entry point
        try
        {
            if(scriptFullMain != nullptr)
            {
                std::vector<const char*> runArgsCStr(runArgs.size());
                for(size_t i = 0; i < runArgs.size(); ++i)
                    runArgsCStr.at(i) = &runArgs.at(i).at(0);
                
                returnStatus = scriptFullMain(runArgsCStr.size(), runArgsCStr.data());
            }
            else if(scriptMain != nullptr)
                returnStatus = scriptMain();
        }
        catch(std::exception& e)
        {
            ssLOG_ERROR("Failed to run script main with exception: " << e.what());
            return true;
        }
        catch(...)
        {
            ssLOG_ERROR("Unknown exception caught");
            return true;
        }
        
        return true;

        INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
    }
}

namespace runcpp2
{
    inline DS::Result<void> CopyFiles(  const ghc::filesystem::path& destDir,
                                        const std::vector<std::string>& filePaths,
                                        std::vector<std::string>& outCopiedPaths)
    {
        ssLOG_FUNC_INFO();
        
        std::error_code e;
        for (const std::string& srcPath : filePaths)
        {
            ghc::filesystem::path destPath = destDir / ghc::filesystem::path(srcPath).filename();

            if(ghc::filesystem::exists(srcPath, e))
            {
                ghc::filesystem::copy(srcPath, 
                                      destPath, 
                                      ghc::filesystem::copy_options::update_existing, 
                                      e);
                if(e)
                {
                    std::string errorMsg =  DS_STR("Failed to copy file from ") + srcPath + " to " + 
                                            destPath.string() + "\nError: " + e.message();
                    return DS_ERROR_MSG(errorMsg);
                }
                
                ssLOG_INFO("Copied from " << srcPath << " to " << destPath.string());
                outCopiedPaths.push_back(ProcessPath(destPath));
            }
            else
                return DS_ERROR_MSG("File to copy not found: " + srcPath);
        }
        
        return {};
    }
    
    inline DS::Result<void> RunProfileCommands( const Data::ProfilesCommands* commands,
                                                const Data::Profile& profile,
                                                const std::string& workingDir,
                                                const std::string& commandType)
    {
        ssLOG_FUNC_INFO();
        
        if(commands != nullptr)
        {
            const std::vector<std::string>* commandSteps = 
                runcpp2::GetValueFromProfileMap(profile, commands->CommandSteps);
                
            if(commandSteps != nullptr)
            {
                for(const std::string& cmd : *commandSteps)
                {
                    std::string output;
                    int returnCode = 0;
                    
                    if(!runcpp2::RunCommand(cmd, true, workingDir, output, returnCode))
                    {
                        std::string errorMsg =  commandType + " command failed: " + cmd + 
                                                " with return code " + DS_STR(returnCode) + "\n";
                        errorMsg += "Was trying to run: " + cmd + "\n";
                        errorMsg += "Output: \n" + output;
                        return DS_ERROR_MSG_EC(errorMsg, (int)PipelineResult::UNEXPECTED_FAILURE);
                    }
                    
                    ssLOG_INFO(commandType << " command ran: \n" << cmd);
                    ssLOG_INFO(commandType << " command output: \n" << output);
                }
            }
        }
        
        return {};
    }

    inline DS::Result<void> ValidateInputs( const std::string& scriptPath, 
                                            const std::vector<Data::Profile>& profiles,
                                            ghc::filesystem::path& outAbsoluteScriptPath,
                                            ghc::filesystem::path& outScriptDirectory,
                                            std::string& outScriptName)
    {
        ssLOG_FUNC_INFO();
        
        if(profiles.empty())
            return DS_ERROR_MSG_EC("No compiler profiles found", (int)PipelineResult::EMPTY_PROFILES);

        //Check if input file exists
        std::error_code _;
        if(!ghc::filesystem::exists(scriptPath, _))
        {
            return DS_ERROR_MSG_EC( "File does not exist: " + scriptPath, 
                                    (int)PipelineResult::INVALID_SCRIPT_PATH);
        }
        
        if(ghc::filesystem::is_directory(scriptPath, _))
        {
            return DS_ERROR_MSG_EC( "The input file must not be a directory: " + scriptPath,
                                    (int)PipelineResult::INVALID_SCRIPT_PATH);
        }

        outAbsoluteScriptPath = ghc::filesystem::absolute(ghc::filesystem::canonical(scriptPath, _));
        outScriptDirectory = outAbsoluteScriptPath.parent_path();
        outScriptName = outAbsoluteScriptPath.stem().string();

        ssLOG_DEBUG("scriptPath: " << scriptPath);
        ssLOG_DEBUG("absoluteScriptPath: " << outAbsoluteScriptPath.string());
        ssLOG_DEBUG("scriptDirectory: " << outScriptDirectory.string());
        ssLOG_DEBUG("scriptName: " << outScriptName);
        ssLOG_DEBUG("is_directory: " << ghc::filesystem::is_directory(outScriptDirectory));

        return {};
    }

    inline DS::Result<void> 
    ParseAndValidateScriptInfo( const ghc::filesystem::path& absoluteScriptPath,
                                const ghc::filesystem::path& scriptDirectory,
                                const std::string& scriptName,
                                const bool buildExecutable,
                                Data::ScriptInfo& outScriptInfo)
    {
        ssLOG_FUNC_INFO();

        //Check if there's script info as yaml file instead
        std::error_code e;
        std::string parsableInfo;
        std::ifstream inputFile;
        
        ghc::filesystem::path scriptInfoFile;
        bool dedicatedYaml = false;
        
        //If we are having yaml as input
        if(absoluteScriptPath.extension() == ".yaml" || absoluteScriptPath.extension() == ".yml")
        {
            scriptInfoFile = absoluteScriptPath;
            dedicatedYaml = true;
        }
        //Otherwise we are having source file as input
        else
        {
            //Try to see if there's a corresponding yaml file
            ghc::filesystem::path dedicatedYamlLoc = 
                scriptDirectory / ghc::filesystem::path(scriptName + ".yaml");
            
            if(ghc::filesystem::exists(dedicatedYamlLoc, e))
            {
                dedicatedYaml = true;
                scriptInfoFile = dedicatedYamlLoc;
            }
            else
                scriptInfoFile = absoluteScriptPath;
        }
        
        //Get parsable info
        {
            //Record write time for script file for watch option
            outScriptInfo.LastWriteTime = ghc::filesystem::last_write_time(scriptInfoFile, e);
            if(e)
            {
                std::string errorMsg = e.message();
                errorMsg += "\nFailed to get last write time for: " + scriptInfoFile.string();
                return DS_ERROR_MSG_EC(errorMsg, (int)PipelineResult::INVALID_SCRIPT_INFO);
            }

            inputFile.open(scriptInfoFile);
            
            if(!inputFile)
            {
                return DS_ERROR_MSG_EC( "Failed to open file: " + scriptInfoFile.string(), 
                                        (int)PipelineResult::INVALID_SCRIPT_PATH);
            }

            std::stringstream buffer;
            buffer << inputFile.rdbuf();
            
            if(dedicatedYaml)
                parsableInfo = buffer.str();
            else
            {
                GetParsableInfo(buffer.str(), parsableInfo)
                    .DS_TRY_ACT(DS_TMP_ERROR.Message += 
                                    "\nAn error has been encountered when parsing info: " + 
                                    scriptInfoFile.string();
                                DS_TMP_ERROR.ErrorCode = (int)PipelineResult::INVALID_SCRIPT_INFO;
                                DS_APPEND_TRACE(DS_TMP_ERROR);
                                return DS::Error(DS_TMP_ERROR));
            }
        }
        
        //Try to parse the runcpp2 info
        ParseScriptInfo(parsableInfo, outScriptInfo)
            .DS_TRY_ACT(DS_APPEND_TRACE(DS_TMP_ERROR);
                        DS_TMP_ERROR.Message += "\nContent trying to parse: \n" + parsableInfo;
                        DS_TMP_ERROR.ErrorCode = (int)PipelineResult::INVALID_SCRIPT_INFO;
                        return DS::Error(DS_TMP_ERROR));
        
        if(!parsableInfo.empty())
        {
            ssLOG_DEBUG("Parsed script info YAML:");
            ssLOG_DEBUG("\n" << outScriptInfo.ToString(""));
        }

        //Replace build type with internal executable type to trigger recompiling when switching to 
        //have or not have "--executable" option
        if(outScriptInfo.CurrentBuildType == Data::BuildType::EXECUTABLE)
        {
            outScriptInfo.CurrentBuildType =    buildExecutable ? 
                                                Data::BuildType::INTERNAL_EXECUTABLE_EXECUTABLE :
                                                Data::BuildType::INTERNAL_EXECUTABLE_SHARED;
        }

        return {};
    }

    inline DS::Result<void> HandleCleanup(  const Data::ScriptInfo& scriptInfo,
                                            const Data::Profile& profile,
                                            const ghc::filesystem::path& scriptDirectory,
                                            const ghc::filesystem::path& buildDir,
                                            const ghc::filesystem::path& absoluteScriptPath,
                                            BuildsManager& buildsManager)
    {
        ssLOG_FUNC_INFO();
        
        const Data::ProfilesCommands* cleanupCommands = 
            runcpp2::GetValueFromPlatformMap(scriptInfo.Cleanup);
                
        if(cleanupCommands != nullptr)
        {
            const std::vector<std::string>* commands = 
                runcpp2::GetValueFromProfileMap(profile, cleanupCommands->CommandSteps);
            if(commands != nullptr)
            {
                for(const std::string& cmd : *commands)
                {
                    std::string output;
                    int returnCode = 0;
                    
                    if(!runcpp2::RunCommand(cmd, true, scriptDirectory, output, returnCode))
                    {
                        return DS_ERROR_MSG_EC( "Cleanup command failed: " + cmd + " with return code " + 
                                                DS_STR(returnCode) + "\nOutput: \n"  + output, 
                                                (int)PipelineResult::UNEXPECTED_FAILURE);
                    }
                    
                    ssLOG_INFO("Cleanup command ran: \n" << cmd);
                    ssLOG_INFO("Cleanup command output: \n" << output);
                }
            }
        }
        
        //Remove build directory
        std::error_code e;
        if(!ghc::filesystem::remove_all(buildDir, e))
        {
            return DS_ERROR_MSG_EC( "Failed to remove build directory: " + buildDir.string(),
                                    (int)PipelineResult::UNEXPECTED_FAILURE);
        }
        
        if(!buildsManager.RemoveBuildMapping(absoluteScriptPath))
        {
            return DS_ERROR_MSG_EC( "Failed to remove build mapping", 
                                    (int)PipelineResult::UNEXPECTED_FAILURE);
        }
        
        if(!buildsManager.SaveBuildsMappings())
        {
            return DS_ERROR_MSG_EC( "Failed to save build mappings", 
                                    (int)PipelineResult::UNEXPECTED_FAILURE);
        }
        
        return {};
    }

    inline DS::Result<void> 
    InitializeBuildDirectory(   const ghc::filesystem::path& configDir,
                                const ghc::filesystem::path& absoluteScriptPath,
                                bool useLocalBuildDir,
                                BuildsManager& outBuildsManager,
                                ghc::filesystem::path& outBuildDir,
                                IncludeManager& outIncludeManager)
    {
        ssLOG_FUNC_INFO();
        
        //Create build directory
        ghc::filesystem::path buildDirPath = useLocalBuildDir ?
                                            ghc::filesystem::current_path() / ".runcpp2" :
                                            configDir;
        
        //Create a class that manages build folder
        outBuildsManager = BuildsManager(buildDirPath);
        
        if(!outBuildsManager.Initialize())
        {
            return DS_ERROR_MSG_EC( "Failed to initialize builds manager", 
                                    (int)PipelineResult::INVALID_BUILD_DIR);
        }
        
        bool createdBuildDir = false;
        bool writeMapping = false;
        if(!outBuildsManager.HasBuildMapping(absoluteScriptPath))
            writeMapping = true;
        
        if(outBuildsManager.GetBuildMapping(absoluteScriptPath, outBuildDir))
        {
            if(writeMapping && !outBuildsManager.SaveBuildsMappings())
                ssLOG_FATAL("Failed to save builds mappings");
            else
                createdBuildDir = true;
        }

        if(!createdBuildDir)
        {
            return DS_ERROR_MSG_EC( "Failed to create local build directory for: " + 
                                    DS_STR(absoluteScriptPath), 
                                    (int)PipelineResult::INVALID_BUILD_DIR);
        }

        outIncludeManager = IncludeManager();
        if(!outIncludeManager.Initialize(outBuildDir))
        {
            return DS_ERROR_MSG_EC( "Failed to initialize include manager", 
                                    (int)PipelineResult::INVALID_BUILD_DIR);
        }

        return {};
    }

    inline DS::Result<void> ResolveScriptImports(   Data::ScriptInfo& scriptInfo,
                                                    const ghc::filesystem::path& scriptPath,
                                                    const ghc::filesystem::path& buildDir)
    {
        ssLOG_FUNC_INFO();

        //Resolve all the script info imports first before evaluating it
        ResolveImports(scriptInfo, scriptPath, buildDir)
            .DS_TRY_ACT(DS_TMP_ERROR.ErrorCode = (int)PipelineResult::UNEXPECTED_FAILURE;
                        DS_APPEND_TRACE(DS_TMP_ERROR);
                        return DS::Error(DS_TMP_ERROR));
        
        return {};
    }
    
    inline DS::Result<void> CheckScriptInfoChanges( const ghc::filesystem::path& buildDir,
                                                    const Data::ScriptInfo& scriptInfo,
                                                    const Data::Profile& profile,
                                                    const ghc::filesystem::path& absoluteScriptPath,
                                                    const Data::ScriptInfo* lastScriptInfo,
                                                    const int maxThreads,
                                                    bool& outAllRecompileNeeded,
                                                    bool& outRelinkNeeded,
                                                    std::vector<std::string>& outChangedDependencies)
    {
        ssLOG_FUNC_INFO();

        const ghc::filesystem::path scriptDirectory = absoluteScriptPath.parent_path();
        ghc::filesystem::path lastScriptInfoFilePath = buildDir / "LastScriptInfo.yaml";
        Data::ScriptInfo lastScriptInfoFromDisk;

        std::error_code e;

        //Run Setup commands if we don't have previous build
        if(!ghc::filesystem::exists(lastScriptInfoFilePath, e))
        {
            const Data::ProfilesCommands* setupCommands = 
                runcpp2::GetValueFromPlatformMap(scriptInfo.Setup);
                
            if(setupCommands != nullptr)
            {
                RunProfileCommands(setupCommands, profile, scriptDirectory.string(), "Setup").DS_TRY();
            }
        }
        
        //Compare script info in memory or from disk
        const Data::ScriptInfo* lastInfo = lastScriptInfo;
        if(lastInfo == nullptr && ghc::filesystem::exists(lastScriptInfoFilePath, e))
        {
            ssLOG_DEBUG("Last script info file exists: " << lastScriptInfoFilePath);
            std::ifstream lastScriptInfoFile;
            lastScriptInfoFile.open(lastScriptInfoFilePath);
            std::stringstream lastScriptInfoBuffer;
            lastScriptInfoBuffer << lastScriptInfoFile.rdbuf();
            
            int currentThreadTargetLevel = ssLOG_GET_CURRENT_THREAD_TARGET_LEVEL();
            ssLOG_SET_CURRENT_THREAD_TARGET_LEVEL(ssLOG_LEVEL_NONE);
            
            do
            {
                if(!ParseScriptInfo(lastScriptInfoBuffer.str(), lastScriptInfoFromDisk).HasValue())
                    break;
                
                //Resolve imports for last script info
                ResolveScriptImports(lastScriptInfoFromDisk, absoluteScriptPath, buildDir).DS_TRY();
                lastInfo = &lastScriptInfoFromDisk;
            }
            while(false);
            
            ssLOG_SET_CURRENT_THREAD_TARGET_LEVEL(currentThreadTargetLevel);
            
            if(lastInfo != nullptr)
                ssLOG_INFO("Last script info parsed");
            else
                ssLOG_INFO("Failed to parse last script info");
        }
        
        //Check if the cached script info has changed
        if(lastInfo != nullptr)
        {
            //Relink if there are any changes to the link flags
            {
                const Data::ProfilesFlagsOverride* lastLinkFlags = 
                    runcpp2::GetValueFromPlatformMap(lastInfo->OverrideLinkFlags);
                const Data::ProfilesFlagsOverride* currentLinkFlags = 
                    runcpp2::GetValueFromPlatformMap(scriptInfo.OverrideLinkFlags);
                
                outRelinkNeeded =   (lastLinkFlags == nullptr) != (currentLinkFlags == nullptr) ||
                                    (
                                        lastLinkFlags != nullptr && 
                                        currentLinkFlags != nullptr && 
                                        !lastLinkFlags->Equals(*currentLinkFlags)
                                    );
            }
            
            outAllRecompileNeeded = scriptInfo.IsAllCompiledCacheInvalidated(*lastInfo);

            //Check dependencies
            for(int i = 0; i < scriptInfo.Dependencies.size(); ++i)
            {
                if( lastInfo->Dependencies.size() <= i || 
                    !scriptInfo.Dependencies[i].Equals(lastInfo->Dependencies[i]))
                {
                    outChangedDependencies.push_back(scriptInfo.Dependencies[i].Name);
                }
            }
            
            if(outAllRecompileNeeded || outRelinkNeeded)
            {
                ssLOG_INFO( "Last script info is out of date, " << 
                            (outAllRecompileNeeded ? "recompiling..." : "relinking..."));
            }
        }
        else
            outAllRecompileNeeded = true;
        
        ssLOG_DEBUG("recompileNeeded: " << outAllRecompileNeeded << 
                    ", changedDependencies.size(): " << outChangedDependencies.size() << 
                    ", relinkNeeded: " << outRelinkNeeded);
        
        //Write to file if there's any changes to the current script info
        if( !lastInfo || 
            outAllRecompileNeeded || 
            !outChangedDependencies.empty() || 
            outRelinkNeeded ||
            !scriptInfo.Equals(*lastInfo))
        {
            std::ofstream writeOutputFile(lastScriptInfoFilePath);
            if(!writeOutputFile)
            {
                return DS_ERROR_MSG_EC( "Failed to open file: " + DS_STR(lastScriptInfoFilePath), 
                                        (int)PipelineResult::INVALID_BUILD_DIR);
            }

            writeOutputFile << scriptInfo.ToString("");
            ssLOG_DEBUG("Wrote current script info to " << lastScriptInfoFilePath.string());
        }

        return {};
    }
    
    
    inline DS::Result<void>
    ProcessDependencies(Data::ScriptInfo& scriptInfo,
                        const Data::Profile& profile,
                        const ghc::filesystem::path& absoluteScriptPath,
                        const ghc::filesystem::path& buildDir,
                        const std::unordered_map<CmdOptions, std::string>& currentOptions,
                        const std::vector<std::string>& changedDependencies,
                        const int maxThreads,
                        std::vector<Data::DependencyInfo*>& outAvailableDependencies,
                        std::vector<std::string>& outGatheredBinariesPaths)
    {
        ssLOG_FUNC_INFO();
        
        for(int i = 0; i < scriptInfo.Dependencies.size(); ++i)
        {
            if(IsDependencyAvailableForThisPlatform(scriptInfo.Dependencies.at(i)))
                outAvailableDependencies.push_back(&scriptInfo.Dependencies.at(i));
        }
        
        std::vector<std::string> dependenciesLocalCopiesPaths;
        std::vector<std::string> dependenciesSourcePaths;
        GetDependenciesPaths(   outAvailableDependencies,
                                dependenciesLocalCopiesPaths,
                                dependenciesSourcePaths,
                                absoluteScriptPath,
                                buildDir)
            .DS_TRY_ACT(DS_TMP_ERROR.ErrorCode = (int)PipelineResult::DEPENDENCIES_FAILED;
                        DS_APPEND_TRACE(DS_TMP_ERROR);
                        return DS::Error(DS_TMP_ERROR));
        
        if(currentOptions.count(CmdOptions::RESET_DEPENDENCIES) > 0 || !changedDependencies.empty())
        {
            if(currentOptions.count(CmdOptions::BUILD_SOURCE_ONLY) > 0)
            {
                std::string errorMsg = 
                    "Dependencies settings have changed or being reset explicitly.\n"
                    "Cannot just build source files only without building dependencies";
                return DS_ERROR_MSG_EC(errorMsg, (int)PipelineResult::INVALID_OPTION);
            }
            
            std::string depsToReset = "all";
            if(!changedDependencies.empty())
            {
                depsToReset = changedDependencies[0];
                for(int i = 1; i < changedDependencies.size(); ++i)
                    depsToReset += "," + changedDependencies[i];
            }
            
            CleanupDependencies(profile,
                                scriptInfo,
                                outAvailableDependencies,
                                dependenciesLocalCopiesPaths,
                                currentOptions.count(CmdOptions::RESET_DEPENDENCIES) > 0 ?
                                currentOptions.at(CmdOptions::RESET_DEPENDENCIES) : 
                                depsToReset)
                .DS_TRY_ACT(DS_TMP_ERROR.ErrorCode = (int)PipelineResult::DEPENDENCIES_FAILED;
                            DS_APPEND_TRACE(DS_TMP_ERROR);
                            return DS::Error(DS_TMP_ERROR));
        }
        
        if(currentOptions.count(CmdOptions::RESET_DEPENDENCIES) > 0)
            return {};
        
        SetupDependenciesIfNeeded(  profile, 
                                    buildDir,
                                    scriptInfo, 
                                    outAvailableDependencies,
                                    dependenciesLocalCopiesPaths,
                                    dependenciesSourcePaths,
                                    maxThreads)
            .DS_TRY_ACT(DS_TMP_ERROR.ErrorCode = (int)PipelineResult::DEPENDENCIES_FAILED;
                        DS_APPEND_TRACE(DS_TMP_ERROR);
                        return DS::Error(DS_TMP_ERROR));

        //Sync local dependencies before building
        SyncLocalDependencies(  outAvailableDependencies,
                                dependenciesSourcePaths,
                                dependenciesLocalCopiesPaths)
            .DS_TRY_ACT(DS_TMP_ERROR.ErrorCode = (int)PipelineResult::DEPENDENCIES_FAILED;
                        DS_APPEND_TRACE(DS_TMP_ERROR);
                        return DS::Error(DS_TMP_ERROR));

        if(currentOptions.count(CmdOptions::BUILD_SOURCE_ONLY) == 0)
        {
            BuildDependencies(  profile,
                                scriptInfo,
                                outAvailableDependencies, 
                                dependenciesLocalCopiesPaths,
                                maxThreads)
                .DS_TRY_ACT
                (
                    DS_TMP_ERROR.Message += 
                        "\nFailed to build script dependencies. Maybe try resetting dependencies "
                        "with \"-rd all\" and run again?";
                    DS_TMP_ERROR.ErrorCode = (int)PipelineResult::DEPENDENCIES_FAILED;
                    DS_APPEND_TRACE(DS_TMP_ERROR);
                    return DS::Error(DS_TMP_ERROR);
                );
        }

        GatherDependenciesBinaries( outAvailableDependencies,
                                    dependenciesLocalCopiesPaths,
                                    profile,
                                    outGatheredBinariesPaths)
            .DS_TRY_ACT(DS_TMP_ERROR.ErrorCode = (int)PipelineResult::DEPENDENCIES_FAILED;
                        DS_APPEND_TRACE(DS_TMP_ERROR);
                        return DS::Error(DS_TMP_ERROR));

        return {};
    }

    inline void SeparateDependencyFiles(const Data::FilesTypesInfo& filesTypes,
                                        const std::vector<std::string>& gatheredBinariesPaths,
                                        std::vector<std::string>& outLinkFilesPaths,
                                        std::vector<std::string>& outFilesToCopyPaths)
    {
        ssLOG_FUNC_INFO();
        INTERNAL_RUNCPP2_SAFE_START();
        
        std::unordered_set<std::string> linkExtensions;

        //Populate the set of link extensions
        if(runcpp2::HasValueFromPlatformMap(filesTypes.StaticLinkFile.Extension))
        {
            linkExtensions.insert(*runcpp2::GetValueFromPlatformMap(filesTypes  .StaticLinkFile
                                                                                .Extension));
        }
        if(runcpp2::HasValueFromPlatformMap(filesTypes.SharedLinkFile.Extension))
        {
            linkExtensions.insert(*runcpp2::GetValueFromPlatformMap(filesTypes  .SharedLinkFile
                                                                                .Extension));
        }
        if(runcpp2::HasValueFromPlatformMap(filesTypes.ObjectLinkFile.Extension))
        {
            linkExtensions.insert(*runcpp2::GetValueFromPlatformMap(filesTypes  .ObjectLinkFile
                                                                                .Extension));
        }

        //Separate the gathered files from dependencies into files to link and files to copy
        for(int i = 0; i < gatheredBinariesPaths.size(); ++i)
        {
            ghc::filesystem::path filePath(gatheredBinariesPaths.at(i));
            std::string extension = runcpp2::GetFileExtensionWithoutVersion(filePath);

            //Check if the file is a link file based on its extension
            if(linkExtensions.find(extension) != linkExtensions.end())
            {
                outLinkFilesPaths.push_back(gatheredBinariesPaths.at(i));
                
                //Special case when SharedLinkFile and SharedLibraryFile share the same extension
                if( runcpp2::HasValueFromPlatformMap(filesTypes.SharedLibraryFile.Extension) && 
                    *runcpp2::GetValueFromPlatformMap(filesTypes.SharedLibraryFile
                                                                .Extension) == extension)
                {
                    outFilesToCopyPaths.push_back(gatheredBinariesPaths.at(i));
                }
            }
            else
                outFilesToCopyPaths.push_back(gatheredBinariesPaths.at(i));
        }

        ssLOG_INFO("Files to link:");
        for(int i = 0; i < outLinkFilesPaths.size(); ++i)
            ssLOG_INFO("  " << outLinkFilesPaths[i]);

        ssLOG_INFO("Files to copy:");
        for(int i = 0; i < outFilesToCopyPaths.size(); ++i)
            ssLOG_INFO("  " << outFilesToCopyPaths[i]);
        
        INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(void());
    }

    inline DS::Result<void> HandlePreBuild( const Data::ScriptInfo& scriptInfo,
                                            const Data::Profile& profile,
                                            const ghc::filesystem::path& buildDir)
    {
        ssLOG_FUNC_INFO();
        
        const Data::ProfilesCommands* preBuildCommands = 
            runcpp2::GetValueFromPlatformMap(scriptInfo.PreBuild);
        
        RunProfileCommands(preBuildCommands, profile, buildDir.string(), "PreBuild").DS_TRY();
        return {};
    }

    inline DS::Result<void> HandlePostBuild(const Data::ScriptInfo& scriptInfo,
                                            const Data::Profile& profile,
                                            const ghc::filesystem::path& buildDir)
    {
        ssLOG_FUNC_INFO();
        
        const Data::ProfilesCommands* postBuildCommands = 
            GetValueFromPlatformMap(scriptInfo.PostBuild);
        
        RunProfileCommands(postBuildCommands, profile, buildDir.string(), "PostBuild").DS_TRY();
        return {};
    }

    inline DS::Result<void>
    RunCompiledOutput(  const ghc::filesystem::path& target,
                        const ghc::filesystem::path& absoluteScriptPath,
                        const Data::ScriptInfo& scriptInfo,
                        const std::vector<std::string>& runArgs,
                        const std::unordered_map<CmdOptions, std::string>& currentOptions,
                        int& returnStatus)
    {
        ssLOG_FUNC_INFO();

        //Skip running if not executable
        if( scriptInfo.CurrentBuildType != Data::BuildType::INTERNAL_EXECUTABLE_EXECUTABLE &&
            scriptInfo.CurrentBuildType != Data::BuildType::INTERNAL_EXECUTABLE_SHARED)
        {
            ssLOG_INFO("Skipping run - output is not executable");
            return {};
        }
        
        std::error_code e;
        if(target.empty() || !ghc::filesystem::exists(target, e))
        {
            return DS_ERROR_MSG_EC( "Failed to find the compiled file to run", 
                                    (int)PipelineResult::COMPILE_LINK_FAILED);
        }
        
        //Prepare run arguments
        std::vector<std::string> finalRunArgs;
        finalRunArgs.push_back(target.string());
        if(scriptInfo.PassScriptPath)
            finalRunArgs.push_back(absoluteScriptPath);
        
        //Add user provided arguments
        for(size_t i = 0; i < runArgs.size(); ++i)
            finalRunArgs.push_back(runArgs[i]);
        
        if(scriptInfo.CurrentBuildType == Data::BuildType::INTERNAL_EXECUTABLE_EXECUTABLE)
        {
            //Running the script with modified args
            if(!RunCompiledScript(target, absoluteScriptPath, finalRunArgs, returnStatus))
                return DS_ERROR_MSG_EC("Failed to run script", (int)PipelineResult::RUN_SCRIPT_FAILED);
        }
        else
        {
            //Load the shared library and run it with modified args
            if(!RunCompiledSharedLib(absoluteScriptPath, target, finalRunArgs, returnStatus))
                return DS_ERROR_MSG_EC("Failed to run script", (int)PipelineResult::RUN_SCRIPT_FAILED);
        }
        
        return {};
    }

    inline DS::Result<void> 
    GetBuiltTargetPaths(const ghc::filesystem::path& buildDir,
                        const std::string& scriptName,
                        const Data::Profile& profile,
                        const std::unordered_map<   CmdOptions, 
                                                    std::string>& currentOptions,
                        const Data::ScriptInfo& scriptInfo,
                        std::vector<ghc::filesystem::path>& outTargets,
                        ghc::filesystem::path* outRunnableTarget)
    {
        ssLOG_FUNC_INFO();
        
        std::error_code _;
        outTargets.clear();

        //Validate executable option against build type
        if( currentOptions.count(CmdOptions::EXECUTABLE) > 0 && 
            scriptInfo.CurrentBuildType != Data::BuildType::INTERNAL_EXECUTABLE_SHARED &&
            scriptInfo.CurrentBuildType != Data::BuildType::INTERNAL_EXECUTABLE_EXECUTABLE)
        {
            std::string errMsg = 
                DS_STR("Cannot run as executable - script is configured for ") +
                Data::BuildTypeToString(scriptInfo.CurrentBuildType) +
                " output. Please remove --executable flag or change build type to Executable";
            
            return DS_ERROR_MSG_EC(errMsg, (int)PipelineResult::INVALID_OPTION);
        }

        //Get all target paths
        std::vector<bool> isRunnable;
        if(!Data::BuildTypeHelper::GetPossibleOutputPaths(  buildDir, 
                                                            scriptName,
                                                            profile,
                                                            scriptInfo.CurrentBuildType,
                                                            outTargets,
                                                            isRunnable))
        {
            return DS_ERROR_MSG_EC( "Extension or prefix not found in compiler profile for build type: " +
                                    runcpp2::Data::BuildTypeToString(scriptInfo.CurrentBuildType), 
                                    (int)PipelineResult::INVALID_SCRIPT_INFO);
        }
        
        //Verify all targets exist
        for(const ghc::filesystem::path& target : outTargets)
        {
            if(!ghc::filesystem::exists(target, _))
            {
                ssLOG_WARNING("Failed to find the compiled file: " << target.string());
                continue;
                //return PipelineResult::COMPILE_LINK_FAILED;
            }
        }

        //If requested, find the runnable target
        if(outRunnableTarget != nullptr)
        {
            for(size_t i = 0; i < outTargets.size(); ++i)
            {
                if(isRunnable.at(i))
                {
                    *outRunnableTarget = outTargets.at(i);
                    break;
                }
            }
        }

        return {};
    }

    inline DS::Result<void> GatherSourceFiles(  const ghc::filesystem::path& absoluteScriptPath, 
                                                const Data::ScriptInfo& scriptInfo,
                                                const Data::Profile& currentProfile,
                                                std::vector<ghc::filesystem::path>& outSourcePaths)
    {
        ssLOG_FUNC_INFO();
        
        if(!absoluteScriptPath.is_absolute())
            return DS_ERROR_MSG("Script path is not absolute: " + DS_STR(absoluteScriptPath));
        
        outSourcePaths.clear();
        if(absoluteScriptPath.extension() != ".yaml" && absoluteScriptPath.extension() != ".yml")
        {
            if(currentProfile.FileExtensions.count(absoluteScriptPath.extension().string()) == 0)
                return DS_ERROR_MSG("Input file cannot be used for profile " + currentProfile.Name);
            else
                outSourcePaths.push_back(absoluteScriptPath);
        }
        
        do
        {
            const Data::ProfilesProcessPaths* compileFiles = 
                GetValueFromPlatformMap(scriptInfo.OtherFilesToBeCompiled);
            
            if(compileFiles == nullptr)
            {
                ssLOG_INFO("No other files to be compiled files current platform");
                
                if(!scriptInfo.OtherFilesToBeCompiled.empty())
                {
                    ssLOG_WARNING(  "Other source files are present, "
                                    "but none are included for current configuration. "
                                    "Is this intended?");
                }
                break;
            }
            
            const std::vector<ghc::filesystem::path>* profileCompileFiles = 
                GetValueFromProfileMap(currentProfile, compileFiles->Paths);
                
            if(!profileCompileFiles)
            {
                ssLOG_INFO("No other files to be compiled for current profile");
                break;
            }

            //TODO: Allow filepaths to contain wildcards as follows
            //* as directory or filename wildcard
            //i.e. "./*/test/*.cpp" will match "./moduleA/test/a.cpp" and "./moduleB/test/b.cpp"
            
            //** as recursive directory wildcard
            //i.e. "./**/*.cpp" will match any .cpp files
            //i.e. "./**/test.cpp" will match any files named "test.cpp"
            
            //For the time being, each entry will represent a path
            {
                const ghc::filesystem::path scriptDirectory = 
                    ghc::filesystem::path(absoluteScriptPath).parent_path();
                
                for(int i = 0; i < profileCompileFiles->size(); ++i)
                {
                    ghc::filesystem::path currentPath = profileCompileFiles->at(i);
                    if(currentPath.is_relative())
                        currentPath = scriptDirectory / currentPath;
                    
                    if(currentPath.is_relative())
                    {
                        std::string errMsg =    DS_STR("Failed to process compile path: ") + 
                                                DS_STR(profileCompileFiles->at(i)) +
                                                "\nTry to append path to script directory but failed" +
                                                "\nFinal appended path: " + 
                                                DS_STR(currentPath);
                        return DS_ERROR_MSG(errMsg);
                    }
                    
                    std::error_code e;
                    if(ghc::filesystem::is_directory(currentPath, e))
                    {
                        return DS_ERROR_MSG("Directory is found instead of file: " + 
                                            DS_STR(profileCompileFiles->at(i)));
                    }
                    
                    if(!ghc::filesystem::exists(currentPath, e))
                    {
                        return DS_ERROR_MSG("File doesn't exist: " + 
                                            DS_STR(profileCompileFiles->at(i)));
                    }
                    
                    outSourcePaths.push_back(currentPath);
                }
            }
        }
        while(0);
        
        if(outSourcePaths.empty())
            return DS_ERROR_MSG("No source files found for compiling.");
        
        return {};
    }

    inline DS::Result<void> 
    GatherIncludePaths( const ghc::filesystem::path& scriptDirectory, 
                        const Data::ScriptInfo& scriptInfo,
                        const Data::Profile& currentProfile,
                        const std::vector<Data::DependencyInfo*>& dependencies,
                        std::vector<ghc::filesystem::path>& outIncludePaths)
    {
        ssLOG_FUNC_INFO();
        
        outIncludePaths.clear();
        
        if(!scriptDirectory.is_absolute())
            return DS_ERROR_MSG("Script directory is not absolute: " + DS_STR(scriptDirectory));

        //Get include paths from script
        const Data::ProfilesProcessPaths* includePaths = 
            GetValueFromPlatformMap(scriptInfo.IncludePaths);
        
        outIncludePaths.push_back(scriptDirectory);
        
        if(includePaths != nullptr)
        {
            const std::vector<ghc::filesystem::path>* profileIncludePaths = 
                GetValueFromProfileMap(currentProfile, includePaths->Paths);
                
            if(profileIncludePaths != nullptr)
            {
                for(const auto& currentPath : *profileIncludePaths)
                {
                    ghc::filesystem::path resolvedPath = currentPath;
                    if(currentPath.is_relative())
                        resolvedPath = scriptDirectory / currentPath;
                    
                    if(resolvedPath.is_relative())
                    {
                        std::string errMsg = 
                            DS_STR("Failed to process include path: ") + DS_STR(currentPath) +
                            "\nTry to append path to script directory but failed" +
                            "\nFinal appended path: " + DS_STR(resolvedPath);
                        return DS_ERROR_MSG(errMsg);
                    }
                    
                    std::error_code e;
                    if(!ghc::filesystem::exists(resolvedPath, e))
                    {
                        std::string errMsg = 
                            DS_STR("Include path doesn't exist: ") + DS_STR(currentPath) +
                            "\nFullpath: " + DS_STR(resolvedPath);
                        return DS_ERROR_MSG(errMsg);
                    }
                    
                    if(!ghc::filesystem::is_directory(resolvedPath, e))
                    {
                        std::string errMsg = 
                            DS_STR("Include path is not a directory: ") + DS_STR(currentPath) +
                            "\nFullpath: " + DS_STR(resolvedPath);
                        return DS_ERROR_MSG(errMsg);
                    }
                    
                    outIncludePaths.push_back(resolvedPath);
                }
            }
        }
        
        //Get include paths from dependencies
        for(const Data::DependencyInfo* dependency : dependencies)
        {
            for(const std::string& includePath : dependency->AbsoluteIncludePaths)
                outIncludePaths.push_back(ghc::filesystem::path(includePath));
        }
        
        return {};
    }

    using SourceIncludeMap = std::unordered_map<std::string, std::vector<ghc::filesystem::path>>;
    inline DS::Result<void> 
    GatherFilesIncludes(const std::vector<ghc::filesystem::path>& sourceFiles,
                        const std::vector<bool>& sourceHasCache,
                        const std::vector<ghc::filesystem::path>& includePaths,
                        SourceIncludeMap& outSourceIncludes)
    {
        ssLOG_FUNC_INFO();
        
        if(sourceFiles.size() != sourceHasCache.size())
            return DS_ERROR_MSG("Size of sourceFiles and sourceHasCache not matching");
        
        outSourceIncludes.clear();
        
        for(int i = 0; i < sourceFiles.size(); ++i)
        {
            if(sourceHasCache.at(i))
                continue;
            
            const ghc::filesystem::path& source = sourceFiles.at(i);
            
            std::unordered_set<std::string> visitedFiles;
            ssLOG_INFO("Gathering includes for " << source.string());
            
            std::vector<ghc::filesystem::path>& currentIncludes = outSourceIncludes[source.string()];
            std::queue<ghc::filesystem::path> filesToProcess;
            filesToProcess.push(source);
            
            while(!filesToProcess.empty())
            {
                ghc::filesystem::path currentFile = filesToProcess.front();
                filesToProcess.pop();
                
                if(visitedFiles.count(currentFile.string()) > 0)
                    continue;

                visitedFiles.insert(currentFile.string());
                
                std::ifstream fileStream(currentFile);
                if(!fileStream.is_open())
                    return DS_ERROR_MSG("Failed to open file: " + DS_STR(currentFile));
                
                std::string line;
                while(std::getline(fileStream, line))
                {
                    std::string includePath;
                    if(!ParseIncludes(line, includePath))
                        continue;
                        
                    ghc::filesystem::path resolvedInclude;
                    bool found = false;
                    
                    //For quoted includes, first check relative to source file
                    if(line.find('\"') != std::string::npos)
                    {
                        resolvedInclude = currentFile.parent_path() / includePath;
                        std::error_code ec;
                        if(ghc::filesystem::exists(resolvedInclude, ec))
                            found = true;
                    }
                    
                    //Search in include paths if not found
                    if(!found)
                    {
                        for(const ghc::filesystem::path& searchPath : includePaths)
                        {
                            resolvedInclude = searchPath / includePath;
                            std::error_code ec;
                            if(ghc::filesystem::exists(resolvedInclude, ec))
                            {
                                found = true;
                                break;
                            }
                        }
                    }
                    
                    if(found)
                    {
                        ssLOG_DEBUG("Found include file: " << resolvedInclude.string());
                        currentIncludes.push_back(resolvedInclude);
                        filesToProcess.push(resolvedInclude);
                    }
                }
            }
        }
        
        return {};
    }
}

#endif
