#include "runcpp2/PipelineSteps.hpp"

#include "runcpp2/ProfileHelper.hpp"
#include "runcpp2/ConfigParsing.hpp"
#include "runcpp2/DependenciesHelper.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "runcpp2/PlatformUtil.hpp"

#include "System2.h"
#include "ssLogger/ssLog.hpp"
#include "ghc/filesystem.hpp"
#include "dylib.hpp"

namespace
{
    bool RunCompiledScript( const ghc::filesystem::path& executable,
                            const std::string& scriptPath,
                            const std::vector<std::string>& runArgs,
                            int& returnStatus)
    {
        INTERNAL_RUNCPP2_SAFE_START();
        ssLOG_FUNC_DEBUG();
        
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
        
        result = System2GetCommandReturnValueSync(&runCommandInfo, &returnStatus);
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
        ssLOG_FUNC_DEBUG();
        
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


bool runcpp2::CopyFiles(const ghc::filesystem::path& destDir,
                        const std::vector<std::string>& filePaths,
                        std::vector<std::string>& outCopiedPaths)
{
    ssLOG_FUNC_DEBUG();
    
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
                ssLOG_ERROR("Failed to copy file from " << srcPath << 
                            " to " << destPath.string());
                ssLOG_ERROR("Error: " << e.message());
                return false;
            }
            
            ssLOG_INFO("Copied from " << srcPath << " to " << destPath.string());
            outCopiedPaths.push_back(ProcessPath(destPath));
        }
        else
        {
            ssLOG_ERROR("File to copy not found: " << srcPath);
            return false;
        }
    }
    
    return true;
}

runcpp2::PipelineResult 
runcpp2::RunProfileCommands(const Data::ProfilesCommands* commands,
                            const Data::Profile& profile,
                            const std::string& workingDir,
                            const std::string& commandType)
{
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
                if(!runcpp2::RunCommandAndGetOutput(cmd, output, returnCode, workingDir))
                {
                    ssLOG_ERROR(commandType << " command failed: " << cmd << 
                                " with return code " << returnCode);
                    ssLOG_ERROR("Output: \n" << output);
                    return PipelineResult::UNEXPECTED_FAILURE;
                }
                
                ssLOG_INFO(commandType << " command ran: \n" << cmd);
                ssLOG_INFO(commandType << " command output: \n" << output);
            }
        }
    }
    return PipelineResult::SUCCESS;
}

runcpp2::PipelineResult runcpp2::ValidateInputs(const std::string& scriptPath, 
                                                const std::vector<Data::Profile>& profiles,
                                                ghc::filesystem::path& outAbsoluteScriptPath,
                                                ghc::filesystem::path& outScriptDirectory,
                                                std::string& outScriptName)
{
    INTERNAL_RUNCPP2_SAFE_START();
    
    if(profiles.empty())
    {
        ssLOG_ERROR("No compiler profiles found");
        return PipelineResult::EMPTY_PROFILES;
    }

    //Check if input file exists
    std::error_code _;
    if(!ghc::filesystem::exists(scriptPath, _))
    {
        ssLOG_ERROR("File does not exist: " << scriptPath);
        return PipelineResult::INVALID_SCRIPT_PATH;
    }
    
    if(ghc::filesystem::is_directory(scriptPath, _))
    {
        ssLOG_ERROR("The input file must not be a directory: " << scriptPath);
        return PipelineResult::INVALID_SCRIPT_PATH;
    }

    outAbsoluteScriptPath = ghc::filesystem::absolute(scriptPath);
    outScriptDirectory = outAbsoluteScriptPath.parent_path();
    outScriptName = outAbsoluteScriptPath.stem().string();

    ssLOG_DEBUG("scriptPath: " << scriptPath);
    ssLOG_DEBUG("absoluteScriptPath: " << outAbsoluteScriptPath.string());
    ssLOG_DEBUG("scriptDirectory: " << outScriptDirectory.string());
    ssLOG_DEBUG("scriptName: " << outScriptName);
    ssLOG_DEBUG("is_directory: " << ghc::filesystem::is_directory(outScriptDirectory));

    return PipelineResult::SUCCESS;
    
    INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(PipelineResult::UNEXPECTED_FAILURE);
}

