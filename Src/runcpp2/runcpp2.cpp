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

extern "C" const uint8_t DefaultScriptInfo[];
extern "C" const size_t DefaultScriptInfo_size;

//Use for SetDllDirectory
#if defined(_WIN32)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
#endif

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
    
    bool GatherSourceFiles( ghc::filesystem::path absoluteScriptPath, 
                            const runcpp2::Data::ScriptInfo& scriptInfo,
                            const runcpp2::Data::Profile& currentProfile,
                            std::vector<ghc::filesystem::path>& outSourcePaths)
    {
        if(!currentProfile.FileExtensions.count(absoluteScriptPath.extension()))
        {
            ssLOG_ERROR("File extension of script doesn't match profile");
            return false;
        }
        
        outSourcePaths.clear();
        outSourcePaths.push_back(absoluteScriptPath);
        
        const runcpp2::Data::ProfilesCompilesFiles* compileFiles = 
            runcpp2::GetValueFromPlatformMap(scriptInfo.OtherFilesToBeCompiled);
        
        if(compileFiles == nullptr)
        {
            ssLOG_INFO("No other files to be compiled files current platform");
            
            if(!scriptInfo.OtherFilesToBeCompiled.empty())
            {
                ssLOG_WARNING(  "Other source files are present, "
                                "but none are included for current configuration. Is this intended?");
            }
            return true;
        }
        
        const std::vector<ghc::filesystem::path>* profileCompileFiles = 
            runcpp2::GetValueFromProfileMap(currentProfile, compileFiles->CompilesFiles);
            
        if(!profileCompileFiles)
        {
            ssLOG_INFO("No other files to be compiled for current profile");
            return true;
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
                    ssLOG_ERROR("Failed to process compile path: " << profileCompileFiles->at(i));
                    ssLOG_ERROR("Try to append path to script directory but failed");
                    ssLOG_ERROR("Final appended path: " << currentPath);
                    return false;
                }
                
                std::error_code e;
                if(ghc::filesystem::is_directory(currentPath, e))
                {
                    ssLOG_ERROR("Directory is found instead of file: " << 
                                profileCompileFiles->at(i));
                    return false;
                }
                
                if(!ghc::filesystem::exists(currentPath, e))
                {
                    ssLOG_ERROR("File doesn't exist: " << profileCompileFiles->at(i));
                    return false;
                }
                
                outSourcePaths.push_back(currentPath);
            }
        }
        
        return true;
    }
    
    bool HasCompiledCache(  const ghc::filesystem::path& scriptPath,
                            const std::vector<ghc::filesystem::path>& sourceFiles,
                            const ghc::filesystem::path& buildDir,
                            const runcpp2::Data::Profile& currentProfile,
                            std::vector<bool>& outHasCache,
                            std::vector<ghc::filesystem::path>& outCachedObjectsFiles,
                            ghc::filesystem::file_time_type& outFinalObjectWriteTime)
    {
        ssLOG_FUNC_DEBUG();
        
        outHasCache.clear();
        outHasCache = std::vector<bool>(sourceFiles.size(), false);
        
        //TODO: Check compile flags
        
        const std::string* rawObjectExt = 
            runcpp2::GetValueFromPlatformMap(currentProfile.FilesTypes.ObjectLinkFile.Extension);
        
        if(rawObjectExt == nullptr)
            return false;
        
        const std::string& objectExt = *rawObjectExt;
        
        outFinalObjectWriteTime = ghc::filesystem::file_time_type();
        
        std::error_code e;
        for(int i = 0; i < sourceFiles.size(); ++i)
        {
            ghc::filesystem::path relativeSourcePath = 
                ghc::filesystem::relative(sourceFiles.at(i), scriptPath.parent_path(), e);
            
            if(e)
            {
                ssLOG_ERROR("Failed to get relative path for " << sourceFiles.at(i).string());
                ssLOG_ERROR("Failed with error: " << e.message());
                return false;
            }
            
            ghc::filesystem::path currentObjectFilePath = buildDir / 
                                                          relativeSourcePath.parent_path() / 
                                                          relativeSourcePath.stem();
            currentObjectFilePath.concat(objectExt);
            
            ssLOG_DEBUG("Trying to use cache: " << sourceFiles.at(i).string());
            std::error_code e;
            if(ghc::filesystem::exists(currentObjectFilePath, e))
            {
                ghc::filesystem::file_time_type lastSourceWriteTime = 
                    ghc::filesystem::last_write_time(sourceFiles.at(i), e);
                
                ghc::filesystem::file_time_type lastObjectWriteTime = 
                    ghc::filesystem::last_write_time(currentObjectFilePath, e);
            
                if(lastObjectWriteTime > lastSourceWriteTime)
                {
                    ssLOG_DEBUG("Using cache for " << sourceFiles.at(i).string());
                    outHasCache.at(i) = true;
                    outCachedObjectsFiles.push_back(currentObjectFilePath);
                }
                
                if(lastObjectWriteTime > outFinalObjectWriteTime)
                    outFinalObjectWriteTime = lastObjectWriteTime;
            }
        }
        
        return true;
    }
    
    bool HasOutputCache(    const std::vector<bool>& sourceHasCache,
                            const ghc::filesystem::path& buildDir,
                            const runcpp2::Data::Profile& currentProfile,
                            bool buildExecutable,
                            const std::string& scriptName,
                            const std::string& exeExt,
                            const std::vector<std::string>& copiedBinariesPaths,
                            const ghc::filesystem::file_time_type& finalObjectWriteTime)
    {
        for(int i = 0; i < sourceHasCache.size(); ++i)
        {
            if(!sourceHasCache.at(i))
                return false;
        }
        
        ghc::filesystem::file_time_type currentFinalObjectWriteTime = finalObjectWriteTime;
        std::error_code e;
        
        for(int i = 0; i < copiedBinariesPaths.size(); ++i)
        {
            if(ghc::filesystem::exists(copiedBinariesPaths.at(i), e))
            {
                ghc::filesystem::file_time_type lastObjectWriteTime = 
                    ghc::filesystem::last_write_time(copiedBinariesPaths.at(i), e);
            
                if(lastObjectWriteTime > currentFinalObjectWriteTime)
                    currentFinalObjectWriteTime = lastObjectWriteTime;
            }
            else
            {
                ssLOG_ERROR("Somehow copied binary path " << copiedBinariesPaths.at(i) << 
                            " doesn't exist");
                return false;
            }
        }
        
        //Check if output is cached
        if(buildExecutable)
        {
            ghc::filesystem::path exeToCopy = buildDir / scriptName;
            exeToCopy.concat(exeExt);
            
            ssLOG_INFO("Trying to use output cache: " << exeToCopy.string());
            
            //If the executable already exists, check if it's newer than the script
            if( ghc::filesystem::exists(exeToCopy, e) && 
                ghc::filesystem::file_size(exeToCopy, e) > 0)
            {
                ghc::filesystem::file_time_type lastExecutableWriteTime = 
                    ghc::filesystem::last_write_time(exeToCopy, e);
                
                if(lastExecutableWriteTime > currentFinalObjectWriteTime)
                {
                    ssLOG_INFO("Using output cache");
                    return true;
                }
                else
                {
                    ssLOG_INFO("Link binaries have more recent write time");
                    ssLOG_DEBUG("lastExecutableWriteTime: " << 
                                lastExecutableWriteTime.time_since_epoch().count());
                    ssLOG_DEBUG("currentFinalObjectWriteTime: " << 
                                currentFinalObjectWriteTime.time_since_epoch().count());
                }
            }
            else
                ssLOG_INFO(exeToCopy.string() << " doesn't exist");
            
            ssLOG_INFO("Not using output cache");
            return false;
        }
        else
        {
            //Check if there's any existing shared library build that is newer than the script
            const std::string* targetSharedLibExt = 
                runcpp2::GetValueFromPlatformMap(currentProfile .FilesTypes
                                                                .SharedLibraryFile
                                                                .Extension);
            
            const std::string* targetSharedLibPrefix =
                runcpp2::GetValueFromPlatformMap(currentProfile .FilesTypes
                                                                .SharedLibraryFile
                                                                .Prefix);
            
            if(targetSharedLibExt == nullptr || targetSharedLibPrefix == nullptr)
            {
                ssLOG_ERROR("Shared library extension or prefix not found in compiler profile");
                return false;
            }
            
            ghc::filesystem::path sharedLibBuild = buildDir / *targetSharedLibPrefix;
            sharedLibBuild.concat(scriptName).concat(*targetSharedLibExt);
            
            ssLOG_INFO("Trying to use output cache: " << sharedLibBuild.string());
            
            if( ghc::filesystem::exists(sharedLibBuild, e) && 
                ghc::filesystem::file_size(sharedLibBuild, e) > 0)
            {
                ghc::filesystem::file_time_type lastSharedLibWriteTime = 
                    ghc::filesystem::last_write_time(sharedLibBuild, e);
                
                if(lastSharedLibWriteTime > currentFinalObjectWriteTime)
                {
                    ssLOG_INFO("Using output cache");
                    return true;
                }
                else
                {
                    ssLOG_INFO("Link binaries have more recent write time");
                    ssLOG_DEBUG("lastSharedLibWriteTime: " << 
                                lastSharedLibWriteTime.time_since_epoch().count());
                    ssLOG_DEBUG("currentFinalObjectWriteTime: " << 
                                currentFinalObjectWriteTime.time_since_epoch().count());
                }
            }
            else
                ssLOG_INFO(sharedLibBuild.string() << " doesn't exist");
            
            ssLOG_INFO("Not using output cache");
            return false;
        }
    }

    bool CopyFiles( const ghc::filesystem::path& destDir,
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
                outCopiedPaths.push_back(runcpp2::ProcessPath(destPath));
            }
            else
            {
                ssLOG_ERROR("File to copy not found: " << srcPath);
                return false;
            }
        }
        
        return true;
    }

    bool RunPostBuildCommands(  const runcpp2::Data::ScriptInfo& scriptInfo,
                                const runcpp2::Data::Profile& profile,
                                const std::string& outputDir)
    {
        const runcpp2::Data::ProfilesCommands* postBuildCommands = 
            runcpp2::GetValueFromPlatformMap(scriptInfo.PostBuild);
        
        if(postBuildCommands != nullptr)
        {
            const std::vector<std::string>* commands = 
                runcpp2::GetValueFromProfileMap(profile, postBuildCommands->CommandSteps);
            if(commands != nullptr)
            {
                for(const std::string& cmd : *commands)
                {
                    std::string output;
                    int returnCode = 0;
                    if(!runcpp2::RunCommandAndGetOutput(cmd, output, returnCode, outputDir))
                    {
                        ssLOG_ERROR("PostBuild command failed: " << cmd << 
                                    " with return code " << returnCode);
                        ssLOG_ERROR("Output: \n" << output);
                        return false;
                    }
                    
                    ssLOG_INFO("PostBuild command ran: \n" << cmd);
                    ssLOG_INFO("PostBuild command output: \n" << output);
                }
            }
        }
        return true;
    }
}

