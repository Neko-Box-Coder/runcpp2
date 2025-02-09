#include "runcpp2/CompilingLinking.hpp"
#include "runcpp2/DependenciesHelper.hpp"
#include "runcpp2/PlatformUtil.hpp"
#include "runcpp2/StringUtil.hpp"
#include "runcpp2/Data/ScriptInfo.hpp"
#include "runcpp2/Data/DependencyLibraryType.hpp"
#include "runcpp2/Data/BuildTypeHelper.hpp"

#include "ssLogger/ssLog.hpp"
#include "System2.h"
#include "ghc/filesystem.hpp"

#include <future>
#include <chrono>

namespace
{
    using OverrideFlags = 
        std::unordered_map<PlatformName, runcpp2::Data::ProfilesFlagsOverride>;
 
    //TODO: Add option to add/remove flags for each run option (executable, shared, static)
    void AppendAndRemoveFlags(  const runcpp2::Data::Profile& profile,
                                const OverrideFlags overrideFlags,
                                std::string& inOutFlags)
    {
        ssLOG_FUNC_DEBUG();
        
        if(!runcpp2::HasValueFromPlatformMap(overrideFlags))
        {
            ssLOG_DEBUG("No override flags found for current platform");
            return;
        }
        
        const runcpp2::Data::ProfilesFlagsOverride& currentFlagsOverride = 
            *runcpp2::GetValueFromPlatformMap(overrideFlags);
        
        const runcpp2::Data::FlagsOverrideInfo* profileFlagsOverride = 
            runcpp2::GetValueFromProfileMap(profile, currentFlagsOverride.FlagsOverrides);
        
        if(!profileFlagsOverride)
        {
            ssLOG_DEBUG("No override flags found for current profile");
            return;
        }
        
        std::vector<std::string> flagsToRemove; 
        runcpp2::SplitString(profileFlagsOverride->Remove, " ", flagsToRemove);
        
        for(int i = 0; i < flagsToRemove.size(); ++i)
        {
            const std::string currentFlagToRemove = flagsToRemove.at(i);
            std::size_t foundIndex = inOutFlags.find(currentFlagToRemove);
            
            if(foundIndex != std::string::npos)
            {
                //Remove the trailing space as well if the flag is in the middle
                if(foundIndex + currentFlagToRemove.size() + 1 <= inOutFlags.size())
                    inOutFlags.erase(foundIndex, currentFlagToRemove.size() + 1);
                else
                    inOutFlags.erase(foundIndex, currentFlagToRemove.size());
                
                continue;
            }
            
            ssLOG_WARNING("Flag to remove not found: " << currentFlagToRemove);
        }
        
        runcpp2::TrimRight(inOutFlags);
        inOutFlags += " " + profileFlagsOverride->Append;
        runcpp2::TrimRight(inOutFlags);
    }
    
    bool PopulateFilesTypesMap( const runcpp2::Data::FilesTypesInfo& fileTypesInfo,
                                std::unordered_map< std::string, 
                                                    std::vector<std::string>>& outSubstitutionMap)
    {
        ssLOG_FUNC_DEBUG();
        
        #define INTERNAL_STR(a) #a
        #define INTERNAL_COMPOSE(a, b) a b
        #define INTERNAL_ADD_TO_MAP(target) \
            outSubstitutionMap[ "{" INTERNAL_COMPOSE(INTERNAL_STR, (target)) "}" ] = \
                { *runcpp2::GetValueFromPlatformMap(fileTypesInfo. target) }

        INTERNAL_ADD_TO_MAP(SharedLibraryFile.Prefix);
        INTERNAL_ADD_TO_MAP(SharedLinkFile.Prefix);
        INTERNAL_ADD_TO_MAP(StaticLinkFile.Prefix);
        INTERNAL_ADD_TO_MAP(ObjectLinkFile.Prefix);
        INTERNAL_ADD_TO_MAP(DebugSymbolFile.Prefix);
        
        INTERNAL_ADD_TO_MAP(SharedLibraryFile.Extension);
        INTERNAL_ADD_TO_MAP(SharedLinkFile.Extension);
        INTERNAL_ADD_TO_MAP(StaticLinkFile.Extension);
        INTERNAL_ADD_TO_MAP(ObjectLinkFile.Extension);
        INTERNAL_ADD_TO_MAP(DebugSymbolFile.Extension);
        
        #undef INTERNAL_STR
        #undef INTERNAL_COMPOSE
        #undef INTERNAL_ADD_TO_MAP
        
        return true;
    }
    
