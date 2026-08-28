#ifndef RUNCPP2_RUNCPP2_HPP
#define RUNCPP2_RUNCPP2_HPP

#include "runcpp2/Data/BuildTypeHelper.hpp"
#include "runcpp2/Data/Profile.hpp"
#include "runcpp2/Data/ScriptInfo.hpp"
#include "runcpp2/Data/FileProperties.hpp"
#include "runcpp2/Data/FilesTypesInfo.hpp"
#include "runcpp2/Data/ParseCommon.hpp"

#include "runcpp2/PipelineSteps.hpp"
#include "runcpp2/ProfileHelper.hpp"
#include "runcpp2/CompilingLinking.hpp"
#include "runcpp2/ConfigParsing.hpp"
#include "runcpp2/PlatformUtil.hpp"
#include "runcpp2/BuildsManager.hpp"
#include "runcpp2/IncludeManager.hpp"

#include "ssLogger/ssLog.hpp"
#include "ghc/filesystem.hpp"
#include "DSResult/DSResult.hpp"

#include <string>
#include <vector>
#include <chrono>
#include <stdint.h>
#include <stdlib.h>
#include <system_error>
#include <unordered_map>

//NOTE: #include "runcpp2/LibYamlImpl.cpp" at the end

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

#if !RUNCPP2_BOOTSTRAP
    extern const uint8_t DefaultScriptInfo[];
    extern const size_t DefaultScriptInfo_size;
#endif

namespace runcpp2 { namespace Data { struct DependencyInfo; } }

