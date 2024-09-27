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
                            const std::vector<std::string>& runArgs,
                            int& returnStatus)
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
        
        System2CommandInfo runCommandInfo = {};
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
                std::vector<std::string> runArgsCopy = runArgs;
                runArgsCopy.insert(runArgsCopy.begin(), scriptPath);
                
                std::vector<char*> runArgsCStr(runArgsCopy.size());
                for(int i = 0; i < runArgsCopy.size(); ++i)
                    runArgsCStr.at(i) = &runArgsCopy.at(i).at(0);
                
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
            ssLOG_ERROR("Failed to get compile files for current platform");
            return false;
        }
        
        std::string foundProfileName;
        std::vector<std::string> currentProfileNames;
        
        //TODO: Make getting values with GetNames a common function 
        currentProfile.GetNames(currentProfileNames);
        for(int i = 0; i < currentProfileNames.size(); ++i)
        {
            if(compileFiles->CompilesFiles.count(currentProfileNames.at(i)) > 0)
            {
                foundProfileName = currentProfileNames.at(i);
                break;
            }
        }
        
        const std::vector<ghc::filesystem::path>& currentCompilesFiles = 
            compileFiles->CompilesFiles.at(foundProfileName);
        
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
            
            for(int i = 0; i < currentCompilesFiles.size(); ++i)
            {
                ghc::filesystem::path currentPath = currentCompilesFiles.at(i);
                if(currentPath.is_relative())
                    currentPath = scriptDirectory / currentPath;
                
                if(currentPath.is_relative())
                {
                    ssLOG_ERROR("Failed to process compile path: " << currentCompilesFiles.at(i));
                    ssLOG_ERROR("Try to append path to script directory but failed");
                    ssLOG_ERROR("Final appended path: " << currentPath);
                    return false;
                }
                
                std::error_code e;
                if(ghc::filesystem::is_directory(currentPath, e))
                {
                    ssLOG_ERROR("Directory is found instead of file: " << 
                                currentCompilesFiles.at(i));
                    return false;
                }
                
                if(!ghc::filesystem::exists(currentPath, e))
                {
                    ssLOG_ERROR("File doesn't exist: " << currentCompilesFiles.at(i));
                    return false;
                }
                
                outSourcePaths.push_back(currentPath);
            }
        }
        
        return true;
    }
    
    bool HasCompiledCache(  const std::vector<ghc::filesystem::path>& sourceFiles,
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
        
        for(int i = 0; i < sourceFiles.size(); ++i)
        {
            ghc::filesystem::path currentObjectFilePath = buildDir / sourceFiles.at(i).stem(); 
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
            }
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
            }
            return false;
        }
    }
}

void runcpp2::GetDefaultScriptInfo(std::string& scriptInfo)
{
    scriptInfo = std::string(   reinterpret_cast<const char*>(DefaultScriptInfo), 
                                DefaultScriptInfo_size);
}