    bool CompileScript( const ghc::filesystem::path& buildDir,
                        const ghc::filesystem::path& scriptPath,
                        const std::vector<ghc::filesystem::path>& sourceFiles,
                        const std::vector<ghc::filesystem::path>& includePaths,
                        const runcpp2::Data::ScriptInfo& scriptInfo,
                        const std::vector<runcpp2::Data::DependencyInfo*>& availableDependencies,
                        const runcpp2::Data::Profile& profile,
                        bool compileAsExecutable,
                        std::vector<ghc::filesystem::path>& outObjectsFilesPaths,
                        const int maxThreads)
    {
        ssLOG_FUNC_INFO();
        
        outObjectsFilesPaths.clear();
        
        using OutputTypeInfo = runcpp2::Data::StageInfo::OutputTypeInfo;
        OutputTypeInfo* tempOutputInfo = nullptr;
        static_assert(  static_cast<int>(runcpp2::Data::BuildType::COUNT) == 4, 
                        "Add new type to be processed");
        switch(scriptInfo.CurrentBuildType)
        {
            case runcpp2::Data::BuildType::STATIC:
                tempOutputInfo = const_cast<OutputTypeInfo*>
                (
                    runcpp2::GetValueFromPlatformMap(profile.Compiler.OutputTypes.Static)
                );
                break;
            case runcpp2::Data::BuildType::SHARED:
                tempOutputInfo = const_cast<OutputTypeInfo*>
                (
                    runcpp2::GetValueFromPlatformMap(profile.Compiler.OutputTypes.Shared)
                );
                break;
            case runcpp2::Data::BuildType::EXECUTABLE:
                if(compileAsExecutable)
                {
                    tempOutputInfo = const_cast<OutputTypeInfo*>
                    (
                        runcpp2::GetValueFromPlatformMap(profile.Compiler.OutputTypes.Executable)
                    );
                }
                else
                {
                    tempOutputInfo = const_cast<OutputTypeInfo*>
                    (
                        runcpp2::GetValueFromPlatformMap(profile.Compiler
                                                                .OutputTypes
                                                                .ExecutableShared)
                    );
                }
                break;
            default:
                ssLOG_ERROR("Unsupported build type for compiling: " << 
                            runcpp2::Data::BuildTypeToString(scriptInfo.CurrentBuildType));
                return false;
        }
        
        const OutputTypeInfo* currentOutputTypeInfo = tempOutputInfo;
        
        if(currentOutputTypeInfo == nullptr)
        {
            ssLOG_ERROR("Failed to find current platform for Compiler in OutputTypes");
            return false;
        }
        std::unordered_map<std::string, std::vector<std::string>> substitutionMapTemplate;
        
        substitutionMapTemplate["{Executable}"] = {(*currentOutputTypeInfo).Executable};
        
        //Compile flags
        {
            std::string compileFlags = (*currentOutputTypeInfo).Flags;
            AppendAndRemoveFlags(profile, scriptInfo.OverrideCompileFlags, compileFlags);
            substitutionMapTemplate["{CompileFlags}"] = {compileFlags};
        }
        
        //Add script and dependency include paths
        for(const ghc::filesystem::path& includePath : includePaths)
        {
            std::string processedInclude = runcpp2::ProcessPath(includePath.string());
            substitutionMapTemplate["{IncludeDirectoryPath}"].push_back(processedInclude);
        }
        
        //Add defines
        if(runcpp2::HasValueFromPlatformMap(scriptInfo.Defines))
        {
            const runcpp2::Data::ProfilesDefines& platformDefines = 
                *runcpp2::GetValueFromPlatformMap(scriptInfo.Defines);
            
            const std::vector<runcpp2::Data::Define>* profileDefines = 
                runcpp2::GetValueFromProfileMap(profile, platformDefines.Defines);
            
            if(profileDefines)
            {
                for(int i = 0; i < profileDefines->size(); ++i)
                {
                    const runcpp2::Data::Define& define = profileDefines->at(i);
                    if(define.HasValue)
                    {
                        substitutionMapTemplate["{DefineName}"].push_back(define.Name);
                        substitutionMapTemplate["{DefineValue}"].push_back(define.Value);
                    }
                    else
                        substitutionMapTemplate["{DefineNameOnly}"].push_back(define.Name);
                }
            }
        }
        
        if(!PopulateFilesTypesMap(profile.FilesTypes, substitutionMapTemplate))
            return false;
        
        substitutionMapTemplate["{/}"] = {runcpp2::ProcessPath("/")};
        
        std::unordered_map<std::string, std::vector<std::string>> substitutionMap;
        substitutionMap = substitutionMapTemplate;
        std::vector<std::future<bool>> actions;

        //Cache logs for worker threads
        ssLOG_ENABLE_CACHE_OUTPUT_FOR_NEW_THREADS();
        int logLevel = ssLOG_GET_CURRENT_THREAD_TARGET_LEVEL();
        
        //Compile async, allow compilation for all source files whether if it succeeded or not
        bool failedAny = false;
        for(int i = 0; i < sourceFiles.size(); ++i)
        {
            std::error_code e;
            ghc::filesystem::path currentSource = sourceFiles.at(i);
            ghc::filesystem::path relativeSourcePath = 
                ghc::filesystem::relative(currentSource, scriptPath.parent_path(), e);
            
            if(e)
            {
                ssLOG_ERROR("Failed to get relative path for " << currentSource);
                ssLOG_ERROR("Failed with error: " << e.message());
                actions.emplace_back(std::async(std::launch::deferred, []{return false;}));
                continue;
            }
            
            //TODO: Maybe do ProcessPath on all the .string()?
            std::string sourceDirectory = currentSource.parent_path().string();
            std::string sourceName = currentSource.stem().string();
            std::string sourceExt = currentSource.extension().string();
            //Input File
            {
                substitutionMap["{InputFileName}"] = {sourceName};
                substitutionMap["{InputFileExtension}"] = {sourceExt};
                substitutionMap["{InputFileDirectory}"] = {sourceDirectory};
                substitutionMap["{InputFilePath}"] = {currentSource.string()};
            }
            
            //Output File
            {
                substitutionMap["{OutputFileDirectory}"] = 
                    {runcpp2::ProcessPath( (buildDir / relativeSourcePath.parent_path()).string() )};
                
                if(!runcpp2::HasValueFromPlatformMap(profile.FilesTypes.ObjectLinkFile.Extension))
                {
                    ssLOG_ERROR("profile " << profile.Name << " missing extension for " <<
                                "object link file");
                    actions.emplace_back(std::async(std::launch::deferred, []{return false;}));
                    continue;
                }
                
                std::string objectExt = 
                    *runcpp2::GetValueFromPlatformMap(profile.FilesTypes.ObjectLinkFile.Extension);
                
                for(int j = 0; j < currentOutputTypeInfo->ExpectedOutputFiles.size(); ++j)
                {
                    std::string currentPath = currentOutputTypeInfo->ExpectedOutputFiles.at(j);
                    if(!profile.Compiler.PerformSubstituions(substitutionMap, currentPath))
                    {
                        ssLOG_ERROR("Failed to substitute \"" << currentPath << "\"");
                        actions.emplace_back(std::async(std::launch::deferred, []{return false;}));
                        continue;
                    }
                    
                    auto path = ghc::filesystem::path(currentPath);
                    ghc::filesystem::create_directories(path.parent_path(), e);
                    if(e)
                    {
                        ssLOG_ERROR("Failed to create directory structure for " << currentPath);
                        ssLOG_ERROR("Failed with error: " << e.message());
                        actions.emplace_back(std::async(std::launch::deferred, []{return false;}));
                        continue;
                    }
                    
                    if(path.extension() != objectExt)
                    {
                        ssLOG_DEBUG("Skipping " << currentPath << " for being added for linking");
                        continue;
                    }
                    
                    outObjectsFilesPaths.push_back(currentPath);
                    //TODO: Check if the current path exists after performing the compilation
                }
            }
            
            actions.emplace_back
            (
                std::async
                (
                    std::launch::async,
                    //Compile the source
                    [
                        i,
                        &profile,
                        &currentOutputTypeInfo,
                        substitutionMap,
                        &buildDir,
                        compileAsExecutable,
                        &scriptInfo,
                        logLevel
                    ]()
                    {
                        ssLOG_SET_CURRENT_THREAD_TARGET_LEVEL(logLevel);
                        
                        //Getting PreRun command
                        std::string preRun =    
                            runcpp2::HasValueFromPlatformMap(profile.Compiler.PreRun) ?
                            *runcpp2::GetValueFromPlatformMap(profile.Compiler.PreRun) : "";
                        
                        //Run setup first if any
                        for(int j = 0; j < currentOutputTypeInfo->Setup.size(); ++j)
                        {
                            std::string setupStep = currentOutputTypeInfo->Setup.at(j);
                            
                            if(!profile.Compiler.PerformSubstituions(substitutionMap, setupStep))
                            {
                                ssLOG_ERROR("Failed to substitute \"" << setupStep << "\"");
                                return false;
                            }
                            
                            if(!preRun.empty())
                                setupStep = preRun + " && " + setupStep;
                            
                            std::string setupOutput;
                            int setupResult;
                            if( !runcpp2::RunCommand(   setupStep, 
                                                        true,
                                                        buildDir.string(),
                                                        setupOutput, 
                                                        setupResult) || 
                                setupResult != 0)
                            {
                                ssLOG_ERROR("Setup command \"" << setupStep << "\" failed");
                                ssLOG_ERROR("Failed with result " << setupResult);
                                ssLOG_ERROR("Failed with output: \n" << setupOutput);
                                return false;
                            }
                        }
                        
                        std::string compileCommand;
                        
                        //Construct the compile command
                        {
                            std::string runPartSubstitutedCommand;
                            if(!profile.Compiler.ConstructCommand(  substitutionMap, 
                                                                    compileAsExecutable,
                                                                    scriptInfo.CurrentBuildType,
                                                                    runPartSubstitutedCommand))
                            {
                                ssLOG_ERROR("Failed to construct compile command");
                                return false;
                            }
                            
                            if(!preRun.empty())
                                compileCommand = preRun + " && " + runPartSubstitutedCommand;
                            else
                                compileCommand = runPartSubstitutedCommand;
                            
                            ssLOG_INFO( "running compile command: " << compileCommand <<
                                         " in " << buildDir.string());
                            
                            std::string commandOutput;
                            int resultCode = 0;
                            
                            if( !runcpp2::RunCommand(   compileCommand, 
                                                        true,
                                                        buildDir.string(),
                                                        commandOutput, 
                                                        resultCode) || 
                                resultCode != 0)
                            {
                                ssLOG_ERROR("Compile command failed with result " << resultCode);
                                ssLOG_ERROR("Compile output: \n" << commandOutput);
                                return false;
                            }
                            else
                            {
                                //TODO: Make this configurable
                                //Attempt to capture warnings
                                if(commandOutput.find(" warning") != std::string::npos)
                                    ssLOG_WARNING("Warning detected:\n" << commandOutput);
                                else
                                    ssLOG_INFO("Compile output:\n" << commandOutput);
                            }
                        }
                        
                        //Run cleanup if any
                        for(int j = 0; j < currentOutputTypeInfo->Cleanup.size(); ++j)
                        {
                            std::string cleanupStep = currentOutputTypeInfo->Cleanup.at(j);
                            
                            if(!profile.Compiler.PerformSubstituions(substitutionMap, cleanupStep))
                            {
                                ssLOG_ERROR("Failed to substitute \"" << cleanupStep << "\"");
                                return false;
                            }
                            
                            if(!preRun.empty())
                                cleanupStep = preRun + " && " + cleanupStep;
                            
                            std::string cleanupOutput;
                            int cleanupResult;
                            
                            if( !runcpp2::RunCommand(   cleanupStep, 
                                                        true,
                                                        buildDir.string(),
                                                        cleanupOutput, 
                                                        cleanupResult) || 
                                cleanupResult != 0)
                            {
                                ssLOG_ERROR("Cleanup command \"" << cleanupStep << "\" failed");
                                ssLOG_ERROR("Failed with result " << cleanupResult);
                                ssLOG_ERROR("Failed with output: \n" << cleanupOutput);
                                return false;
                            }
                        }
                        
                        return true;
                    }
                )
            ); //actions.emplace_back
            
            //Evaluate the compile results for each batch for compilations
            if(actions.size() >= maxThreads || i == sourceFiles.size() - 1)
            {
                std::chrono::system_clock::time_point deadline = 
                    std::chrono::system_clock::now() + std::chrono::seconds(60);
                for(int j = 0; j < actions.size(); ++j)
                {
                    if(!actions.at(j).valid())
                    {
                        ssLOG_ERROR("Failed to construct actions for compiling");
                        ssLOG_OUTPUT_ALL_CACHE_GROUPED();
                        return false;
                    }
                    
                    std::future_status actionStatus = actions.at(j).wait_until(deadline);
                    bool result = false;
                    
                    if( actionStatus == std::future_status::deferred || 
                        actionStatus == std::future_status::ready)
                    {
                        result = actions.at(j).get();
                    }
                    else
                    {
                        ssLOG_ERROR("Compiling timeout");
                        ssLOG_OUTPUT_ALL_CACHE_GROUPED();
                        failedAny = true;
                    }
                    
                    if(!result)
                    {
                        ssLOG_ERROR("Compiling Failed");
                        ssLOG_OUTPUT_ALL_CACHE_GROUPED();
                        failedAny = true;
                    }
                }
                actions.clear();
            }
        }

        ssLOG_OUTPUT_ALL_CACHE_GROUPED();
        return !failedAny;
    }