runcpp2::PipelineResult 
runcpp2::ParseAndValidateScriptInfo(const ghc::filesystem::path& absoluteScriptPath,
                                    const ghc::filesystem::path& scriptDirectory,
                                    const std::string& scriptName,
                                    const Data::ScriptInfo* lastScriptInfo,
                                    Data::ScriptInfo& outScriptInfo)
{
    INTERNAL_RUNCPP2_SAFE_START();

    //Check if there's script info as yaml file instead
    std::error_code e;
    std::string parsableInfo;
    std::ifstream inputFile;
    std::string dedicatedYamlLoc = ProcessPath(scriptDirectory.string() +"/" + scriptName + ".yaml");
    
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
            return PipelineResult::INVALID_SCRIPT_PATH;
        }

        std::stringstream buffer;
        buffer << inputFile.rdbuf();
        std::string source(buffer.str());
        
        if(!GetParsableInfo(source, parsableInfo))
        {
            ssLOG_ERROR("An error has been encountered when parsing info: " << absoluteScriptPath);
            return PipelineResult::INVALID_SCRIPT_INFO;
        }
    }
    
    //Try to parse the runcpp2 info
    if(!ParseScriptInfo(parsableInfo, outScriptInfo))
    {
        ssLOG_ERROR("Failed to parse info");
        ssLOG_ERROR("Content trying to parse: " << "\n" << parsableInfo);
        return PipelineResult::INVALID_SCRIPT_INFO;
    }
    
    if(!parsableInfo.empty())
    {
        ssLOG_INFO("Parsed script info YAML:");
        ssLOG_INFO("\n" << outScriptInfo.ToString(""));
    }

    return PipelineResult::SUCCESS;
    
    INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(PipelineResult::UNEXPECTED_FAILURE);
}

runcpp2::PipelineResult runcpp2::HandleCleanup( const Data::ScriptInfo& scriptInfo,
                                                const Data::Profile& profile,
                                                const ghc::filesystem::path& scriptDirectory,
                                                const ghc::filesystem::path& buildDir,
                                                const ghc::filesystem::path& absoluteScriptPath,
                                                BuildsManager& buildsManager)
{
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
                if(!runcpp2::RunCommandAndGetOutput(cmd, output, returnCode, scriptDirectory))
                {
                    ssLOG_ERROR("Cleanup command failed: " << cmd << 
                                " with return code " << returnCode);
                    ssLOG_ERROR("Output: \n" << output);
                    return PipelineResult::UNEXPECTED_FAILURE;
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
        ssLOG_ERROR("Failed to remove build directory: " << buildDir);
        return PipelineResult::UNEXPECTED_FAILURE;
    }
    
    if(!buildsManager.RemoveBuildMapping(absoluteScriptPath))
    {
        ssLOG_ERROR("Failed to remove build mapping");
        return PipelineResult::UNEXPECTED_FAILURE;
    }
    
    if(!buildsManager.SaveBuildsMappings())
    {
        ssLOG_ERROR("Failed to save build mappings");
        return PipelineResult::UNEXPECTED_FAILURE;
    }
    return PipelineResult::SUCCESS;
}

runcpp2::PipelineResult 
runcpp2::InitializeBuildDirectory(  const ghc::filesystem::path& configDir,
                                    const ghc::filesystem::path& absoluteScriptPath,
                                    bool useLocalBuildDir,
                                    BuildsManager& outBuildsManager,
                                    ghc::filesystem::path& outBuildDir)
{
    //Create build directory
    ghc::filesystem::path buildDirPath = useLocalBuildDir ?
                                        ghc::filesystem::current_path() / ".runcpp2" :
                                        configDir;
    
    //Create a class that manages build folder
    outBuildsManager = BuildsManager(buildDirPath);
    
    if(!outBuildsManager.Initialize())
    {
        ssLOG_FATAL("Failed to initialize builds manager");
        return PipelineResult::INVALID_BUILD_DIR;
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
        ssLOG_FATAL("Failed to create local build directory for: " << absoluteScriptPath);
        return PipelineResult::INVALID_BUILD_DIR;
    }

    return PipelineResult::SUCCESS;
}

