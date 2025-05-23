#include "runcpp2/runcpp2.hpp"
#include "runcpp2/PipelineSteps.hpp"

#include "runcpp2/ProfileHelper.hpp"
#include "runcpp2/CompilingLinking.hpp"
#include "runcpp2/ConfigParsing.hpp"
#include "runcpp2/PlatformUtil.hpp"
#include "runcpp2/Data/BuildTypeHelper.hpp"
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
                            runcpp2::IncludeManager& includeManager,
                            std::vector<bool>& outHasCache,
                            std::vector<ghc::filesystem::path>& outCachedObjectsFiles,
                            ghc::filesystem::file_time_type& outFinalObjectWriteTime,
                            ghc::filesystem::file_time_type& outFinalSourceWriteTime,
                            ghc::filesystem::file_time_type& outFinalIncludeWriteTime)
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
            
            //Check source file timestamp
            ghc::filesystem::file_time_type currentSourceWriteTime = 
                ghc::filesystem::last_write_time(sourceFiles.at(i), e);
            if(currentSourceWriteTime > outFinalSourceWriteTime)
                outFinalSourceWriteTime = currentSourceWriteTime;

            //Check include record
            bool outdatedIncludeRecord = false;
            ghc::filesystem::file_time_type currentIncludeWriteTime;
            {
                std::vector<ghc::filesystem::path> cachedIncludes;
                ghc::filesystem::file_time_type recordTime;
                
                if(includeManager.ReadIncludeRecord(sourceFiles.at(i), cachedIncludes, recordTime))
                {
                    if(includeManager.NeedsUpdate(sourceFiles.at(i), cachedIncludes, recordTime))
                        outdatedIncludeRecord = true;
                }
                
                if(outdatedIncludeRecord)
                    ssLOG_DEBUG("Needs to update include record for " << sourceFiles.at(i).string());
                
                for(int j = 0; j < cachedIncludes.size(); ++j)
                {
                    ghc::filesystem::file_time_type includeWriteTime = 
                        ghc::filesystem::last_write_time(cachedIncludes.at(j), e);
                    
                    if(includeWriteTime > currentIncludeWriteTime)
                        currentIncludeWriteTime = includeWriteTime;
                }
                
                if(currentIncludeWriteTime > outFinalIncludeWriteTime)
                    outFinalIncludeWriteTime = currentIncludeWriteTime;
            }
            
            //Check object file timestamp
            if(ghc::filesystem::exists(currentObjectFilePath, e))
            {
                ghc::filesystem::file_time_type currentObjectWriteTime = 
                    ghc::filesystem::last_write_time(currentObjectFilePath, e);
                
                bool useCache = currentObjectWriteTime > currentSourceWriteTime &&
                                currentObjectWriteTime > currentIncludeWriteTime &&
                                !outdatedIncludeRecord;
                
                ssLOG_DEBUG("currentObjectWriteTime: " << 
                            currentObjectWriteTime.time_since_epoch().count());
                ssLOG_DEBUG("currentSourceWriteTime: " << 
                            currentSourceWriteTime.time_since_epoch().count());
                ssLOG_DEBUG("currentIncludeWriteTime: " << 
                            currentIncludeWriteTime.time_since_epoch().count());
                ssLOG_DEBUG("outdatedIncludeRecord: " << outdatedIncludeRecord);
                
                if(useCache)
                {
                    ssLOG_INFO("Using cache for " << sourceFiles.at(i).string());
                    outHasCache.at(i) = true;
                    outCachedObjectsFiles.push_back(currentObjectFilePath);
                }
                else
                    ssLOG_INFO("Cache invalidated for " << sourceFiles.at(i).string());
                
                if(currentObjectWriteTime > outFinalObjectWriteTime)
                    outFinalObjectWriteTime = currentObjectWriteTime;
            }
        }
        
        return true;
    }
    
    bool HasOutputCache(    const std::vector<bool>& sourceHasCache,
                            const ghc::filesystem::path& buildDir,
                            const runcpp2::Data::Profile& currentProfile,
                            const runcpp2::Data::ScriptInfo& scriptInfo,
                            const std::string& scriptName,
                            const std::vector<std::string>& copiedBinariesPaths,
                            const ghc::filesystem::file_time_type& finalBinaryWriteTime,
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
        
        ghc::filesystem::file_time_type currentFinalBinaryWriteTime = finalBinaryWriteTime;
        std::error_code e;
        
        for(int i = 0; i < copiedBinariesPaths.size(); ++i)
        {
            if(ghc::filesystem::exists(copiedBinariesPaths.at(i), e))
            {
                ghc::filesystem::file_time_type lastBinaryWriteTime = 
                    ghc::filesystem::last_write_time(copiedBinariesPaths.at(i), e);
            
                if(lastBinaryWriteTime > currentFinalBinaryWriteTime)
                    currentFinalBinaryWriteTime = lastBinaryWriteTime;
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
        std::vector<ghc::filesystem::path> outputPaths;
        std::vector<bool> runnable;
        
        if(!runcpp2::Data::BuildTypeHelper::GetPossibleOutputPaths( buildDir,
                                                                    scriptName,
                                                                    currentProfile,
                                                                    scriptInfo.CurrentBuildType,
                                                                    outputPaths,
                                                                    runnable))
        {
            return false;
        }
        
        int existCount = 0;
        for(const ghc::filesystem::path& outputPath : outputPaths)
        {
            ssLOG_INFO("Trying to use output cache: " << outputPath.string());
            
            if( ghc::filesystem::exists(outputPath, e) && 
                ghc::filesystem::file_size(outputPath, e) > 0)
            {
                ++existCount;
                ghc::filesystem::file_time_type lastOutputBinary = 
                    ghc::filesystem::last_write_time(outputPath, e);
                
                if(lastOutputBinary >= currentFinalBinaryWriteTime)
                {
                    ssLOG_INFO("Using output cache for " << outputPath.string());
                    continue;
                }
                else
                {
                    ssLOG_INFO("Object files have more recent write time");
                    ssLOG_DEBUG("lastOutputBinary: " << 
                                lastOutputBinary.time_since_epoch().count());
                    ssLOG_DEBUG("currentFinalBinaryWriteTime: " << 
                                currentFinalBinaryWriteTime.time_since_epoch().count());
                    outOutputCache = false;
                    return true;
                }
            }
            else
                ssLOG_INFO(outputPath.string() << " doesn't exist");
        }
        
        //TODO: Parsing ExpectedOutputFiles in the profile to see cache is valid or not
        //NOTE: We don't know which ones are optionals, at least for now. 
        //      If there's nothing, there's no cache for sure. 
        //      If there's something, it's very likely we have it cached. 
        //      Dumb logic, I know, but it works for now.
        if(existCount == 0)
        {
            outOutputCache = false;
            return true;
        }
        
        ssLOG_INFO("Using output cache");
        outOutputCache = true;
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

runcpp2::PipelineResult 
runcpp2::CheckSourcesNeedUpdate(    const std::string& scriptPath,
                                    const std::vector<Data::Profile>& profiles,
                                    const std::string& configPreferredProfile,
                                    const Data::ScriptInfo& scriptInfo,
                                    const std::unordered_map<   CmdOptions, 
                                                                std::string>& currentOptions,
                                    const ghc::filesystem::file_time_type& prevFinalSourceWriteTime,
                                    const ghc::filesystem::file_time_type& prevFinalIncludeWriteTime,
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

    //First check if script info file has changed
    std::error_code e;
    ghc::filesystem::path dedicatedYamlLoc = 
        scriptDirectory / ghc::filesystem::path(scriptName + ".yaml");
    
    ghc::filesystem::file_time_type currentWriteTime;
    if(ghc::filesystem::exists(dedicatedYamlLoc, e))
        currentWriteTime = ghc::filesystem::last_write_time(dedicatedYamlLoc, e);
    else
        currentWriteTime = ghc::filesystem::last_write_time(absoluteScriptPath, e);

    if(e)
    {
        ssLOG_ERROR("Failed to get write time for script info");
        return PipelineResult::UNEXPECTED_FAILURE;
    }

    //If script info file is newer than last check, we need to update
    if(currentWriteTime > scriptInfo.LastWriteTime)
    {
        outNeedsUpdate = true;
        return PipelineResult::SUCCESS;
    }

    //Initialize BuildsManager and IncludeManager
    ghc::filesystem::path configDir = GetConfigFilePath();
    configDir = configDir.parent_path();
    if(!ghc::filesystem::is_directory(configDir, e))
    {
        ssLOG_FATAL("Unexpected path for config directory: " << configDir.string());
        return PipelineResult::INVALID_CONFIG_PATH;
    }
    
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
    const int profileIndex = GetPreferredProfileIndex(  scriptPath, 
                                                        scriptInfo, 
                                                        profiles, 
                                                        configPreferredProfile);
    const Data::Profile& currentProfile = profiles.at(profileIndex);
    
    std::vector<ghc::filesystem::path> sourceFiles;
    if(!GatherSourceFiles(absoluteScriptPath, scriptInfo, currentProfile, sourceFiles))
        return PipelineResult::UNEXPECTED_FAILURE;

    for(int i = 0; i < sourceFiles.size(); ++i)
        ssLOG_DEBUG("sourceFiles.at(i).string(): " << sourceFiles.at(i).string());

    std::vector<bool> sourceHasCache;
    std::vector<ghc::filesystem::path> cachedObjectsFiles;
    ghc::filesystem::file_time_type finalObjectWriteTime;
    ghc::filesystem::file_time_type finalSourceWriteTime;
    ghc::filesystem::file_time_type finalIncludeWriteTime;
    
    if(!HasCompiledCache(   absoluteScriptPath,
                            sourceFiles,
                            buildDir,
                            currentProfile,
                            includeManager,
                            sourceHasCache,
                            cachedObjectsFiles,
                            finalObjectWriteTime,
                            finalSourceWriteTime,
                            finalIncludeWriteTime))
    {
        //TODO: Maybe add a pipeline result for this?
        return PipelineResult::UNEXPECTED_FAILURE;
    }
    
    if( finalSourceWriteTime > prevFinalSourceWriteTime ||
        finalIncludeWriteTime > prevFinalIncludeWriteTime)
    {
        outNeedsUpdate = true;
    }
    else
        outNeedsUpdate = false;
    
    return PipelineResult::SUCCESS;

    INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(PipelineResult::UNEXPECTED_FAILURE);
}

runcpp2::PipelineResult 
runcpp2::StartPipeline( const std::string& scriptPath, 
                        const std::vector<Data::Profile>& profiles,
                        const std::string& configPreferredProfile,
                        const std::unordered_map<CmdOptions, std::string>& currentOptions,
                        const std::vector<std::string>& runArgs,
                        const Data::ScriptInfo* lastScriptInfo,
                        const std::string& buildOutputDir,
                        Data::ScriptInfo& outScriptInfo,
                        ghc::filesystem::file_time_type& outFinalSourceWriteTime,
                        ghc::filesystem::file_time_type& outFinalIncludeWriteTime,
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

    ghc::filesystem::path configDir = GetConfigFilePath();
    ghc::filesystem::path buildDir;

    //Parse script info
    Data::ScriptInfo scriptInfo;
    result = ParseAndValidateScriptInfo(absoluteScriptPath,
                                        scriptDirectory,
                                        scriptName,
                                        currentOptions.count(CmdOptions::EXECUTABLE) > 0,
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
        return PipelineResult::NO_AVAILABLE_PROFILE;

    //Parsing the script, setting up dependencies, compiling and linking
    std::vector<std::string> filesToCopyPaths;
    {
        const int maxThreads = 
            currentOptions.count(CmdOptions::THREADS) ? 
            strtol(currentOptions.at(CmdOptions::THREADS).c_str(), nullptr, 10) : 
            8;
        if(maxThreads == 0)
        {
            ssLOG_ERROR("Invalid number of threads passed in");
            return PipelineResult::INVALID_OPTION;
        }
        
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
        
        //Check if script info has changed if provided and run setup if needed
        bool recompileNeeded = false;
        bool relinkNeeded = false;
        std::vector<std::string> changedDependencies;
        
        result = CheckScriptInfoChanges(buildDir, 
                                        scriptInfo, 
                                        profiles.at(profileIndex), 
                                        absoluteScriptPath,
                                        lastScriptInfo, 
                                        maxThreads,
                                        recompileNeeded, 
                                        relinkNeeded, 
                                        changedDependencies);
        if(result != PipelineResult::SUCCESS)
            return result;
        
        //if(!lastScriptInfo || recompileNeeded || !changedDependencies.empty() || relinkNeeded)
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
                                        maxThreads,
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
        
        if(currentOptions.count(CmdOptions::RESET_CACHE) > 0 || recompileNeeded)
            sourceHasCache = std::vector<bool>(sourceFiles.size(), false);
        else if(!HasCompiledCache(  absoluteScriptPath,
                                    sourceFiles, 
                                    buildDir, 
                                    profiles.at(profileIndex),
                                    includeManager,
                                    sourceHasCache,
                                    cachedObjectsFiles,
                                    finalObjectWriteTime,
                                    outFinalSourceWriteTime,
                                    outFinalIncludeWriteTime))
        {
            //TODO: Maybe add a pipeline result for this?
            return PipelineResult::UNEXPECTED_FAILURE;
        }
        
        runcpp2::SourceIncludeMap sourcesIncludes;
        if(!runcpp2::GatherFilesIncludes(sourceFiles, sourceHasCache, includePaths, sourcesIncludes))
            return PipelineResult::UNEXPECTED_FAILURE;
        
        for(int i = 0; i < sourceFiles.size(); ++i)
        {
            if(!sourceHasCache.at(i))
            {
                ssLOG_DEBUG("Updating include record for " << sourceFiles.at(i).string());
                if(sourcesIncludes.count(sourceFiles.at(i)) == 0)
                {
                    ssLOG_WARNING(  "Includes not gathered for " << 
                                    sourceFiles.at(i).string());
                    continue;
                }
                
                bool writeResult = 
                    includeManager.WriteIncludeRecord
                    (
                        sourceFiles.at(i), 
                        sourcesIncludes.at(sourceFiles.at(i))
                    );
                
                if(!writeResult)
                {
                    ssLOG_ERROR("Failed to write include record for " << 
                                sourceFiles.at(i).string());
                    return PipelineResult::UNEXPECTED_FAILURE;
                }
            }
            else
                ssLOG_DEBUG("Include record for " << sourceFiles.at(i).string() << " is up to date");
        }
        
        std::vector<std::string> linkFilesPaths;
        SeparateDependencyFiles(profiles.at(profileIndex).FilesTypes, 
                                gatheredBinariesPaths, 
                                linkFilesPaths, 
                                filesToCopyPaths);
        
        std::error_code e;

        //Get finalBinaryWriteTime by combining final object and dependencies write times
        ghc::filesystem::file_time_type finalBinaryWriteTime = finalObjectWriteTime;
        for(int i = 0; i < linkFilesPaths.size(); ++i)
        {
            if(!ghc::filesystem::exists(linkFilesPaths.at(i), e))
            {
                ssLOG_ERROR(linkFilesPaths.at(i) << " reported as cached but doesn't exist");
                return PipelineResult::UNEXPECTED_FAILURE;
            }
            
            ghc::filesystem::file_time_type lastWriteTime = 
                ghc::filesystem::last_write_time(linkFilesPaths.at(i), e);

            if(lastWriteTime > finalBinaryWriteTime)
                finalBinaryWriteTime = lastWriteTime;
        }
        
        //Run PreBuild commands before compilation
        result = HandlePreBuild(scriptInfo, profiles.at(profileIndex), buildDir);
        if(result != PipelineResult::SUCCESS)
            return result;

        //Compiling/Linking
        bool outputCache = false;
        if(!HasOutputCache( sourceHasCache, 
                            buildDir, 
                            profiles.at(profileIndex),
                            scriptInfo,
                            scriptName,
                            linkFilesPaths,
                            finalBinaryWriteTime,
                            outputCache))
        {
            ssLOG_WARNING("Error detected when trying to use output cache. A cleanup is recommended");
            //return PipelineResult::UNEXPECTED_FAILURE;
        }
        
        if(!outputCache || relinkNeeded)
        {
            for(int i = 0; i < cachedObjectsFiles.size(); ++i)
                linkFilesPaths.push_back(cachedObjectsFiles.at(i));
            
            //TODO: Compile and link for watch as well. Load library as well
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
                                        maxThreads))
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
                                            maxThreads))
            {
                ssLOG_ERROR("Failed to compile or link script");
                return PipelineResult::COMPILE_LINK_FAILED;
            }
        }
    }

    //Trigger post build and run the script if needed
    {
        std::vector<ghc::filesystem::path> targets;
        ghc::filesystem::path runnableTarget;
        result = GetBuiltTargetPaths(   buildDir, 
                                        scriptName, 
                                        profiles.at(profileIndex), 
                                        currentOptions,
                                        scriptInfo,
                                        targets,
                                        &runnableTarget);
            
        if(result != PipelineResult::SUCCESS)
            return result;

        if(targets.empty())
        {
            ssLOG_WARNING("No target files found");
            return PipelineResult::SUCCESS;
        }
        
        //Copy files to build directory
        std::vector<std::string> copiedPaths;
        if(!buildOutputDir.empty())
        {
            buildDir = buildOutputDir;
            //filesToCopyPaths.push_back(runnableTarget.string());
            for(const ghc::filesystem::path& target : targets)
                filesToCopyPaths.push_back(target.string());
        }

        if(!CopyFiles(buildDir, filesToCopyPaths, copiedPaths))
        {
            ssLOG_ERROR("Failed to copy binaries before running the script");
            return PipelineResult::UNEXPECTED_FAILURE;
        }
        
        //Run PostBuild commands after successful compilation
        result = HandlePostBuild(scriptInfo, profiles.at(profileIndex), buildDir.string());
        if(result != PipelineResult::SUCCESS)
            return PipelineResult::UNEXPECTED_FAILURE;

        //Don't run if we are just watching, reseting source cache or building
        if( currentOptions.count(CmdOptions::WATCH) > 0 || 
            currentOptions.count(CmdOptions::RESET_CACHE) > 0 ||
            currentOptions.count(CmdOptions::BUILD) > 0)
        {
            return PipelineResult::SUCCESS;
        }
        
        //Run otherwise
        ssLOG_INFO("Running script...");
        return RunCompiledOutput(   runnableTarget,
                                    absoluteScriptPath, 
                                    scriptInfo, 
                                    runArgs, 
                                    currentOptions, 
                                    returnStatus);
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

bool runcpp2::DownloadTutorial(char* runcppPath)
{
    std::string dummy;
    int returnCode = 0;
    
    std::string input;
    while(true)
    {
        input.clear();
        ssLOG_BASE( "This will download InteractiveTutorial.cpp from github to current directory. "
                    "Continue? [Y/n]");
        
        if(!std::getline(std::cin, input))
        {
            ssLOG_ERROR("IO Error when trying to get cin");
            return false;
        }
        
        if(!input.empty())
        {
            if(input == "y" || input == "Y")
                break;
            else if(input == "n" || input == "N")
            {
                ssLOG_BASE("Not continuing");
                return 0;
            }
            else
                ssLOG_BASE("Please only answer with y or n");
        }
        else
            break;
    }
    
    std::string targetBranch = RUNCPP2_VERSION;
    size_t dashPos = targetBranch.find("-");
    if(dashPos != std::string::npos)
        targetBranch = targetBranch.substr(0, dashPos);

    #ifdef _WIN32
        if(!RunCommand( "powershell -Command \""
                        "Invoke-WebRequest https://github.com/Neko-Box-Coder/runcpp2/raw/"
                        "refs/tags/" + targetBranch + "/Examples/InteractiveTutorial.cpp "
                        "-OutFile InteractiveTutorial.cpp\"",
                        false,
                        "./",
                        dummy,
                        returnCode))
        {
            return false;
        }    
    #else
        if(!RunCommand( "curl -L -o InteractiveTutorial.cpp "
                        "https://github.com/Neko-Box-Coder/runcpp2/raw/refs/tags/" +
                        targetBranch + "/Examples/InteractiveTutorial.cpp",
                        false,
                        "./",
                        dummy,
                        returnCode))
        {
            return false;
        }
    #endif
    
    ssLOG_INFO("targetBranch: " << targetBranch);
    ssLOG_BASE("Downloaded InteractiveTutorial.cpp from github.");
    ssLOG_BASE("Do `" << runcppPath << " InteractiveTutorial.cpp to start the tutorial.");
    
    return true;
}
