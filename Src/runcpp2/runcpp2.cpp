#include "runcpp2/runcpp2.hpp"
#include "runcpp2/PipelineSteps.hpp"

#include "runcpp2/ProfileHelper.hpp"
#include "runcpp2/CompilingLinking.hpp"
#include "runcpp2/ConfigParsing.hpp"
#include "runcpp2/PlatformUtil.hpp"
#include "ssLogger/ssLog.hpp"
#include "ghc/filesystem.hpp"

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
    bool HasCompiledCache(  const ghc::filesystem::path& scriptPath,
                            const std::vector<ghc::filesystem::path>& sourceFiles,
                            const ghc::filesystem::path& buildDir,
                            const runcpp2::Data::Profile& currentProfile,
                            const std::vector<ghc::filesystem::path>& includePaths,
                            runcpp2::IncludeManager& includeManager,
                            std::vector<bool>& outHasCache,
                            std::vector<ghc::filesystem::path>& outCachedObjectsFiles,
                            ghc::filesystem::file_time_type& outFinalObjectWriteTime)
    {
        ssLOG_FUNC_INFO();
        
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
        
        std::vector<ghc::filesystem::path> sourcesNeedGathering;
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
            
            ghc::filesystem::path currentObjectFilePath =   buildDir / 
                                                            relativeSourcePath.parent_path() / 
                                                            relativeSourcePath.stem();
            currentObjectFilePath.concat(objectExt);
            
            ssLOG_DEBUG("Trying to use cache: " << sourceFiles.at(i).string());
            
            if(ghc::filesystem::exists(currentObjectFilePath, e))
            {
                ghc::filesystem::file_time_type lastObjectWriteTime = 
                    ghc::filesystem::last_write_time(currentObjectFilePath, e);
                
                //Check source file timestamp
                ghc::filesystem::file_time_type lastSourceWriteTime = 
                    ghc::filesystem::last_write_time(sourceFiles.at(i), e);
                    
                bool useCache = lastObjectWriteTime > lastSourceWriteTime;
                
                //Update include files timestamps
                {
                    std::vector<ghc::filesystem::path> cachedIncludes;
                    ghc::filesystem::file_time_type recordTime;
                    bool needsGather = true;
                    
                    if(includeManager.ReadIncludeRecord(sourceFiles.at(i), 
                                                        cachedIncludes, 
                                                        recordTime))
                    {
                        if(!includeManager.NeedsUpdate( sourceFiles.at(i), 
                                                        cachedIncludes, 
                                                        recordTime))
                        {
                            needsGather = false;
                        }
                        else if(useCache)
                            useCache = false;
                    }
                    
                    if(needsGather)
                    {
                        ssLOG_DEBUG("Needs to update include record for " << 
                                    sourceFiles.at(i).string());
                        sourcesNeedGathering.push_back(sourceFiles.at(i));
                        useCache = false;
                    }
                }
            
                if(useCache)
                {
                    ssLOG_INFO("Using cache for " << sourceFiles.at(i).string());
                    outHasCache.at(i) = true;
                    outCachedObjectsFiles.push_back(currentObjectFilePath);
                }
                else
                    ssLOG_INFO("Cache invalidated for " << sourceFiles.at(i).string());
                
                if(lastObjectWriteTime > outFinalObjectWriteTime)
                    outFinalObjectWriteTime = lastObjectWriteTime;
            }
        }
        
        //Update the include records
        runcpp2::SourceIncludeMap sourcesIncludes;
        if(!runcpp2::GatherFilesIncludes(sourcesNeedGathering, includePaths, sourcesIncludes))
            return false;
        
        for(auto it = sourcesIncludes.cbegin(); it != sourcesIncludes.cend(); ++it)
        {
            ssLOG_DEBUG("Updating include record for " << it->first);
            if(!includeManager.WriteIncludeRecord(  ghc::filesystem::path(it->first),
                                                    it->second))
            {
                ssLOG_ERROR("Failed to write include record for " << it->first);
                return false;
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
                            const ghc::filesystem::file_time_type& finalObjectWriteTime,
                            bool& outOutputCache)
    {
        for(int i = 0; i < sourceHasCache.size(); ++i)
        {
            if(!sourceHasCache.at(i))
            {
                outOutputCache = false;
                return true;
            }
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
                outOutputCache = false;
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
                
                if(lastExecutableWriteTime >= currentFinalObjectWriteTime)
                {
                    ssLOG_INFO("Using output cache");
                    outOutputCache = true;
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
            outOutputCache = false;
            return true;
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
                outOutputCache = false;
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
                
                if(lastSharedLibWriteTime >= currentFinalObjectWriteTime)
                {
                    ssLOG_INFO("Using output cache");
                    outOutputCache = true;
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
            outOutputCache = false;
            return true;
        }
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

runcpp2::PipelineResult 
runcpp2::CheckSourcesNeedUpdate(    const std::string& scriptPath,
                                    const std::vector<Data::Profile>& profiles,
                                    const std::string& configPreferredProfile,
                                    const Data::ScriptInfo& scriptInfo,
                                    const std::unordered_map<CmdOptions, std::string>& currentOptions,
                                    bool& outNeedsUpdate)
{
    INTERNAL_RUNCPP2_SAFE_START();
    ssLOG_FUNC_INFO();

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

    //Initialize BuildsManager and IncludeManager
    ghc::filesystem::path configDir = GetConfigFilePath();
    ghc::filesystem::path buildDir;
    BuildsManager buildsManager("/tmp");
    IncludeManager includeManager;
    
    const bool useLocalBuildDir = currentOptions.count(CmdOptions::LOCAL) > 0;
    result = InitializeBuildDirectory(  configDir,
                                        absoluteScriptPath,
                                        useLocalBuildDir,
                                        buildsManager,
                                        buildDir,
                                        includeManager);
        
    if(result != PipelineResult::SUCCESS)
        return result;

    //Get profile and gather source files
    const int profileIndex = GetPreferredProfileIndex(scriptPath, scriptInfo, profiles, configPreferredProfile);
    const Data::Profile& currentProfile = profiles.at(profileIndex);
    
    std::vector<ghc::filesystem::path> sourceFiles;
    if(!GatherSourceFiles(absoluteScriptPath, scriptInfo, currentProfile, sourceFiles))
        return PipelineResult::UNEXPECTED_FAILURE;

    for(int i = 0; i < sourceFiles.size(); ++i)
        ssLOG_DEBUG("sourceFiles.at(i).string(): " << sourceFiles.at(i).string());

    //Get include paths
    std::vector<ghc::filesystem::path> includePaths;
    if(!GatherIncludePaths(scriptDirectory, scriptInfo, currentProfile, {}, includePaths))
    {
        ssLOG_ERROR("Failed to gather include paths");
        return PipelineResult::UNEXPECTED_FAILURE;
    }
    
    for(int i = 0; i < includePaths.size(); ++i)
        ssLOG_DEBUG("includePaths.at(i).string(): " << includePaths.at(i).string());

    for(int i = 0; i < sourceFiles.size(); ++i)
    {
        std::vector<ghc::filesystem::path> cachedIncludes;
        ghc::filesystem::file_time_type recordTime;
        
        if(includeManager.ReadIncludeRecord(sourceFiles.at(i), 
                                            cachedIncludes, 
                                            recordTime))
        {
            if(includeManager.NeedsUpdate(  sourceFiles.at(i), 
                                            cachedIncludes, 
                                            recordTime))
            {
                outNeedsUpdate = true;
                return PipelineResult::SUCCESS;
            }
        }
    }
    
    outNeedsUpdate = false;
    return PipelineResult::SUCCESS;

    INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(PipelineResult::UNEXPECTED_FAILURE);
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
    ssLOG_FUNC_INFO();

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
        IncludeManager includeManager;
        PipelineResult result = 
            InitializeBuildDirectory(   configDir,
                                        absoluteScriptPath,
                                        currentOptions.count(CmdOptions::LOCAL) > 0,
                                        buildsManager,
                                        buildDir,
                                        includeManager);
            
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
        
        //Resolve imports
        result = ResolveScriptImports(scriptInfo, absoluteScriptPath, buildDir);
        if(result != PipelineResult::SUCCESS)
            return result;
        
        //Check if script info has changed if provided
        bool recompileNeeded = false;
        bool relinkNeeded = false;
        std::vector<std::string> changedDependencies;
        
        result = CheckScriptInfoChanges(buildDir, 
                                        scriptInfo, 
                                        profiles.at(profileIndex), 
                                        absoluteScriptPath,
                                        lastScriptInfo, 
                                        recompileNeeded, 
                                        relinkNeeded, 
                                        changedDependencies);
        if(result != PipelineResult::SUCCESS)
            return result;
        
        if(!lastScriptInfo || recompileNeeded || !changedDependencies.empty() || relinkNeeded)
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

        //Get all include paths
        std::vector<ghc::filesystem::path> includePaths;
        if(!GatherIncludePaths( scriptDirectory,
                                scriptInfo,
                                profiles.at(profileIndex),
                                availableDependencies,
                                includePaths))
        {
            ssLOG_ERROR("Failed to gather include paths");
            return PipelineResult::INVALID_SCRIPT_INFO;
        }

        //Check if we have already compiled before.
        std::vector<bool> sourceHasCache;
        std::vector<ghc::filesystem::path> cachedObjectsFiles;
        ghc::filesystem::file_time_type finalObjectWriteTime;
        
        if(currentOptions.count(runcpp2::CmdOptions::RESET_CACHE) > 0 || recompileNeeded)
        {
            sourceHasCache = std::vector<bool>(sourceFiles.size(), false);
            
            //Update the include records
            {
                runcpp2::SourceIncludeMap sourcesIncludes;
                if(!runcpp2::GatherFilesIncludes(sourceFiles, includePaths, sourcesIncludes))
                    return PipelineResult::UNEXPECTED_FAILURE;
                
                for(auto it = sourcesIncludes.cbegin(); it != sourcesIncludes.cend(); ++it)
                {
                    ssLOG_DEBUG("Updating include record for " << it->first);
                    if(!includeManager.WriteIncludeRecord(  ghc::filesystem::path(it->first),
                                                            it->second))
                    {
                        ssLOG_ERROR("Failed to write include record for " << it->first);
                        return PipelineResult::UNEXPECTED_FAILURE;
                    }
                }
            }
        }
        else if(!HasCompiledCache(  absoluteScriptPath,
                                    sourceFiles, 
                                    buildDir, 
                                    profiles.at(profileIndex),
                                    includePaths,
                                    includeManager,
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
        bool outputCache = false;
        if(!HasOutputCache( sourceHasCache, 
                            buildDir, 
                            profiles.at(profileIndex), 
                            currentOptions.count(CmdOptions::EXECUTABLE) > 0,
                            scriptName,
                            exeExt,
                            linkFilesPaths,
                            finalObjectWriteTime,
                            outputCache))
        {
            ssLOG_WARNING("Error detected when trying to use output cache. A cleanup is recommended");
            //return PipelineResult::UNEXPECTED_FAILURE;
        }
        
        if(!outputCache || relinkNeeded)
        {
            for(int i = 0; i < cachedObjectsFiles.size(); ++i)
                linkFilesPaths.push_back(cachedObjectsFiles.at(i));
            
            if(currentOptions.count(CmdOptions::WATCH) > 0)
            {
                if(!CompileScriptOnly(  buildDir,
                                        absoluteScriptPath,
                                        sourceFiles,
                                        sourceHasCache,
                                        includePaths, 
                                        scriptInfo,
                                        availableDependencies,
                                        profiles.at(profileIndex),
                                        currentOptions.count(CmdOptions::EXECUTABLE) > 0))
                {
                    return PipelineResult::COMPILE_LINK_FAILED;
                }
                
                return PipelineResult::SUCCESS;
            }
            else if(!CompileAndLinkScript(  buildDir,
                                            absoluteScriptPath,
                                            ghc::filesystem::path(absoluteScriptPath).stem(), 
                                            sourceFiles,
                                            sourceHasCache,
                                            includePaths,  
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
        ghc::filesystem::path target;
        result = GetTargetPath( buildDir, 
                                scriptName, 
                                profiles.at(profileIndex), 
                                currentOptions, 
                                target);
            
        if(result != PipelineResult::SUCCESS)
            return result;
        
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
            result = HandlePostBuild(scriptInfo, profiles.at(profileIndex), buildDir.string());
            if(result != PipelineResult::SUCCESS)
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