runcpp2::PipelineResult 
runcpp2::CheckScriptInfoChanges(const ghc::filesystem::path& buildDir,
                                const Data::ScriptInfo& scriptInfo,
                                const Data::Profile& profile,
                                const ghc::filesystem::path& scriptDirectory,
                                const Data::ScriptInfo* lastScriptInfo,
                                bool& outRecompileNeeded,
                                bool& outRelinkNeeded,
                                std::vector<std::string>& outChangedDependencies)
{
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
            PipelineResult result = RunProfileCommands( setupCommands, 
                                                        profile, 
                                                        scriptDirectory.string(), 
                                                        "Setup");
            if(result != PipelineResult::SUCCESS)
                return result;
        }
    }
    
    //Compare script info in memory or from disk
    const Data::ScriptInfo* lastInfo = lastScriptInfo;
    if(lastScriptInfo == nullptr && ghc::filesystem::exists(lastScriptInfoFilePath, e))
    {
        ssLOG_DEBUG("Last script info file exists: " << lastScriptInfoFilePath);
        std::ifstream lastScriptInfoFile;
        lastScriptInfoFile.open(lastScriptInfoFilePath);
        std::stringstream lastScriptInfoBuffer;
        lastScriptInfoBuffer << lastScriptInfoFile.rdbuf();
        
        if(ParseScriptInfo(lastScriptInfoBuffer.str(), lastScriptInfoFromDisk))
            lastInfo = &lastScriptInfoFromDisk;
    }
    
    if(lastInfo != nullptr)
    {
        //Check link flags
        const Data::ProfilesFlagsOverride* lastLinkFlags = 
            runcpp2::GetValueFromPlatformMap(lastInfo->OverrideLinkFlags);
        const Data::ProfilesFlagsOverride* currentLinkFlags = 
            runcpp2::GetValueFromPlatformMap(scriptInfo.OverrideLinkFlags);
        
        outRelinkNeeded = (lastLinkFlags == nullptr) != (currentLinkFlags == nullptr) ||
                        (
                            lastLinkFlags != nullptr && 
                            currentLinkFlags != nullptr && 
                            !lastLinkFlags->Equals(*currentLinkFlags)
                        );
        
        outRecompileNeeded = outRelinkNeeded;
        
        //Check dependencies
        if(lastInfo->Dependencies.size() == scriptInfo.Dependencies.size())
        {
            for(int i = 0; i < scriptInfo.Dependencies.size(); ++i)
            {
                if(!scriptInfo.Dependencies.at(i).Equals(lastInfo->Dependencies.at(i)))
                {
                    outChangedDependencies.push_back(scriptInfo.Dependencies.at(i).Name);
                    outRecompileNeeded = true;
                }
            }
        }
        else
        {
            outRecompileNeeded = true;
            //All dependencies need to be reset if count changed
            outChangedDependencies.clear();
        }
        
        if(!outRecompileNeeded)
        {
            //Other changes that require recompilation
            const Data::ProfilesFlagsOverride* lastCompileFlags = 
                runcpp2::GetValueFromPlatformMap(lastInfo->OverrideCompileFlags);
            const Data::ProfilesFlagsOverride* currentCompileFlags = 
                runcpp2::GetValueFromPlatformMap(scriptInfo.OverrideCompileFlags);
            
            const Data::ProfilesCompilesFiles* lastCompileFiles = 
                runcpp2::GetValueFromPlatformMap(lastInfo->OtherFilesToBeCompiled);
            const Data::ProfilesCompilesFiles* currentCompileFiles = 
                runcpp2::GetValueFromPlatformMap(scriptInfo.OtherFilesToBeCompiled);
            
            const Data::ProfilesDefines* lastDefines = 
                runcpp2::GetValueFromPlatformMap(lastInfo->Defines);
            const Data::ProfilesDefines* currentDefines = 
                runcpp2::GetValueFromPlatformMap(scriptInfo.Defines);
        
            outRecompileNeeded = 
                (lastCompileFlags == nullptr) != (currentCompileFlags == nullptr) ||
                (
                    lastCompileFlags != nullptr && 
                    currentCompileFlags != nullptr && 
                    !lastCompileFlags->Equals(*currentCompileFlags)
                ) ||
                (lastCompileFiles == nullptr) != (currentCompileFiles == nullptr) ||
                (
                    lastCompileFiles != nullptr && 
                    currentCompileFiles != nullptr && 
                    !lastCompileFiles->Equals(*currentCompileFiles)
                ) ||
                (lastDefines == nullptr) != (currentDefines == nullptr) ||
                (
                    lastDefines != nullptr && 
                    currentDefines != nullptr && 
                    !lastDefines->Equals(*currentDefines)
                );
        }
    }
    else
        outRecompileNeeded = true;
    
    ssLOG_DEBUG("recompileNeeded: " << outRecompileNeeded << 
                ", changedDependencies.size(): " << outChangedDependencies.size() << 
                ", relinkNeeded: " << outRelinkNeeded);
    
    if(outRecompileNeeded || !outChangedDependencies.empty() || outRelinkNeeded)
    {
        std::ofstream writeOutputFile(lastScriptInfoFilePath);
        if(!writeOutputFile)
        {
            ssLOG_ERROR("Failed to open file: " << lastScriptInfoFilePath);
            return PipelineResult::INVALID_BUILD_DIR;
        }

        writeOutputFile << scriptInfo.ToString("");
        ssLOG_DEBUG("Wrote current script info to " << lastScriptInfoFilePath.string());
    }

    return PipelineResult::SUCCESS;
}