void runcpp2::GetDefaultScriptInfo(std::string& scriptInfo)
{
    scriptInfo = std::string(   reinterpret_cast<const char*>(DefaultScriptInfo), 
                                DefaultScriptInfo_size);
}

//NOTE: Mainly used for test to reduce spamminig
void runcpp2::SetLogLevel(const std::string& logLevel)
{
    if(logLevel == "Debug")
        ssLOG_SET_CURRENT_THREAD_TARGET_LEVEL(ssLOG_LEVEL_DEBUG);
    else if(logLevel == "Warning")
        ssLOG_SET_CURRENT_THREAD_TARGET_LEVEL(ssLOG_LEVEL_WARNING);
    else if(logLevel == "Error")
        ssLOG_SET_CURRENT_THREAD_TARGET_LEVEL(ssLOG_LEVEL_ERROR);
    else
        ssLOG_ERROR("Invalid log level: " << logLevel);
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
            const std::vector<std::string>* commands = 
                runcpp2::GetValueFromProfileMap(profile, setupCommands->CommandSteps);
            if(commands != nullptr)
            {
                for(const std::string& cmd : *commands)
                {
                    std::string output;
                    int returnCode = 0;
                    if(!runcpp2::RunCommandAndGetOutput(cmd, output, returnCode, scriptDirectory))
                    {
                        ssLOG_ERROR("Setup command failed: " << cmd << 
                                    " with return code " << returnCode);
                        ssLOG_ERROR("Output: \n" << output);
                        return PipelineResult::UNEXPECTED_FAILURE;
                    }
                    
                    ssLOG_INFO("Setup command ran: \n" << cmd);
                    ssLOG_INFO("Setup command output: \n" << output);
                }
            }
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

runcpp2::PipelineResult runcpp2::HandlePreBuild(    const Data::ScriptInfo& scriptInfo,
                                                    const Data::Profile& profile,
                                                    const ghc::filesystem::path& buildDir)
{
    const Data::ProfilesCommands* preBuildCommands = 
        runcpp2::GetValueFromPlatformMap(scriptInfo.PreBuild);
    
    if(preBuildCommands != nullptr)
    {
        const std::vector<std::string>* commands = 
            runcpp2::GetValueFromProfileMap(profile, preBuildCommands->CommandSteps);
        if(commands != nullptr)
        {
            for(const std::string& cmd : *commands)
            {
                std::string output;
                int returnCode = 0;
                if(!runcpp2::RunCommandAndGetOutput(cmd, 
                                                  output, 
                                                  returnCode, 
                                                  buildDir.string()))
                {
                    ssLOG_ERROR("PreBuild command failed: " << cmd << 
                               " with return code " << returnCode);
                    ssLOG_ERROR("Output: \n" << output);
                    return PipelineResult::UNEXPECTED_FAILURE;
                }
                
                ssLOG_INFO("PreBuild command ran: \n" << cmd);
                ssLOG_INFO("PreBuild command output: \n" << output);
            }
        }
    }
    
    return PipelineResult::SUCCESS;
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
    if(!RunPostBuildCommands(scriptInfo, profile, buildOutputDir))
        return PipelineResult::UNEXPECTED_FAILURE;
    
    //Don't output anything here if we are just watching
    if(currentOptions.count(CmdOptions::WATCH) > 0)
        return PipelineResult::SUCCESS;
    
    ssLOG_BASE("Build completed. Files copied to " << buildOutputDir);
    return PipelineResult::SUCCESS;
}

runcpp2::PipelineResult 
runcpp2::StartPipeline( const std::string& scriptPath, 
                        const std::vector<Data::Profile>& profiles,
                        const std::string& configPreferredProfile,
                        const std::unordered_map<CmdOptions, std::string> currentOptions,
                        const std::vector<std::string>& runArgs,
                        const Data::ScriptInfo* lastScriptInfo,
                        Data::ScriptInfo& outScriptInfo,
                        const std::string& buildOutputDir,
                        int& returnStatus)
{
    INTERNAL_RUNCPP2_SAFE_START();
    ssLOG_FUNC_DEBUG();

    //Validate inputs and get paths
    ghc::filesystem::path absoluteScriptPath;
    ghc::filesystem::path scriptDirectory;
    std::string scriptName;
    
    PipelineResult result = ValidateInputs( scriptPath, 
                                            profiles, 
                                            absoluteScriptPath,
                                            scriptDirectory,
                                            scriptName);
    if(result != PipelineResult::SUCCESS)
        return result;

    //Rest of the original function remains the same...
    ghc::filesystem::path configDir = GetConfigFilePath();
    ghc::filesystem::path buildDir;

    //Parse script info
    Data::ScriptInfo scriptInfo;
    result = ParseAndValidateScriptInfo(absoluteScriptPath,
                                        scriptDirectory,
                                        scriptName,
                                        lastScriptInfo,
                                        scriptInfo);
    
    if(result != PipelineResult::SUCCESS)
        return result;
    
    //Parse and get the config directory
    {
        std::error_code e;
        if(ghc::filesystem::is_directory(configDir, e))
        {
            ssLOG_FATAL("Unexpected path for config file: " << configDir.string());
            return PipelineResult::INVALID_CONFIG_PATH;
        }
        
        configDir = configDir.parent_path();
        if(!ghc::filesystem::is_directory(configDir, e))
        {
            ssLOG_FATAL("Unexpected path for config directory: " << configDir.string());
            return PipelineResult::INVALID_CONFIG_PATH;
        }
    }
    
    int profileIndex = GetPreferredProfileIndex(absoluteScriptPath, 
                                                scriptInfo, 
                                                profiles, 
                                                configPreferredProfile);

    if(profileIndex == -1)
    {
        ssLOG_ERROR("Failed to find a profile to run");
        return PipelineResult::NO_AVAILABLE_PROFILE;
    }

    //Parsing the script, setting up dependencies, compiling and linking
    std::vector<std::string> filesToCopyPaths;
    {
        BuildsManager buildsManager("/tmp");
        PipelineResult result = 
            InitializeBuildDirectory(   configDir,
                                        absoluteScriptPath,
                                        currentOptions.count(CmdOptions::LOCAL) > 0,
                                        buildsManager,
                                        buildDir);
            
        if (result != PipelineResult::SUCCESS)
            return result;

        //Handle cleanup command if present
        if(currentOptions.count(CmdOptions::CLEANUP) > 0)
        {
            return HandleCleanup(   scriptInfo, 
                                    profiles.at(profileIndex),
                                    scriptDirectory,
                                    buildDir,
                                    absoluteScriptPath,
                                    buildsManager);
        }
        
        //Check if script info has changed if provided
        bool recompileNeeded = false;
        bool relinkNeeded = false;
        std::vector<std::string> changedDependencies;
        
        result = CheckScriptInfoChanges(buildDir, 
                                        scriptInfo, 
                                        profiles.at(profileIndex), 
                                        scriptDirectory, 
                                        lastScriptInfo, 
                                        recompileNeeded, 
                                        relinkNeeded, 
                                        changedDependencies);
        if(result != PipelineResult::SUCCESS)
            return result;
        
        if(recompileNeeded || !changedDependencies.empty() || relinkNeeded)
            outScriptInfo = scriptInfo;
        
        std::vector<std::string> gatheredBinariesPaths;
        
        //Process Dependencies
        std::vector<Data::DependencyInfo*> availableDependencies;
        result = ProcessDependencies(   scriptInfo,
                                        profiles.at(profileIndex),
                                        absoluteScriptPath,
                                        buildDir,
                                        currentOptions,
                                        changedDependencies,
                                        availableDependencies,
                                        gatheredBinariesPaths);
            
        if(result != PipelineResult::SUCCESS)
            return result;
        
        if(currentOptions.count(CmdOptions::RESET_DEPENDENCIES) > 0)
            return PipelineResult::SUCCESS;

        //Get all the files we are trying to compile
        std::vector<ghc::filesystem::path> sourceFiles;
        if(!GatherSourceFiles(  absoluteScriptPath, 
                                scriptInfo, 
                                profiles.at(profileIndex), 
                                sourceFiles))
        {
            return PipelineResult::INVALID_SCRIPT_INFO;
        }
        
        //Check if we have already compiled before.
        std::vector<bool> sourceHasCache;
        std::vector<ghc::filesystem::path> cachedObjectsFiles;
        ghc::filesystem::file_time_type finalObjectWriteTime;
        
        if(currentOptions.count(runcpp2::CmdOptions::RESET_CACHE) > 0 || recompileNeeded)
            sourceHasCache = std::vector<bool>(sourceFiles.size(), false);
        else if(!HasCompiledCache(  absoluteScriptPath,
                                    sourceFiles, 
                                    buildDir, 
                                    profiles.at(profileIndex), 
                                    sourceHasCache,
                                    cachedObjectsFiles,
                                    finalObjectWriteTime))
        {
            //TODO: Maybee add a pipeline result for this?
            return PipelineResult::UNEXPECTED_FAILURE;
        }
        
        std::vector<std::string> linkFilesPaths;
        SeparateDependencyFiles(profiles.at(profileIndex).FilesTypes, 
                                gatheredBinariesPaths, 
                                linkFilesPaths, 
                                filesToCopyPaths);
        
        std::error_code e;

        //Update finalObjectWriteTime
        for(int i = 0; i < linkFilesPaths.size(); ++i)
        {
            if(!ghc::filesystem::exists(linkFilesPaths.at(i), e))
            {
                ssLOG_ERROR(linkFilesPaths.at(i) << " reported as cached but doesn't exist");
                return PipelineResult::UNEXPECTED_FAILURE;
            }
            
            ghc::filesystem::file_time_type lastWriteTime = 
                ghc::filesystem::last_write_time(linkFilesPaths.at(i), e);

            if(lastWriteTime > finalObjectWriteTime)
                finalObjectWriteTime = lastWriteTime;
        }
        
        //Run PreBuild commands before compilation
        result = HandlePreBuild(scriptInfo, profiles.at(profileIndex), buildDir);
        if(result != PipelineResult::SUCCESS)
            return result;

        std::string exeExt = "";
        #ifdef _WIN32
            exeExt = ".exe";
        #endif

        //Compiling/Linking
        if(!HasOutputCache( sourceHasCache, 
                            buildDir, 
                            profiles.at(profileIndex), 
                            currentOptions.count(CmdOptions::EXECUTABLE) > 0,
                            scriptName,
                            exeExt,
                            linkFilesPaths,
                            finalObjectWriteTime) || relinkNeeded)
        {
            for(int i = 0; i < cachedObjectsFiles.size(); ++i)
                linkFilesPaths.push_back(cachedObjectsFiles.at(i));
            
            if(currentOptions.count(CmdOptions::WATCH) > 0)
            {
                if(!CompileScriptOnly(  buildDir,
                                        absoluteScriptPath,
                                        sourceFiles,
                                        sourceHasCache,
                                        scriptInfo,
                                        availableDependencies,
                                        profiles.at(profileIndex),
                                        currentOptions.count(CmdOptions::EXECUTABLE) > 0))
                {
                    return PipelineResult::COMPILE_LINK_FAILED;
                }
            }
            else if(!CompileAndLinkScript(  buildDir,
                                            absoluteScriptPath,
                                            ghc::filesystem::path(absoluteScriptPath).stem(), 
                                            sourceFiles,
                                            sourceHasCache,
                                            scriptInfo,
                                            availableDependencies,
                                            profiles.at(profileIndex),
                                            linkFilesPaths,
                                            currentOptions.count(CmdOptions::EXECUTABLE) > 0,
                                            exeExt))
            {
                ssLOG_ERROR("Failed to compile or link script");
                return PipelineResult::COMPILE_LINK_FAILED;
            }
        }
    }

    //Run the compiled file at script directory
    {
        std::string exeExt = "";
        #ifdef _WIN32
            exeExt = ".exe";
        #endif
        
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
                return PipelineResult::INVALID_PROFILE;
            }

            target = (target / *targetSharedLibPrefix)  .concat(scriptName)
                                                        .concat(*targetSharedLibExt);
        }
        
        if(!ghc::filesystem::exists(target, _))
        {
            ssLOG_ERROR("Failed to find the compiled file: " << target.string());
            return PipelineResult::COMPILE_LINK_FAILED;
        }
        
        if(currentOptions.count(CmdOptions::BUILD) == 0)
        {
            //Move copying to before post build
            std::vector<std::string> copiedPaths;
            if(!CopyFiles(buildDir, filesToCopyPaths, copiedPaths))
            {
                ssLOG_ERROR("Failed to copy binaries before running the script");
                return PipelineResult::UNEXPECTED_FAILURE;
            }

            //Run PostBuild commands after successful compilation
            if(!RunPostBuildCommands(scriptInfo, profiles.at(profileIndex), buildDir.string()))
                return PipelineResult::UNEXPECTED_FAILURE;
            
            //Don't run if we are just watching
            if(currentOptions.count(CmdOptions::WATCH) > 0)
                return PipelineResult::SUCCESS;
            
            ssLOG_INFO("Running script...");
            return RunCompiledOutput(   target, 
                                        absoluteScriptPath, 
                                        scriptInfo, 
                                        runArgs, 
                                        currentOptions, 
                                        returnStatus);
        }
        else
        {
            return HandleBuildOutput(   target, 
                                        filesToCopyPaths, 
                                        scriptInfo, 
                                        profiles.at(profileIndex), 
                                        buildOutputDir, 
                                        currentOptions);
        }
    }

    return PipelineResult::UNEXPECTED_FAILURE;
    
    INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(PipelineResult::UNEXPECTED_FAILURE);
}