namespace
{
    DS::Result<bool> HasCompiledCache(  const ghc::filesystem::path& scriptDirectory,
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
        
        DS_ASSERT_FALSE(rawObjectExt == nullptr);
        
        const std::string& objectExt = *rawObjectExt;
        outFinalObjectWriteTime = ghc::filesystem::file_time_type();
        
        std::error_code e;
        for(int i = 0; i < sourceFiles.size(); ++i)
        {
            ghc::filesystem::path relativeSourcePath = 
                ghc::filesystem::relative(sourceFiles.at(i), scriptDirectory, e);
            
            if(e)
            {
                std::string retMsg =    DS_STR("Failed to get relative path for ") + 
                                        sourceFiles.at(i).string() + "\n";
                retMsg += DS_STR("Failed with error: ") + e.message();
                return DS_ERROR_MSG(retMsg);
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
        
        return {};
    }
    
    bool HasOutputCache(    const std::vector<bool>& sourceHasCache,
                            const ghc::filesystem::path& buildDir,
                            const runcpp2::Data::Profile& currentProfile,
                            const runcpp2::Data::ScriptInfo& scriptInfo,
                            const std::string& scriptName,
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
        
        //Check if output is cached
        std::error_code e;
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
        
        ssLOG_INFO("finalBinaryWriteTime: " << runcpp2::SerializeTimePoint(finalBinaryWriteTime));
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
                
                ssLOG_INFO("lastOutputBinary: " << runcpp2::SerializeTimePoint(lastOutputBinary));
                if(lastOutputBinary >= finalBinaryWriteTime)
                {
                    ssLOG_INFO("Using output cache for " << outputPath.string());
                    continue;
                }
                else
                {
                    ssLOG_INFO("Object files have more recent write time");
                    ssLOG_DEBUG("lastOutputBinary: " << 
                                lastOutputBinary.time_since_epoch().count());
                    ssLOG_DEBUG("finalBinaryWriteTime: " << 
                                finalBinaryWriteTime.time_since_epoch().count());
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

namespace runcpp2
{
    #if !RUNCPP2_BOOTSTRAP
        inline void GetDefaultScriptInfo(std::string& scriptInfo)
        {
            scriptInfo = std::string(   reinterpret_cast<const char*>(DefaultScriptInfo), 
                                        DefaultScriptInfo_size);
        }
    #endif

    //NOTE: Mainly used for test to reduce spamminig
    inline void SetLogLevel(const std::string& logLevel)
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

    inline DS::Result<void> GetScriptInfoData(  const ghc::filesystem::path& scriptPath, 
                                                const std::string& rawParameters,
                                                
                                                Data::ScriptInfo& outScriptInfo,
                                                ghc::filesystem::path& outAbsoluteScriptPath,
                                                ghc::filesystem::path& outScriptDirectory,
                                                std::string& outScriptName,
                                                std::unordered_map< std::string, 
                                                                    std::string>& outParameterValues)
    {
        ssLOG_FUNC_INFO();
        
        //Validate inputs and get paths
        {
            //Check if input file exists
            std::error_code _;
            if(!ghc::filesystem::exists(scriptPath, _))
                return DS_ERROR_MSG("File does not exist: " + scriptPath.string());
            
            if(ghc::filesystem::is_directory(scriptPath, _))
                return DS_ERROR_MSG( "The input file must not be a directory: " + scriptPath.string());

            outAbsoluteScriptPath = 
                ghc::filesystem::absolute(ghc::filesystem::canonical(scriptPath, _));
            outScriptDirectory = outAbsoluteScriptPath.parent_path();
            outScriptName = outAbsoluteScriptPath.stem().string();

            ssLOG_DEBUG("scriptPath: " << scriptPath);
            ssLOG_DEBUG("absoluteScriptPath: " << outAbsoluteScriptPath.string());
            ssLOG_DEBUG("scriptDirectory: " << outScriptDirectory.string());
            ssLOG_DEBUG("scriptName: " << outScriptName);
            ssLOG_DEBUG("is_directory: " << ghc::filesystem::is_directory(outScriptDirectory));
        }

        //Create parameters
        std::unordered_map<std::string, std::string> parameterValues;
        CreateParameterValues(rawParameters, parameterValues).DS_TRY();

        //Parse script info
        ParseAndValidateScriptInfo( outAbsoluteScriptPath,
                                    outScriptDirectory,
                                    outScriptName,
                                    parameterValues,
                                    outScriptInfo).DS_TRY();
        
        return {};
    }

    inline DS::Result<ghc::filesystem::path> GetDefaultBuildDir()
    {
        ghc::filesystem::path configDir = GetConfigFilePath().DS_TRY();
        
        //Parse and get the config directory
        {
            std::error_code e;
            if(ghc::filesystem::is_directory(configDir, e))
                return DS_ERROR_MSG("Unexpected path for config file: " + configDir.string());
            
            configDir = configDir.parent_path();
            if(!ghc::filesystem::is_directory(configDir, e))
                return DS_ERROR_MSG("Unexpected path for config directory: " + configDir.string());
        }
        
        return configDir;
    }

    struct CoreParams
    {
        const ghc::filesystem::path& scriptPath;
        const std::vector<Data::Profile>& profiles;
        const std::string rawParameters;
        bool buildLocally;
        const std::string& configPreferredProfile;
    };

    inline DS::Result<void> RunCleanup(CoreParams params)
    {
        ghc::filesystem::path absoluteScriptPath;
        ghc::filesystem::path scriptDirectory;
        std::string scriptName;
        std::unordered_map<std::string, std::string> parameters;
        Data::ScriptInfo scriptInfo;
        GetScriptInfoData(  params.scriptPath, 
                            params.rawParameters, 
                            
                            //Output:
                            scriptInfo,
                            absoluteScriptPath,
                            scriptDirectory,
                            scriptName,
                            parameters).DS_TRY();
        
        if(params.profiles.empty())
            return DS_ERROR_MSG("No compiler profiles found");
        
        int profileIndex =  GetPreferredProfileIndex(   absoluteScriptPath, 
                                                        scriptInfo, 
                                                        params.profiles, 
                                                        params.configPreferredProfile).DS_TRY();
        {
            ghc::filesystem::path buildDir = GetDefaultBuildDir().DS_TRY();
            BuildsManager buildsManager("/tmp");
            IncludeManager includeManager;
            InitializeBuildDirectory(   buildDir,
                                        absoluteScriptPath,
                                        params.buildLocally,
                                        buildsManager,
                                        buildDir,
                                        includeManager).DS_TRY();
            HandleCleanup(  scriptInfo, 
                            params.profiles.at(profileIndex),
                            scriptDirectory,
                            buildDir,
                            absoluteScriptPath,
                            buildsManager).DS_TRY();
        }
        
        return {};
    }

    inline DS::Result<void> RunResetDependencies(   CoreParams params,
                                                    const std::string& targetDepToReset)
    {
        ghc::filesystem::path absoluteScriptPath;
        ghc::filesystem::path scriptDirectory;
        std::string scriptName;
        std::unordered_map<std::string, std::string> parameters;
        Data::ScriptInfo scriptInfo;
        GetScriptInfoData(  params.scriptPath, 
                            params.rawParameters, 
                            
                            //Output:
                            scriptInfo,
                            absoluteScriptPath,
                            scriptDirectory,
                            scriptName,
                            parameters).DS_TRY();
        
        if(params.profiles.empty())
            return DS_ERROR_MSG("No compiler profiles found");
        
        int profileIndex =  GetPreferredProfileIndex(   absoluteScriptPath, 
                                                        scriptInfo, 
                                                        params.profiles, 
                                                        params.configPreferredProfile).DS_TRY();
        
        
        //Parsing the script, setting up dependencies, compiling and linking
        std::vector<std::string> filesToCopyPaths;
        ghc::filesystem::path buildDir = GetDefaultBuildDir().DS_TRY();
        
        BuildsManager buildsManager("/tmp");
        IncludeManager includeManager;
        InitializeBuildDirectory(   buildDir,
                                    absoluteScriptPath,
                                    params.buildLocally,
                                    buildsManager,
                                    buildDir,
                                    includeManager).DS_TRY();
        ResolveDependenciesImports(scriptInfo, scriptDirectory, buildDir, parameters).DS_TRY();
        
        //Process Dependencies
        ResetDependencies(  scriptInfo,
                            params.profiles.at(profileIndex), 
                            scriptDirectory,
                            buildDir,
                            targetDepToReset).DS_TRY();
        return {};
    }

    inline DS::Result<bool> 
    CheckSourcesNeedUpdate( CoreParams params,
                            const std::string rawMaxThreads,
                            const Data::ScriptInfo* lastScriptInfo,
                            const ghc::filesystem::file_time_type& prevFinalSourceWriteTime,
                            const ghc::filesystem::file_time_type& prevFinalIncludeWriteTime)
    {
        ghc::filesystem::path absoluteScriptPath;
        ghc::filesystem::path scriptDirectory;
        std::string scriptName;
        std::unordered_map<std::string, std::string> parameters;
        Data::ScriptInfo scriptInfo;
        
        //TODO: Reduce number of parameters here
        GetScriptInfoData(  params.scriptPath, 
                            params.rawParameters, 
                            
                            //Output:
                            scriptInfo,
                            absoluteScriptPath,
                            scriptDirectory,
                            scriptName,
                            parameters).DS_TRY();
        
        //First check if script info file has changed
        {
            std::error_code e;
            ghc::filesystem::path dedicatedYamlLoc = 
                scriptDirectory / ghc::filesystem::path(scriptName + ".yaml");
            
            ghc::filesystem::file_time_type currentWriteTime;
            if(ghc::filesystem::exists(dedicatedYamlLoc, e))
                currentWriteTime = ghc::filesystem::last_write_time(dedicatedYamlLoc, e);
            else
                currentWriteTime = ghc::filesystem::last_write_time(absoluteScriptPath, e);

            if(e)
                return DS_ERROR_MSG("Failed to get write time for script info" + e.message());

            //If script info file is newer than last check, we need to update
            if(currentWriteTime > scriptInfo.LastWriteTime)
                return true;
        }
        
        if(params.profiles.empty())
            return DS_ERROR_MSG("No compiler profiles found");
        
        int profileIndex =  GetPreferredProfileIndex(   absoluteScriptPath, 
                                                        scriptInfo, 
                                                        params.profiles, 
                                                        params.configPreferredProfile).DS_TRY();
        
        //Parsing the script, setting up dependencies, compiling and linking
        std::vector<std::string> filesToCopyPaths;
        ghc::filesystem::path buildDir = GetDefaultBuildDir().DS_TRY();
        BuildsManager buildsManager("/tmp");
        IncludeManager includeManager;
        InitializeBuildDirectory(   buildDir,
                                    absoluteScriptPath,
                                    params.buildLocally,
                                    buildsManager,
                                    buildDir,
                                    includeManager).DS_TRY();
        
        const int maxThreads = rawMaxThreads.empty() ? 8 : strtol(  rawMaxThreads.c_str(), 
                                                                    nullptr, 
                                                                    10);
        if(maxThreads <= 0)
            return DS_ERROR_MSG("Invalid number of threads passed in");
        
        ResolveDependenciesImports(scriptInfo, scriptDirectory, buildDir, parameters).DS_TRY();
        
        //Check if script info has changed if provided and run setup if needed
        bool recompileNeeded = false;
        bool relinkNeeded = false;
        std::vector<std::string> changedDependencies;
        CheckScriptInfoChanges( buildDir, 
                                scriptInfo, 
                                params.profiles.at(profileIndex), 
                                scriptDirectory,
                                lastScriptInfo, 
                                parameters,
                                recompileNeeded, 
                                relinkNeeded, 
                                changedDependencies).DS_TRY();
        if(relinkNeeded)
            return true;
        
        std::vector<std::string> gatheredBinariesPaths;
        
        //Process Dependencies
        std::vector<Data::DependencyInfo*> availableDependencies;
        ProcessDependencies(scriptInfo,
                            params.profiles.at(profileIndex),
                            scriptDirectory,
                            buildDir,
                            changedDependencies,
                            false,
                            maxThreads,
                            availableDependencies,
                            gatheredBinariesPaths).DS_TRY();
        
        //Get all the files we are trying to compile
        std::vector<ghc::filesystem::path> sourceFiles;
        GatherSourceFiles(  absoluteScriptPath, 
                            scriptInfo, 
                            params.profiles.at(profileIndex), 
                            sourceFiles).DS_TRY();

        //Check if we have already compiled before.
        std::vector<bool> sourceHasCache;
        std::vector<ghc::filesystem::path> cachedObjectsFiles;
        ghc::filesystem::file_time_type finalObjectWriteTime;
        ghc::filesystem::file_time_type finalSourceWriteTime;
        ghc::filesystem::file_time_type finalIncludeWriteTime;
        HasCompiledCache(   scriptDirectory,
                            sourceFiles, 
                            buildDir, 
                            params.profiles.at(profileIndex),
                            includeManager,
                            sourceHasCache,
                            cachedObjectsFiles,
                            finalObjectWriteTime,
                            finalSourceWriteTime,
                            finalIncludeWriteTime).DS_TRY();
        
        if( finalSourceWriteTime > prevFinalSourceWriteTime ||
            finalIncludeWriteTime > prevFinalIncludeWriteTime)
        {
            return true;
        }
        else
            return false;
    }

    struct RunParams
    {
        CoreParams Core;
        bool rebuild;
        bool compileOnly;
        bool buildSourceOnly;
        bool buildOnly;
        const std::vector<std::string>& runArgs;
        const std::string rawMaxThreads;
        const Data::ScriptInfo* lastScriptInfo;
        const ghc::filesystem::path& buildOutputDir;
    };

    inline DS::Result<int> Run( RunParams runParams,
                                Data::ScriptInfo& outScriptInfo,
                                ghc::filesystem::file_time_type& outFinalSourceWriteTime,
                                ghc::filesystem::file_time_type& outFinalIncludeWriteTime)
    {
        ssLOG_FUNC_INFO();
        
        ghc::filesystem::path absoluteScriptPath;
        ghc::filesystem::path scriptDirectory;
        std::string scriptName;
        std::unordered_map<std::string, std::string> parameters;
        Data::ScriptInfo scriptInfo;
        
        //TODO: Reduce number of parameters here
        GetScriptInfoData(  runParams.Core.scriptPath, 
                            runParams.Core.rawParameters, 
                            
                            //Output:
                            scriptInfo,
                            absoluteScriptPath,
                            scriptDirectory,
                            scriptName,
                            parameters).DS_TRY();
        
        if(runParams.Core.profiles.empty())
            return DS_ERROR_MSG("No compiler profiles found");
        
        int profileIndex = GetPreferredProfileIndex(absoluteScriptPath, 
                                                    scriptInfo, 
                                                    runParams.Core.profiles, 
                                                    runParams.Core.configPreferredProfile).DS_TRY();
        
        //Parsing the script, setting up dependencies, compiling and linking
        std::vector<ghc::filesystem::path> filesToCopyPaths;
        ghc::filesystem::path buildDir = GetDefaultBuildDir().DS_TRY();
        {
            BuildsManager buildsManager("/tmp");
            IncludeManager includeManager;
            InitializeBuildDirectory(   buildDir,
                                        absoluteScriptPath,
                                        runParams.Core.buildLocally,
                                        buildsManager,
                                        buildDir,
                                        includeManager).DS_TRY();
            
            const int maxThreads =  runParams.rawMaxThreads.empty() ? 
                                    8 : 
                                    strtol(runParams.rawMaxThreads.c_str(), nullptr, 10);
            if(maxThreads <= 0)
                return DS_ERROR_MSG("Invalid number of threads passed in");
            
            ResolveDependenciesImports(scriptInfo, scriptDirectory, buildDir, parameters).DS_TRY();
            
            //Check if script info has changed if provided and run setup if needed
            bool recompileNeeded = false;
            bool relinkNeeded = false;
            std::vector<std::string> changedDependencies;
            CheckScriptInfoChanges( buildDir, 
                                    scriptInfo, 
                                    runParams.Core.profiles.at(profileIndex), 
                                    scriptDirectory,
                                    runParams.lastScriptInfo, 
                                    parameters,
                                    recompileNeeded, 
                                    relinkNeeded, 
                                    changedDependencies).DS_TRY();
            outScriptInfo = scriptInfo;
            
            std::vector<std::string> gatheredBinariesPaths;
            
            //Process Dependencies
            std::vector<Data::DependencyInfo*> availableDependencies;
            ProcessDependencies(scriptInfo,
                                runParams.Core.profiles.at(profileIndex),
                                scriptDirectory,
                                buildDir,
                                changedDependencies,
                                runParams.buildSourceOnly,
                                maxThreads,
                                availableDependencies,
                                gatheredBinariesPaths).DS_TRY();
            
            //Get all the files we are trying to compile
            std::vector<ghc::filesystem::path> sourceFiles;
            GatherSourceFiles(  absoluteScriptPath, 
                                scriptInfo, 
                                runParams.Core.profiles.at(profileIndex), 
                                sourceFiles).DS_TRY();

            //Get all include paths
            std::vector<ghc::filesystem::path> sourceIncludePaths;
            std::vector<ghc::filesystem::path> depIncludePaths;
            GatherIncludePaths( scriptDirectory,
                                scriptInfo,
                                runParams.Core.profiles.at(profileIndex),
                                availableDependencies,
                                sourceIncludePaths,
                                depIncludePaths).DS_TRY();

            //Check if we have already compiled before.
            std::vector<bool> sourceHasCache;
            std::vector<ghc::filesystem::path> cachedObjectsFiles;
            ghc::filesystem::file_time_type finalObjectWriteTime;
            if(runParams.rebuild || recompileNeeded)
                sourceHasCache = std::vector<bool>(sourceFiles.size(), false);
            else
            {
                HasCompiledCache(   scriptDirectory,
                                    sourceFiles, 
                                    buildDir, 
                                    runParams.Core.profiles.at(profileIndex),
                                    includeManager,
                                    sourceHasCache,
                                    cachedObjectsFiles,
                                    finalObjectWriteTime,
                                    outFinalSourceWriteTime,
                                    outFinalIncludeWriteTime).DS_TRY();
            }
            
            runcpp2::SourceIncludeMap sourceIncludeMap;
            {
                std::vector<ghc::filesystem::path> allIncludePaths = sourceIncludePaths;
                allIncludePaths.insert( allIncludePaths.end(), 
                                        depIncludePaths.begin(), 
                                        depIncludePaths.end());
                runcpp2::GatherFilesIncludes(   sourceFiles, 
                                                sourceHasCache, 
                                                allIncludePaths, 
                                                sourceIncludeMap).DS_TRY();
            }
            for(int i = 0; i < sourceFiles.size(); ++i)
            {
                if(!sourceHasCache.at(i))
                {
                    ssLOG_DEBUG("Updating include record for " << sourceFiles.at(i).string());
                    if(sourceIncludeMap.count(sourceFiles.at(i)) == 0)
                    {
                        ssLOG_WARNING("Includes not gathered for " << sourceFiles.at(i).string());
                        continue;
                    }
                    
                    bool writeResult =  includeManager.WriteIncludeRecord
                                        (
                                            sourceFiles.at(i), 
                                            sourceIncludeMap.at(sourceFiles.at(i))
                                        );
                    if(!writeResult)
                    {
                        return DS_ERROR_MSG("Failed to write include record for " + 
                                            sourceFiles.at(i).string());
                    }
                }
                else
                {
                    ssLOG_DEBUG("Include record for " << sourceFiles.at(i).string() << 
                                " is up to date");
                }
            }
            
            std::vector<ghc::filesystem::path> depLinkFilesPaths;
            SeparateDependencyFiles(runParams.Core.profiles.at(profileIndex).FilesTypes, 
                                    gatheredBinariesPaths, 
                                    depLinkFilesPaths, 
                                    filesToCopyPaths);
            
            //TODO: Allow user to pass the priorities outside
            //Set dependencies files to be lower priority
            std::vector<int> depBinaryFilesPriorities;
            for(int i = 0; i < depLinkFilesPaths.size(); ++i)
                depBinaryFilesPriorities.push_back(-100);
            
            //Get finalBinaryWriteTime by combining final object and dependencies write times
            std::error_code e;
            ghc::filesystem::file_time_type finalBinaryWriteTime = finalObjectWriteTime;
            for(int i = 0; i < depLinkFilesPaths.size(); ++i)
            {
                if(!ghc::filesystem::exists(depLinkFilesPaths.at(i), e))
                {
                    return DS_ERROR_MSG(depLinkFilesPaths.at(i).string() + 
                                        " reported as cached but doesn't exist");
                }
                
                ghc::filesystem::file_time_type lastWriteTime = 
                    ghc::filesystem::last_write_time(depLinkFilesPaths.at(i), e);

                if(lastWriteTime > finalBinaryWriteTime)
                    finalBinaryWriteTime = lastWriteTime;
            }
            
            //Run PreBuild commands before compilation
            HandlePreBuild(scriptInfo, runParams.Core.profiles.at(profileIndex), buildDir).DS_TRY();
            
            //Compiling/Linking
            bool outputCache = false;
            if(!HasOutputCache( sourceHasCache, 
                                buildDir, 
                                runParams.Core.profiles.at(profileIndex),
                                scriptInfo,
                                scriptName,
                                finalBinaryWriteTime,
                                outputCache))
            {
                ssLOG_WARNING(  "Error detected when trying to use output cache. "
                                "A cleanup is recommended");
            }
            
            if(!outputCache || relinkNeeded)
            {
                std::vector<ghc::filesystem::path> sourceLinkFilesPaths;
                std::vector<int> sourceBinaryFilesPriorities;
                for(int i = 0; i < cachedObjectsFiles.size(); ++i)
                {
                    sourceLinkFilesPaths.push_back(cachedObjectsFiles.at(i));
                    sourceBinaryFilesPriorities.push_back(0);
                }
                
                //TODO: Compile and link for watch as well. Load library as well
                if(runParams.compileOnly)
                {
                    CompileScriptOnly(  buildDir,
                                        scriptDirectory,
                                        sourceFiles,
                                        sourceHasCache,
                                        sourceIncludePaths, 
                                        depIncludePaths, 
                                        scriptInfo,
                                        runParams.Core.profiles.at(profileIndex),
                                        maxThreads).DS_TRY();
                    return 0;
                }
                else
                {
                    CompileAndLinkScript(   buildDir,
                                            scriptDirectory,
                                            ghc::filesystem::path(scriptName), 
                                            sourceFiles,
                                            sourceHasCache,
                                            sourceIncludePaths, 
                                            depIncludePaths, 
                                            scriptInfo,
                                            availableDependencies,
                                            runParams.Core.profiles.at(profileIndex),
                                            depLinkFilesPaths,
                                            depBinaryFilesPriorities,
                                            sourceLinkFilesPaths,
                                            sourceBinaryFilesPriorities,
                                            maxThreads)
                        .DS_TRY_ACT(DS_TMP_ERROR.Message += "\nFailed to compile or link script.";
                                    DS_APPEND_TRACE(DS_TMP_ERROR);
                                    return DS::Error(DS_TMP_ERROR));
                }
            }
        }
        
        //Trigger post build and run the script if needed
        int returnStatus = -1;
        {
            std::vector<ghc::filesystem::path> targets;
            ghc::filesystem::path runnableTarget;
            GetBuiltTargetPaths(buildDir, 
                                scriptName, 
                                runParams.Core.profiles.at(profileIndex), 
                                scriptInfo,
                                targets,
                                &runnableTarget).DS_TRY();
            
            if(targets.empty())
            {
                ssLOG_WARNING("No target files found");
                return 0;
            }
            
            //Copy files to build directory
            std::vector<std::string> copiedPaths;
            if(!runParams.buildOutputDir.empty())
            {
                std::error_code e;
                if(!ghc::filesystem::exists(runParams.buildOutputDir, e))
                {
                    if(!ghc::filesystem::create_directories(runParams.buildOutputDir, e))
                        return DS_ERROR_MSG("Failed to create output directory");
                }
                
                buildDir = runParams.buildOutputDir;
                //filesToCopyPaths.push_back(runnableTarget.string());
                for(const ghc::filesystem::path& target : targets)
                    filesToCopyPaths.push_back(target);
            }

            CopyFiles(buildDir, filesToCopyPaths, copiedPaths)
                .DS_TRY_ACT(DS_TMP_ERROR.Message += "\nFailed to copy binaries before running the "
                                                    "script";
                            DS_APPEND_TRACE(DS_TMP_ERROR);
                            return DS::Error(DS_TMP_ERROR));
            
            //Run PostBuild commands after successful compilation
            HandlePostBuild(scriptInfo, 
                            runParams.Core.profiles.at(profileIndex), 
                            buildDir.string()).DS_TRY();
            
            //Don't run if we are just watching or building
            if(runParams.buildOnly)
                return 0;
            
            //Run otherwise
            ssLOG_INFO("Running script...");
            RunCompiledOutput(  runnableTarget,
                                absoluteScriptPath, 
                                scriptInfo, 
                                runParams.runArgs, 
                                returnStatus).DS_TRY();
        }
        
        return returnStatus;
    }

    inline DS::Result<void> DownloadTutorial(char* runcppPath)
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
                return DS_ERROR_MSG("IO Error when trying to get cin");
            
            if(!input.empty())
            {
                if(input == "y" || input == "Y")
                    break;
                else if(input == "n" || input == "N")
                {
                    ssLOG_BASE("Not continuing");
                    return {};
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
                return DS_ERROR_MSG("Failed to download tutorial");
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
                return DS_ERROR_MSG("Failed to download tutorial");
            }
        #endif
        
        ssLOG_INFO("targetBranch: " << targetBranch);
        ssLOG_BASE("Downloaded InteractiveTutorial.cpp from github.");
        ssLOG_BASE("Do `" << runcppPath << " run InteractiveTutorial.cpp to start the tutorial.");
        
        return {};
    }
}

#include "runcpp2/LibYamlImpl.cpp"

#endif