runcpp2::PipelineResult 
runcpp2::ProcessDependencies(   Data::ScriptInfo& scriptInfo,
                                const Data::Profile& profile,
                                const ghc::filesystem::path& absoluteScriptPath,
                                const ghc::filesystem::path& buildDir,
                                const std::unordered_map<CmdOptions, std::string>& currentOptions,
                                const std::vector<std::string>& changedDependencies,
                                std::vector<Data::DependencyInfo*>& outAvailableDependencies,
                                std::vector<std::string>& outGatheredBinariesPaths)
{
    for(int i = 0; i < scriptInfo.Dependencies.size(); ++i)
    {
        if(IsDependencyAvailableForThisPlatform(scriptInfo.Dependencies.at(i)))
            outAvailableDependencies.push_back(&scriptInfo.Dependencies.at(i));
    }
    
    std::vector<std::string> dependenciesLocalCopiesPaths;
    std::vector<std::string> dependenciesSourcePaths;
    if(!GetDependenciesPaths(   outAvailableDependencies,
                                dependenciesLocalCopiesPaths,
                                dependenciesSourcePaths,
                                absoluteScriptPath,
                                buildDir))
    {
        ssLOG_ERROR("Failed to get dependencies paths");
        return PipelineResult::DEPENDENCIES_FAILED;
    }
    
    if(currentOptions.count(CmdOptions::RESET_DEPENDENCIES) > 0 || !changedDependencies.empty())
    {
        if(currentOptions.count(CmdOptions::BUILD_SOURCE_ONLY) > 0)
        {
            ssLOG_ERROR("Dependencies settings have changed or being reset explicitly.");
            ssLOG_ERROR("Cannot just build source files only without building dependencies");
            return PipelineResult::INVALID_OPTION;
        }
        
        std::string depsToReset = "all";
        if(!changedDependencies.empty())
        {
            depsToReset = changedDependencies[0];
            for(int i = 1; i < changedDependencies.size(); ++i)
                depsToReset += "," + changedDependencies[i];
        }
        
        if(!CleanupDependencies(profile,
                                scriptInfo,
                                outAvailableDependencies,
                                dependenciesLocalCopiesPaths,
                                currentOptions.count(CmdOptions::RESET_DEPENDENCIES) > 0 ?
                                    currentOptions.at(CmdOptions::RESET_DEPENDENCIES) : 
                                    depsToReset))
        {
            ssLOG_ERROR("Failed to cleanup dependencies");
            return PipelineResult::DEPENDENCIES_FAILED;
        }
    }
    
    if(currentOptions.count(CmdOptions::RESET_DEPENDENCIES) > 0)
    {
        ssLOG_LINE("Removed script dependencies");
        return PipelineResult::SUCCESS;
    }
    
    if(!SetupDependenciesIfNeeded(  profile, 
                                    buildDir,
                                    scriptInfo, 
                                    outAvailableDependencies,
                                    dependenciesLocalCopiesPaths,
                                    dependenciesSourcePaths))
    {
        ssLOG_ERROR("Failed to setup script dependencies");
        return PipelineResult::DEPENDENCIES_FAILED;
    }

    if(currentOptions.count(CmdOptions::BUILD_SOURCE_ONLY) == 0)
    {
        if(!BuildDependencies(  profile,
                                scriptInfo,
                                outAvailableDependencies, 
                                dependenciesLocalCopiesPaths))
        {
            ssLOG_ERROR("Failed to build script dependencies");
            return PipelineResult::DEPENDENCIES_FAILED;
        }
    }

    if(!GatherDependenciesBinaries( outAvailableDependencies,
                                    dependenciesLocalCopiesPaths,
                                    profile,
                                    outGatheredBinariesPaths))
    {
        ssLOG_ERROR("Failed to gather dependencies binaries");
        return PipelineResult::DEPENDENCIES_FAILED;
    }

    return PipelineResult::SUCCESS;
}