    bool LinkScript(const ghc::filesystem::path& buildDir,
                    const std::string& outputName, 
                    const runcpp2::Data::ScriptInfo& scriptInfo,
                    const std::string& additionalLinkFlags,
                    const runcpp2::Data::Profile& profile,
                    const std::vector<ghc::filesystem::path>& objectsFilesPaths,
                    bool linkAsExecutable)
    {
        ssLOG_FUNC_INFO();
        const runcpp2::Data::StageInfo::OutputTypeInfo* currentOutputTypeInfo = nullptr;
        
        if(linkAsExecutable)
            currentOutputTypeInfo = runcpp2::GetValueFromPlatformMap(profile.Linker.OutputTypes.Executable);
        else
        {
            //Only use BuildType for non-executable builds
            static_assert(static_cast<int>(runcpp2::Data::BuildType::COUNT) == 4, 
                          "Add new type to be processed");
            switch(scriptInfo.CurrentBuildType) 
            {
                case runcpp2::Data::BuildType::STATIC:
                    currentOutputTypeInfo = 
                        runcpp2::GetValueFromPlatformMap(profile.Linker.OutputTypes.Static);
                    break;
                case runcpp2::Data::BuildType::SHARED:
                    currentOutputTypeInfo = 
                        runcpp2::GetValueFromPlatformMap(profile.Linker.OutputTypes.Shared);
                    break;
                case runcpp2::Data::BuildType::EXECUTABLE:
                    currentOutputTypeInfo = 
                        runcpp2::GetValueFromPlatformMap(profile.Linker
                                                                .OutputTypes
                                                                .ExecutableShared);
                    break;
                default:
                    ssLOG_ERROR("Unsupported build type for linking: " << 
                                runcpp2::Data::BuildTypeToString(scriptInfo.CurrentBuildType));
                    return false;
            }
        }
        
        if(currentOutputTypeInfo == nullptr)
        {
            ssLOG_ERROR("Failed to find current platform for Linker in OutputTypes");
            return false;
        }
        std::unordered_map<std::string, std::vector<std::string>> substitutionMap;
        substitutionMap["{Executable}"] = {(*currentOutputTypeInfo).Executable};
        
        //Link Flags
        {
            std::string linkFlags = (*currentOutputTypeInfo).Flags;
            AppendAndRemoveFlags(profile, scriptInfo.OverrideLinkFlags, linkFlags);
            
            if(!additionalLinkFlags.empty())
            {
                if(linkFlags.empty())
                    linkFlags = additionalLinkFlags;
                else
                    linkFlags += std::string(" ") + additionalLinkFlags;
            }
            
            substitutionMap["{LinkFlags}"] = {linkFlags};
        }
        
        //Output File
        substitutionMap["{OutputFileName}"] = {outputName};
        substitutionMap["{OutputFileDirectory}"] = {buildDir.string()};
        
        if(!PopulateFilesTypesMap(profile.FilesTypes, substitutionMap))
            return false;
        
        substitutionMap["{/}"] = {runcpp2::ProcessPath("/")};
        
        //Link Files
        {
            for(int i = 0; i < objectsFilesPaths.size(); ++i)
            {
                ssLOG_DEBUG("Trying to link " << objectsFilesPaths.at(i));
                using namespace runcpp2;
                
                //Check if this is a file we can link
                std::string extension = GetFileExtensionWithoutVersion(objectsFilesPaths.at(i));
                Data::DependencyLibraryType currentLinkType = Data::DependencyLibraryType::COUNT;
                
                if(!HasValueFromPlatformMap(profile.FilesTypes.ObjectLinkFile.Extension))
                {
                    ssLOG_WARNING(  "profile " << profile.Name << " missing extension for " <<
                                    "object link file");
                }
                else if(extension == 
                        *GetValueFromPlatformMap(profile.FilesTypes.ObjectLinkFile.Extension))
                {
                    currentLinkType = Data::DependencyLibraryType::OBJECT;
                    goto processLinkFile;
                }
                
                //TODO: Shared and static link file cannot be differentiated with just 
                //      file extension because they are the same. We should pass the dependency in 
                //      instead
                if(!HasValueFromPlatformMap(profile.FilesTypes.SharedLinkFile.Extension))
                {
                    ssLOG_WARNING(  "profile " << profile.Name << " missing extension for " <<
                                    "shared link file");
                }
                else if(extension == 
                        *GetValueFromPlatformMap(profile.FilesTypes.SharedLinkFile.Extension))
                {
                    currentLinkType = Data::DependencyLibraryType::SHARED;
                    goto processLinkFile;
                }
                
                if(!HasValueFromPlatformMap(profile.FilesTypes.StaticLinkFile.Extension))
                {
                    ssLOG_WARNING(  "profile " << profile.Name << " missing extension for " <<
                                    "static link file");
                }
                else if(extension == 
                        *GetValueFromPlatformMap(profile.FilesTypes.StaticLinkFile.Extension))
                {
                    currentLinkType = Data::DependencyLibraryType::STATIC;
                    
                    if(!linkAsExecutable)
                    {
                        ssLOG_WARNING(  "Trying to link static dependency when script is being " <<
                                        "built as shared. Linking might not work on some platforms.");
                        
                        //TODO: Maybe revert the default back to executable?
                        ssLOG_WARNING(  "If linking fails, run with -e instead");
                    }
                    
                    goto processLinkFile;
                }
                
                ssLOG_WARNING("Failing to match current extension: " << extension);
                
                processLinkFile:;
                if(currentLinkType == Data::DependencyLibraryType::COUNT)
                {
                    ssLOG_DEBUG("Skip linking " << objectsFilesPaths.at(i));
                    continue;
                }
                
                auto depLinkParsedPath = objectsFilesPaths.at(i);
                std::string depLinkDirectory = depLinkParsedPath.parent_path().string();
                std::string depLinkName = depLinkParsedPath.stem().string();
                std::string depLinkExt = depLinkParsedPath.extension().string();
                
                substitutionMap["{LinkFileName}"].push_back(depLinkName);
                substitutionMap["{LinkFileExt}"].push_back(depLinkExt);
                substitutionMap["{LinkFileDirectory}"].push_back(depLinkDirectory);
                const std::string processedLinkFilePath = 
                    runcpp2::ProcessPath(objectsFilesPaths.at(i));
                substitutionMap["{LinkFilePath}"].push_back(processedLinkFilePath);
                
                static_assert(  static_cast<int>(Data::DependencyLibraryType::COUNT) == 4, 
                                "Add new type to be processed");
                switch(currentLinkType)
                {
                    case Data::DependencyLibraryType::STATIC:
                    {
                        substitutionMap["{LinkStaticFileName}"].push_back(depLinkName);
                        substitutionMap["{LinkStaticFileExt}"].push_back(depLinkExt);
                        substitutionMap["{LinkStaticFileDirectory}"].push_back(depLinkDirectory);
                        substitutionMap["{LinkStaticFilePath}"].push_back(processedLinkFilePath);
                        break;
                    }
                    case Data::DependencyLibraryType::SHARED:
                    {
                        substitutionMap["{LinkSharedFileName}"].push_back(depLinkName);
                        substitutionMap["{LinkSharedFileExt}"].push_back(depLinkExt);
                        substitutionMap["{LinkSharedFileDirectory}"].push_back(depLinkDirectory);
                        substitutionMap["{LinkSharedFilePath}"].push_back(processedLinkFilePath);
                        break;
                    }
                    case Data::DependencyLibraryType::OBJECT:
                    {
                        substitutionMap["{LinkObjectFileName}"].push_back(depLinkName);
                        substitutionMap["{LinkObjectFileExt}"].push_back(depLinkExt);
                        substitutionMap["{LinkObjectFileDirectory}"].push_back(depLinkDirectory);
                        substitutionMap["{LinkObjectFilePath}"].push_back(processedLinkFilePath);
                        break;
                    }
                    case Data::DependencyLibraryType::HEADER:
                    case Data::DependencyLibraryType::COUNT:
                    {
                        ssLOG_WARNING(  "Unexpected currentLinkType: " << 
                                        static_cast<int>(currentLinkType) << 
                                        " for " << objectsFilesPaths.at(i));
                        break;
                    }
                }
            }
        }
        
        //Use ExpectedOutputFiles?
        #if 0
            for(int i = 0; i < currentOutputTypeInfo->ExpectedOutputFiles.size(); ++i)
            {
                std::string currentPath = currentOutputTypeInfo->ExpectedOutputFiles.at(i);
                if(!profile.Linker.PerformSubstituions(substitutionMap, currentPath))
                {
                    ssLOG_ERROR("Failed to substitute \"" << currentPath << "\"");
                    return false;
                }
                
                outObjectsFilesPaths.push_back(currentPath);
            }
        #endif
        
        //Link the script
        {
            //Getting PreRun command
            std::string preRun =    runcpp2::HasValueFromPlatformMap(profile.Linker.PreRun) ?
                                    *runcpp2::GetValueFromPlatformMap(profile.Linker.PreRun) : "";
            
            //Run setup first if any
            for(int i = 0; i < currentOutputTypeInfo->Setup.size(); ++i)
            {
                std::string setupStep = currentOutputTypeInfo->Setup.at(i);
                
                if(!profile.Linker.PerformSubstituions(substitutionMap, setupStep))
                {
                    ssLOG_ERROR("Failed to substitute \"" << setupStep << "\"");
                    return false;
                }
                
                if(!preRun.empty())
                    setupStep = preRun + " && " + setupStep;
                
                std::string setupOutput;
                int setupResult;
                
                if( !runcpp2::RunCommand(   setupStep, 
                                            true,
                                            buildDir.string(),
                                            setupOutput, 
                                            setupResult) || 
                    setupResult != 0)
                {
                    ssLOG_ERROR("Setup command \"" << setupStep << "\" failed");
                    ssLOG_ERROR("Failed with result " << setupResult);
                    ssLOG_ERROR("Failed with output: \n" << setupOutput);
                    return false;
                }
            }
            
            
            std::string linkCommand;
            
            //Construct the link command
            {
                std::string runPartSubstitutedCommand;
                if(!profile.Linker.ConstructCommand(substitutionMap, 
                                                    linkAsExecutable,
                                                    scriptInfo.CurrentBuildType,
                                                    runPartSubstitutedCommand))
                {
                    ssLOG_ERROR("Failed to construct link command");
                    return false;
                }
                
                if(!preRun.empty())
                    linkCommand = preRun + " && " + runPartSubstitutedCommand;
                else
                    linkCommand = runPartSubstitutedCommand;
                
                ssLOG_INFO("running link command: " << linkCommand << " in " << buildDir.string());
                std::string linkOutput;
                int resultCode = 0;
                
                if( !runcpp2::RunCommand(   linkCommand, 
                                            true,
                                            buildDir.string(),
                                            linkOutput, 
                                            resultCode) ||
                    resultCode != 0)
                {
                    ssLOG_ERROR("Link command failed with result " << resultCode);
                    ssLOG_ERROR("Link output: \n" << linkOutput);
                    return false;
                }
                else
                    ssLOG_INFO("Link output:\n" << linkOutput);
            }
            
            //Run cleanup if any
            for(int i = 0; i < currentOutputTypeInfo->Cleanup.size(); ++i)
            {
                std::string cleanupStep = currentOutputTypeInfo->Cleanup.at(i);
                
                if(!profile.Linker.PerformSubstituions(substitutionMap, cleanupStep))
                {
                    ssLOG_ERROR("Failed to substitute \"" << cleanupStep << "\"");
                    return false;
                }
                
                if(!preRun.empty())
                    cleanupStep = preRun + " && " + cleanupStep;
                
                std::string cleanupOutput;
                int cleanupResult;
                
                if( !runcpp2::RunCommand(   cleanupStep, 
                                            true,
                                            buildDir.string(),
                                            cleanupOutput, 
                                            cleanupResult) || 
                    cleanupResult != 0)
                {
                    ssLOG_ERROR("Cleanup command \"" << cleanupStep << "\" failed");
                    ssLOG_ERROR("Failed with result " << cleanupResult);
                    ssLOG_ERROR("Failed with output: \n" << cleanupOutput);
                    return false;
                }
            }
        }
        
        return true;
    }
    