std::string runcpp2::PipelineResultToString(PipelineResult result)
{
    static_assert(static_cast<int>(PipelineResult::COUNT) == 13, "PipelineResult enum has changed");

    switch(result)
    {
        case PipelineResult::UNEXPECTED_FAILURE:
            return "UNEXPECTED_FAILURE";
        case PipelineResult::SUCCESS:
            return "SUCCESS";
        case PipelineResult::EMPTY_PROFILES:
            return "EMPTY_PROFILES";
        case PipelineResult::INVALID_SCRIPT_PATH:
            return "INVALID_SCRIPT_PATH";
        case PipelineResult::INVALID_CONFIG_PATH:
            return "INVALID_CONFIG_PATH";
        case PipelineResult::INVALID_BUILD_DIR:
            return "INVALID_BUILD_DIR";
        case PipelineResult::INVALID_SCRIPT_INFO:
            return "INVALID_SCRIPT_INFO";
        case PipelineResult::NO_AVAILABLE_PROFILE:
            return "NO_AVAILABLE_PROFILE";
        case PipelineResult::DEPENDENCIES_FAILED:
            return "DEPENDENCIES_FAILED";
        case PipelineResult::COMPILE_LINK_FAILED:
            return "COMPILE_LINK_FAILED";
        case PipelineResult::INVALID_PROFILE:
            return "INVALID_PROFILE";
        case PipelineResult::RUN_SCRIPT_FAILED:
            return "RUN_SCRIPT_FAILED";
        case PipelineResult::INVALID_OPTION:
            return "INVALID_OPTION";
        default:
            return "UNKNOWN_PIPELINE_RESULT";
    }
}