void runcpp2::SeparateDependencyFiles(  const Data::FilesTypesInfo& filesTypes,
                                        const std::vector<std::string>& gatheredBinariesPaths,
                                        std::vector<std::string>& outLinkFilesPaths,
                                        std::vector<std::string>& outFilesToCopyPaths)
{
    std::unordered_set<std::string> linkExtensions;

    //Populate the set of link extensions
    if(runcpp2::HasValueFromPlatformMap(filesTypes.StaticLinkFile.Extension))
        linkExtensions.insert(*runcpp2::GetValueFromPlatformMap(filesTypes.StaticLinkFile.Extension));
    if(runcpp2::HasValueFromPlatformMap(filesTypes.SharedLinkFile.Extension))
        linkExtensions.insert(*runcpp2::GetValueFromPlatformMap(filesTypes.SharedLinkFile.Extension));
    if(runcpp2::HasValueFromPlatformMap(filesTypes.ObjectLinkFile.Extension))
        linkExtensions.insert(*runcpp2::GetValueFromPlatformMap(filesTypes.ObjectLinkFile.Extension));

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
}

runcpp2::PipelineResult runcpp2::HandlePreBuild(const Data::ScriptInfo& scriptInfo,
                                                const Data::Profile& profile,
                                                const ghc::filesystem::path& buildDir)
{
    const Data::ProfilesCommands* preBuildCommands = 
        runcpp2::GetValueFromPlatformMap(scriptInfo.PreBuild);
    
    return RunProfileCommands(preBuildCommands, profile, buildDir.string(), "PreBuild");
}

runcpp2::PipelineResult runcpp2::HandlePostBuild(   const Data::ScriptInfo& scriptInfo,
                                                    const Data::Profile& profile,
                                                    const ghc::filesystem::path& buildDir)
{
    const Data::ProfilesCommands* postBuildCommands = 
        GetValueFromPlatformMap(scriptInfo.PostBuild);
    
    return RunProfileCommands(postBuildCommands, profile, buildDir.string(), "PostBuild");
}