runcpp2::PipelineResult 
runcpp2::StartPipeline( const std::string& scriptPath, 
                        const std::vector<Data::Profile>& profiles,
                        const std::string& configPreferredProfile,
                        const std::unordered_map<CmdOptions, std::string> currentOptions,
                        const std::vector<std::string>& runArgs,
                        const Data::ScriptInfo* lastScriptInfo,
                        Data::ScriptInfo& outScriptInfo,
                        int& returnStatus)
{
    INTERNAL_RUNCPP2_SAFE_START();
    
    //Do lexically_normal on all the paths
    //lexically_normal
    
    ssLOG_FUNC_DEBUG();
    
    if(profiles.empty())
    {
        ssLOG_ERROR("No compiler profiles found");
        return PipelineResult::EMPTY_PROFILES;
    }
    
    //Check if input file exists
    {
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
            return PipelineResult::INVALID_CONFIG_PATH;
        }
        
        configDir = configDir.parent_path();
        if(!ghc::filesystem::is_directory(configDir, e))
        {
            ssLOG_FATAL("Unexpected path for config directory: " << configDir.string());
            return PipelineResult::INVALID_CONFIG_PATH;
        }
    }
    
    //Create a class that manages build folder
    BuildsManager buildsManager(configDir);
    if(!buildsManager.Initialize())
    {
        ssLOG_FATAL("Failed to initialize builds manager");
        return PipelineResult::INVALID_BUILD_DIR;
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
                return PipelineResult::INVALID_SCRIPT_PATH;
            }

            std::stringstream buffer;
            buffer << inputFile.rdbuf();
            std::string source(buffer.str());
            
            if(!GetParsableInfo(source, parsableInfo))
            {
                ssLOG_ERROR("An error has been encountered when parsing info: " << 
                            absoluteScriptPath);
                return PipelineResult::INVALID_SCRIPT_INFO;
            }
        }
        
        //Try to parse the runcpp2 info
        Data::ScriptInfo scriptInfo;
        if(!ParseScriptInfo(parsableInfo, scriptInfo))
        {
            ssLOG_ERROR("Failed to parse info");
            ssLOG_ERROR("Content trying to parse: " << "\n" << parsableInfo);
            return PipelineResult::INVALID_SCRIPT_INFO;
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
                return PipelineResult::INVALID_BUILD_DIR;
            }
        }
        
        //Check if script info has changed if provided
        bool scriptInfoChanged = true;
        {
            ghc::filesystem::path lastScriptInfoFilePath = buildDir / "LastScriptInfo.yaml";
            
            //Compare script info in memory
            if(lastScriptInfo != nullptr)
                scriptInfoChanged = lastScriptInfo->ToString("") != scriptInfo.ToString("");
            //Compare script info in disk
            else
            {
                if(ghc::filesystem::exists(lastScriptInfoFilePath, e))
                {
                    ssLOG_DEBUG("Last script info file exists: " << lastScriptInfoFilePath);
                    std::ifstream lastScriptInfoFile;
                    lastScriptInfoFile.open(lastScriptInfoFilePath);
                    std::stringstream lastScriptInfoBuffer;
                    lastScriptInfoBuffer << lastScriptInfoFile.rdbuf();
                    scriptInfoChanged = lastScriptInfoBuffer.str() != scriptInfo.ToString("");
                }
            }
            
            std::ofstream writeOutputFile(lastScriptInfoFilePath);
            if(!writeOutputFile)
            {
                ssLOG_ERROR("Failed to open file: " << lastScriptInfoFilePath);
                //TODO: Maybee add a pipeline result for this?
                return PipelineResult::INVALID_BUILD_DIR;
            }

            writeOutputFile << scriptInfo.ToString("");
            
            //Pass the current script info out
            outScriptInfo = scriptInfo;
        }

        profileIndex = GetPreferredProfileIndex(absoluteScriptPath, 
                                                scriptInfo, 
                                                profiles, 
                                                configPreferredProfile);

        if(profileIndex == -1)
        {
            ssLOG_ERROR("Failed to find a profile to run");
            return PipelineResult::NO_AVAILABLE_PROFILE;
        }
        
        std::vector<std::string> copiedBinariesPaths;
        
        //Process Dependencies
        std::vector<Data::DependencyInfo*> availableDependencies;
        do
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
                return PipelineResult::DEPENDENCIES_FAILED;
            }
            
            if( currentOptions.count(CmdOptions::RESET_CACHE) > 0 ||
                currentOptions.count(CmdOptions::REMOVE_DEPENDENCIES) > 0 ||
                scriptInfoChanged)
            {
                if(!CleanupDependencies(profiles.at(profileIndex),
                                        scriptInfo,
                                        availableDependencies,
                                        dependenciesLocalCopiesPaths))
                {
                    ssLOG_ERROR("Failed to cleanup dependencies");
                    return PipelineResult::DEPENDENCIES_FAILED;
                }
            }
            
            if(currentOptions.count(CmdOptions::REMOVE_DEPENDENCIES) > 0)
            {
                ssLOG_LINE("Removed script dependencies");
                return PipelineResult::SUCCESS;
            }
            
            if(!SetupDependenciesIfNeeded(  profiles.at(profileIndex), 
                                            buildDir,
                                            scriptInfo, 
                                            availableDependencies,
                                            dependenciesLocalCopiesPaths,
                                            dependenciesSourcePaths))
            {
                ssLOG_ERROR("Failed to setup script dependencies");
                return PipelineResult::DEPENDENCIES_FAILED;
            }
            
            //NOTE: We don't need to build and copy the dependencies for watch since we
            //      only need to compile the script
            if(currentOptions.count(CmdOptions::WATCH) > 0)
                break;
            
            if(!BuildDependencies(  profiles.at(profileIndex),
                                    scriptInfo,
                                    availableDependencies, 
                                    dependenciesLocalCopiesPaths))
            {
                ssLOG_ERROR("Failed to build script dependencies");
                return PipelineResult::DEPENDENCIES_FAILED;
            }

            if(!CopyDependenciesBinaries(   buildDir, 
                                            availableDependencies,
                                            dependenciesLocalCopiesPaths,
                                            profiles.at(profileIndex),
                                            copiedBinariesPaths))
            {
                ssLOG_ERROR("Failed to copy dependencies binaries");
                return PipelineResult::DEPENDENCIES_FAILED;
            }
        }
        while(0);
        
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
        
        if(currentOptions.count(runcpp2::CmdOptions::RESET_CACHE) > 0 || scriptInfoChanged)
            sourceHasCache = std::vector<bool>(sourceFiles.size(), false);
        else if(!HasCompiledCache(  sourceFiles, 
                                    buildDir, 
                                    profiles.at(profileIndex), 
                                    sourceHasCache,
                                    cachedObjectsFiles,
                                    finalObjectWriteTime))
        {
            //TODO: Maybee add a pipeline result for this?
            return PipelineResult::UNEXPECTED_FAILURE;
        }
        
        //Update finalObjectWriteTime
        for(int i = 0; i < copiedBinariesPaths.size(); ++i)
        {
            if(!ghc::filesystem::exists(copiedBinariesPaths.at(i), e))
            {
                ssLOG_ERROR(copiedBinariesPaths.at(i) << " reported as cached but doesn't exist");
                return PipelineResult::UNEXPECTED_FAILURE;
            }
            
            ghc::filesystem::file_time_type lastWriteTime = 
                ghc::filesystem::last_write_time(copiedBinariesPaths.at(i), e);
        
            if(lastWriteTime > finalObjectWriteTime)
                finalObjectWriteTime = lastWriteTime;
        }
        
        //Compiling/Linking
        if(!HasOutputCache( sourceHasCache, 
                            buildDir, 
                            profiles.at(profileIndex), 
                            currentOptions.count(CmdOptions::EXECUTABLE) > 0,
                            scriptName,
                            exeExt,
                            copiedBinariesPaths,
                            finalObjectWriteTime))
        {
            for(int i = 0; i < cachedObjectsFiles.size(); ++i)
                copiedBinariesPaths.push_back(cachedObjectsFiles.at(i));
            
            if(currentOptions.count(CmdOptions::WATCH) > 0)
            {
                if(!CompileScriptOnly(  buildDir,
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
                                            ghc::filesystem::path(absoluteScriptPath).stem(), 
                                            sourceFiles,
                                            sourceHasCache,
                                            scriptInfo,
                                            availableDependencies,
                                            profiles.at(profileIndex),
                                            copiedBinariesPaths,
                                            currentOptions.count(CmdOptions::EXECUTABLE) > 0,
                                            exeExt))
            {
                ssLOG_ERROR("Failed to compile or link script");
                return PipelineResult::COMPILE_LINK_FAILED;
            }
        }
    }

    //We are only compiling when watching changes
    if(currentOptions.count(CmdOptions::WATCH) > 0)
        return PipelineResult::SUCCESS;

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
        
        if(currentOptions.count(CmdOptions::EXECUTABLE) > 0)
        {
            //Running the script
            if(!RunCompiledScript(target, absoluteScriptPath, runArgs, returnStatus))
            {
                ssLOG_ERROR("Failed to run script");
                return PipelineResult::RUN_SCRIPT_FAILED;
            }
            
            return PipelineResult::SUCCESS;
        }
        //Load the shared library and run it
        else
        {
            if(!RunCompiledSharedLib(absoluteScriptPath, target, runArgs, returnStatus))
            {
                ssLOG_ERROR("Failed to run script");
                return PipelineResult::RUN_SCRIPT_FAILED;
            }
            
            return PipelineResult::SUCCESS;
        }
    }

    return PipelineResult::UNEXPECTED_FAILURE;
    
    INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(PipelineResult::UNEXPECTED_FAILURE);
}