    bool RunGlobalSteps(const ghc::filesystem::path& buildDir,
                        const std::unordered_map<   PlatformName, 
                                                    std::vector<std::string>>& platformSteps)
    {
        if(platformSteps.empty())
            return true;
        
        if( !runcpp2::HasValueFromPlatformMap(platformSteps) ||
            runcpp2::GetValueFromPlatformMap(platformSteps)->empty())
        {
            return true;
        }
        
        const std::vector<std::string>& steps = *runcpp2::GetValueFromPlatformMap(platformSteps);
        
        for(int i = 0; i < steps.size(); ++i)
        {
            std::string commandOutput;
            int commandResult = 0;
            
            if( !runcpp2::RunCommand(   steps.at(i), 
                                        true,
                                        buildDir.string(),
                                        commandOutput,
                                        commandResult) ||
                commandResult != 0)
            {
                ssLOG_ERROR("Command \"" << steps.at(i) << "\" failed");
                ssLOG_ERROR("Failed with result " << commandResult);
                ssLOG_ERROR("Failed with output: \n" << commandOutput);
                return false;
            }
        }
        
        return true;
    }
}

bool runcpp2::CompileScriptOnly(const ghc::filesystem::path& buildDir,
                                const ghc::filesystem::path& scriptPath,
                                const std::vector<ghc::filesystem::path>& sourceFiles,
                                const std::vector<bool>& sourceHasCache,
                                const std::vector<ghc::filesystem::path>& includePaths,
                                const Data::ScriptInfo& scriptInfo,
                                const std::vector<Data::DependencyInfo*>& availableDependencies,
                                const Data::Profile& profile,
                                bool buildExecutable,
                                const int maxThreads)
{
    if(!RunGlobalSteps(buildDir, profile.Setup))
    {
        ssLOG_ERROR("Failed to run profile global setup steps");
        return false;
    }
    
    std::vector<ghc::filesystem::path> sourceFilesNeededToCompile;

    for(int i = 0; i < sourceFiles.size(); ++i)
    {
        if(!sourceHasCache.at(i))
            sourceFilesNeededToCompile.push_back(sourceFiles.at(i));
    }

    std::vector<ghc::filesystem::path> objectsFilesPaths;

    if(!CompileScript(  buildDir,
                        scriptPath,
                        sourceFilesNeededToCompile, 
                        includePaths,
                        scriptInfo, 
                        availableDependencies, 
                        profile, 
                        buildExecutable, 
                        objectsFilesPaths,
                        maxThreads))
    {
        ssLOG_ERROR("CompileScript failed");
        if(!RunGlobalSteps(buildDir, profile.Cleanup))
            ssLOG_ERROR("Failed to run profile global cleanup steps");
        return false;
    }
    
    if(!RunGlobalSteps(buildDir, profile.Cleanup))
    {
        ssLOG_ERROR("Failed to run profile global cleanup steps");
        return false;
    }
    
    return true;
}