runcpp2::PipelineResult 
runcpp2::RunCompiledOutput( const ghc::filesystem::path& target,
                            const ghc::filesystem::path& absoluteScriptPath,
                            const Data::ScriptInfo& scriptInfo,
                            const std::vector<std::string>& runArgs,
                            const std::unordered_map<CmdOptions, std::string>& currentOptions,
                            int& returnStatus)
{
    //Prepare run arguments
    std::vector<std::string> finalRunArgs;
    finalRunArgs.push_back(target.string());
    if(scriptInfo.PassScriptPath)
        finalRunArgs.push_back(absoluteScriptPath);
    
    //Add user provided arguments
    for(size_t i = 0; i < runArgs.size(); ++i)
        finalRunArgs.push_back(runArgs[i]);
    
    if(currentOptions.count(CmdOptions::EXECUTABLE) > 0)
    {
        //Running the script with modified args
        if(!RunCompiledScript(target, absoluteScriptPath, finalRunArgs, returnStatus))
        {
            ssLOG_ERROR("Failed to run script");
            return PipelineResult::RUN_SCRIPT_FAILED;
        }
    }
    else
    {
        //Load the shared library and run it with modified args
        if(!RunCompiledSharedLib(absoluteScriptPath, target, finalRunArgs, returnStatus))
        {
            ssLOG_ERROR("Failed to run script");
            return PipelineResult::RUN_SCRIPT_FAILED;
        }
    }
    
    return PipelineResult::SUCCESS;
}

runcpp2::PipelineResult 
runcpp2::HandleBuildOutput( const ghc::filesystem::path& target,
                            const std::vector<std::string>& filesToCopyPaths,
                            const Data::ScriptInfo& scriptInfo,
                            const Data::Profile& profile,
                            const std::string& buildOutputDir,
                            const std::unordered_map<CmdOptions, std::string>& currentOptions)
{
    //Copy the output file
    std::vector<std::string> filesToCopy = filesToCopyPaths;
    filesToCopy.push_back(target);
    
    std::vector<std::string> copiedPaths;
    if(!CopyFiles(buildOutputDir, filesToCopy, copiedPaths))
    {
        ssLOG_ERROR("Failed to copy binaries before running the script");
        return PipelineResult::UNEXPECTED_FAILURE;
    }
    
    //Run PostBuild commands after successful compilation
    PipelineResult result = HandlePostBuild(scriptInfo, profile, buildOutputDir);
    if(result != PipelineResult::SUCCESS)
        return result;
    
    //Don't output anything here if we are just watching
    if(currentOptions.count(CmdOptions::WATCH) > 0)
        return PipelineResult::SUCCESS;
    
    ssLOG_BASE("Build completed. Files copied to " << buildOutputDir);
    return PipelineResult::SUCCESS;
}

runcpp2::PipelineResult 
runcpp2::GetTargetPath( const ghc::filesystem::path& buildDir,
                        const std::string& scriptName,
                        const Data::Profile& profile,
                        const std::unordered_map<CmdOptions, std::string>& currentOptions,
                        ghc::filesystem::path& outTarget)
{
    std::string exeExt = "";
    #ifdef _WIN32
        exeExt = ".exe";
    #endif
    
    std::error_code _;
    outTarget = buildDir;
    
    const std::string* targetSharedLibExt = 
        runcpp2::GetValueFromPlatformMap(profile.FilesTypes.SharedLibraryFile.Extension);
    
    const std::string* targetSharedLibPrefix =
        runcpp2::GetValueFromPlatformMap(profile.FilesTypes.SharedLibraryFile.Prefix);
    
    if(currentOptions.find(CmdOptions::EXECUTABLE) != currentOptions.end())
        outTarget = (outTarget / scriptName).concat(exeExt);
    else
    {
        if(targetSharedLibExt == nullptr || targetSharedLibPrefix == nullptr)
        {
            ssLOG_ERROR("Shared library extension or prefix not found in compiler profile");
            return PipelineResult::INVALID_PROFILE;
        }

        outTarget = (outTarget / *targetSharedLibPrefix).concat(scriptName)
                                                        .concat(*targetSharedLibExt);
    }
    
    if(!ghc::filesystem::exists(outTarget, _))
    {
        ssLOG_ERROR("Failed to find the compiled file: " << outTarget.string());
        return PipelineResult::COMPILE_LINK_FAILED;
    }

    return PipelineResult::SUCCESS;
}