bool runcpp2::CompileAndLinkScript( const ghc::filesystem::path& buildDir,
                                    const ghc::filesystem::path& scriptPath,
                                    const std::string& outputName,
                                    const std::vector<ghc::filesystem::path>& sourceFiles,
                                    const std::vector<bool>& sourceHasCache,
                                    const std::vector<ghc::filesystem::path>& includePaths,
                                    const Data::ScriptInfo& scriptInfo,
                                    const std::vector<Data::DependencyInfo*>& availableDependencies,
                                    const Data::Profile& profile,
                                    const std::vector<std::string>& compiledObjectsPaths,
                                    bool buildExecutable,
                                    const int maxThreads)
{
    if(!RunGlobalSteps(buildDir, profile.Setup))
    {
        ssLOG_ERROR("Failed to run profile global setup steps");
        return false;
    }
    
    std::vector<ghc::filesystem::path> sourceFilesNeededToCompile;
    for(int i = 0; i < sourceFiles.size(); ++i)
    {
        if(!sourceHasCache.at(i))
            sourceFilesNeededToCompile.push_back(sourceFiles.at(i));
    }

    std::vector<ghc::filesystem::path> objectsFilesPaths;

    //Compile source files that don't have cache
    if(!CompileScript(  buildDir,
                        scriptPath,
                        sourceFilesNeededToCompile, 
                        includePaths,
                        scriptInfo, 
                        availableDependencies, 
                        profile, 
                        buildExecutable, 
                        objectsFilesPaths,
                        maxThreads))
    {
        ssLOG_ERROR("CompileScript failed");
        if(!RunGlobalSteps(buildDir, profile.Cleanup))
            ssLOG_ERROR("Failed to run profile global cleanup steps");
        return false;
    }
    
    //Add compiled object files
    for(int i = 0; i < compiledObjectsPaths.size(); ++i)
        objectsFilesPaths.push_back(compiledObjectsPaths.at(i));

    //Skip linking if build type doesn't need it
    if(!Data::BuildTypeHelper::NeedsLinking(scriptInfo.CurrentBuildType))
    {
        ssLOG_INFO( "Skipping linking - build type is " << 
                    Data::BuildTypeToString(scriptInfo.CurrentBuildType));
        return true;
    }
    
    std::string dependenciesLinkFlags;
    
    //Add link flags for the dependencies
    for(int i = 0; i < availableDependencies.size(); ++i)
    {
        if(!runcpp2::HasValueFromPlatformMap(availableDependencies.at(i)->LinkProperties))
            continue;
        
        const runcpp2::Data::DependencyLinkProperty& linkProperty = 
            *runcpp2::GetValueFromPlatformMap(availableDependencies.at(i)->LinkProperties);
        
        const runcpp2::Data::ProfileLinkProperty* profileLinkProperty = 
            runcpp2::GetValueFromProfileMap(profile, linkProperty.ProfileProperties);
            
        if(!profileLinkProperty)
            continue;
        
        for(const std::string& option : profileLinkProperty->AdditionalLinkOptions)
            dependenciesLinkFlags += option + " ";
    }
    
    runcpp2::TrimRight(dependenciesLinkFlags);
    
    if(!LinkScript( buildDir,
                    outputName,
                    scriptInfo,
                    dependenciesLinkFlags,
                    profile,
                    objectsFilesPaths, 
                    buildExecutable))
    {
        ssLOG_ERROR("LinkScript failed");
        return false;
    }
    
    if(!RunGlobalSteps(buildDir, profile.Cleanup))
    {
        ssLOG_ERROR("Failed to run profile global cleanup steps");
        if(!RunGlobalSteps(buildDir, profile.Cleanup))
            ssLOG_ERROR("Failed to run profile global cleanup steps");
        return false;
    }
    
    return true;
}

