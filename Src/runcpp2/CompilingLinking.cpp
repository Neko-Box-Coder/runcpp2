#include "runcpp2/CompilingLinking.hpp"
#include "runcpp2/DependenciesHelper.hpp"
#include "runcpp2/PlatformUtil.hpp"
#include "runcpp2/StringUtil.hpp"
#include "runcpp2/Data/ScriptInfo.hpp"
#include "runcpp2/Data/DependencyLibraryType.hpp"

#include "ssLogger/ssLog.hpp"
#include "System2.h"
#include "ghc/filesystem.hpp"

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
            ssLOG_INFO("No override flags found for current platform");
            return;
        }
        
        const runcpp2::Data::ProfilesFlagsOverride& currentFlagsOverride = 
            *runcpp2::GetValueFromPlatformMap(overrideFlags);
        
        std::string foundProfileName;
        std::vector<std::string> currentProfileNames;
        profile.GetNames(currentProfileNames);
        
        for(int i = 0; i < currentProfileNames.size(); ++i)
        {
            if(currentFlagsOverride.FlagsOverrides.count(currentProfileNames.at(i)) > 0)
            {
                foundProfileName = currentProfileNames.at(i);
                break;
            }
        }
        
        if(foundProfileName.empty())
        {
            ssLOG_INFO("No override flags found for current profile");
            return;
        }
        
        std::vector<std::string> flagsToRemove; 
        runcpp2::SplitString(   currentFlagsOverride.FlagsOverrides.at(foundProfileName).Remove, 
                                " ", 
                                flagsToRemove);
        
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
            
            ssLOG_WARNING(  "Flag to remove not found: " << currentFlagToRemove);
        }
        
        runcpp2::TrimRight(inOutFlags);
        inOutFlags += " " + currentFlagsOverride.FlagsOverrides.at(foundProfileName).Append;
        runcpp2::TrimRight(inOutFlags);
    }
    
    bool CompileScript( const ghc::filesystem::path& buildDir,
                        const ghc::filesystem::path& scriptPath,
                        const std::vector<ghc::filesystem::path>& sourceFiles,
                        const runcpp2::Data::ScriptInfo& scriptInfo,
                        const std::vector<runcpp2::Data::DependencyInfo*>& availableDependencies,
                        const runcpp2::Data::Profile& profile,
                        bool compileAsExecutable,
                        std::vector<ghc::filesystem::path>& outObjectsFilesPaths)
    {
        ssLOG_FUNC_DEBUG();
        
        outObjectsFilesPaths.clear();
        
        const runcpp2::Data::StageInfo::OutputTypeInfo* currentOutputTypeInfo = 
            compileAsExecutable ? 
            runcpp2::GetValueFromPlatformMap(profile.Compiler.OutputTypes.Executable) :
            runcpp2::GetValueFromPlatformMap(profile.Compiler.OutputTypes.Shared);
        
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
        
        //Include Directories
        {
            for(int i = 0; i < availableDependencies.size(); ++i)
            {
                for(int j = 0; j < availableDependencies.at(i)->AbsoluteIncludePaths.size(); ++j)
                {
                    const std::string& currentIncludePath = 
                        availableDependencies.at(i)->AbsoluteIncludePaths.at(j);
                    
                    substitutionMapTemplate["{IncludeDirectoryPath}"].push_back(currentIncludePath);
                }
            }
        }
        
        // Add defines
        if(runcpp2::HasValueFromPlatformMap(scriptInfo.Defines))
        {
            const runcpp2::Data::ProfilesDefines& platformDefines = 
                *runcpp2::GetValueFromPlatformMap(scriptInfo.Defines);
            
            std::vector<std::string> profileNames;
            profile.GetNames(profileNames);
            
            std::string validProfileName;
            for(int i = 0; i < profileNames.size(); ++i)
            {
                if(platformDefines.Defines.count(profileNames[i]) > 0)
                {
                    validProfileName = profileNames.at(i);
                    break;
                }
            }
            
            if(!validProfileName.empty())
            {
                const std::vector<runcpp2::Data::Define>& profileDefines = 
                    platformDefines.Defines.at(validProfileName);
                
                for(int i = 0; i < profileDefines.size(); ++i)
                {
                    const runcpp2::Data::Define& define = profileDefines.at(i);
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
        
        std::unordered_map<std::string, std::vector<std::string>> substitutionMap;
        substitutionMap = substitutionMapTemplate;
        
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
                return false;
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
                std::string objectFileExt = 
                    *runcpp2::GetValueFromPlatformMap(profile.FilesTypes.ObjectLinkFile.Extension);
                
                if(objectFileExt.empty())
                    ssLOG_WARNING("Object file extension is empty");
                
                ghc::filesystem::path outputFilePath =  buildDir / 
                                                        relativeSourcePath.parent_path() / 
                                                        sourceName;
                outputFilePath.concat(objectFileExt);
                
                // Create the directory structure if it doesn't exist
                ghc::filesystem::create_directories(outputFilePath.parent_path(), e);
                if(e)
                {
                    ssLOG_ERROR("Failed to create directory structure for " << outputFilePath);
                    ssLOG_ERROR("Failed with error: " << e.message());
                    return false;
                }
                
                substitutionMap["{OutputFileName}"] = {sourceName};
                substitutionMap["{OutputFileExtension}"] = {objectFileExt};
                substitutionMap["{OutputFileDirectory}"] = 
                    {runcpp2::ProcessPath(outputFilePath.parent_path().string())};
                substitutionMap["{OutputFilePath}"] = 
                    {runcpp2::ProcessPath(outputFilePath.string())};
                outObjectsFilesPaths.push_back(outputFilePath);
            }
            
            //Compile the source
            {
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
                    
                    if( !runcpp2::RunCommandAndGetOutput(   setupStep, 
                                                            setupOutput, 
                                                            setupResult,
                                                            buildDir.string()) || 
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
                                                            runPartSubstitutedCommand))
                    {
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
                    if( !runcpp2::RunCommandAndGetOutput(   compileCommand, 
                                                            commandOutput, 
                                                            resultCode,
                                                            buildDir.string()) || 
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
                    
                    if( !runcpp2::RunCommandAndGetOutput(   cleanupStep, 
                                                            cleanupOutput, 
                                                            cleanupResult,
                                                            buildDir.string()) || 
                        cleanupResult != 0)
                    {
                        ssLOG_ERROR("Cleanup command \"" << cleanupStep << "\" failed");
                        ssLOG_ERROR("Failed with result " << cleanupResult);
                        ssLOG_ERROR("Failed with output: \n" << cleanupOutput);
                        return false;
                    }
                }
            }
        }
        
        return true;
    }

    bool LinkScript(const ghc::filesystem::path& buildDir,
                    const std::string& outputName, 
                    const runcpp2::Data::ScriptInfo& scriptInfo,
                    const std::string& additionalLinkFlags,
                    const runcpp2::Data::Profile& profile,
                    const std::vector<ghc::filesystem::path>& objectsFilesPaths,
                    bool linkAsExecutable,
                    const std::string& exeExt)
    {
        ssLOG_FUNC_DEBUG();
        const runcpp2::Data::StageInfo::OutputTypeInfo* currentOutputTypeInfo = 
            linkAsExecutable ? 
            runcpp2::GetValueFromPlatformMap(profile.Linker.OutputTypes.Executable) :
            runcpp2::GetValueFromPlatformMap(profile.Linker.OutputTypes.Shared);
        
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
        {
            const std::string sharedLibPrefix =
                *runcpp2::GetValueFromPlatformMap(profile.FilesTypes.SharedLinkFile.Prefix);
            const std::string sharedLibExtension =
                    *runcpp2::GetValueFromPlatformMap(profile   .FilesTypes
                                                                .SharedLibraryFile
                                                                .Extension);
            
            std::string outputFile =    linkAsExecutable ?
                                        outputName + exeExt :
                                        sharedLibPrefix + outputName + sharedLibExtension;
            
            outputFile = runcpp2::ProcessPath( (buildDir / outputFile).string() );
        
            substitutionMap["{OutputFileName}"] = {outputName};
            substitutionMap["{OutputFileExtension}"] = 
            {
                (linkAsExecutable ? exeExt : sharedLibExtension)
            };
            substitutionMap["{OutputFileDirectory}"] = {buildDir.string()};
            substitutionMap["{OutputFilePath}"] = {outputFile};
        }
        
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
                
                if( !runcpp2::RunCommandAndGetOutput(   setupStep, 
                                                        setupOutput, 
                                                        setupResult,
                                                        buildDir.string()) || 
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
                                                    runPartSubstitutedCommand))
                {
                    return false;
                }
                
                if(!preRun.empty())
                    linkCommand = preRun + " && " + runPartSubstitutedCommand;
                else
                    linkCommand = runPartSubstitutedCommand;
                
                ssLOG_INFO("running link command: " << linkCommand << " in " << buildDir.string());
                std::string linkOutput;
                int resultCode = 0;
                if( !runcpp2::RunCommandAndGetOutput(   linkCommand, 
                                                        linkOutput, 
                                                        resultCode, 
                                                        buildDir.string()) ||
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
                
                if( !runcpp2::RunCommandAndGetOutput(   cleanupStep, 
                                                        cleanupOutput, 
                                                        cleanupResult,
                                                        buildDir.string()) || 
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
            
            if( !runcpp2::RunCommandAndGetOutput(   steps.at(i), 
                                                    commandOutput,
                                                    commandResult,
                                                    buildDir.string()) ||
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
                                const Data::ScriptInfo& scriptInfo,
                                const std::vector<Data::DependencyInfo*>& availableDependencies,
                                const Data::Profile& profile,
                                bool buildExecutable)
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
                        scriptInfo, 
                        availableDependencies, 
                        profile, 
                        buildExecutable, 
                        objectsFilesPaths))
    {
        ssLOG_ERROR("CompileScript failed");
        return false;
    }
    
    return true;
}

bool runcpp2::CompileAndLinkScript( const ghc::filesystem::path& buildDir,
                                    const ghc::filesystem::path& scriptPath,
                                    const std::string& outputName,
                                    const std::vector<ghc::filesystem::path>& sourceFiles,
                                    const std::vector<bool>& sourceHasCache,
                                    const Data::ScriptInfo& scriptInfo,
                                    const std::vector<Data::DependencyInfo*>& availableDependencies,
                                    const Data::Profile& profile,
                                    const std::vector<std::string>& compiledObjectsPaths,
                                    bool buildExecutable,
                                    const std::string exeExt)
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
                        scriptInfo, 
                        availableDependencies, 
                        profile, 
                        buildExecutable, 
                        objectsFilesPaths))
    {
        ssLOG_ERROR("CompileScript failed");
        return false;
    }
    
    //Add compiled object files
    for(int i = 0; i < compiledObjectsPaths.size(); ++i)
        objectsFilesPaths.push_back(compiledObjectsPaths.at(i));
    
    std::string dependenciesLinkFlags;
    
    //Add link flags for the dependencies
    for(int i = 0; i < availableDependencies.size(); ++i)
    {
        std::string targetProfileName;
        std::vector<std::string> currentProfileNames;
        profile.GetNames(currentProfileNames);
        
        for(int j = 0; j < currentProfileNames.size(); ++j)
        {
            if( availableDependencies.at(i)->LinkProperties.find(currentProfileNames.at(j)) != 
                availableDependencies.at(i)->LinkProperties.end())
            {
                targetProfileName = currentProfileNames.at(j);
                break;
            }
        }
        
        if(targetProfileName.empty())
            continue;
        
        const runcpp2::Data::DependencyLinkProperty& currentLinkProperty = 
            availableDependencies.at(i)->LinkProperties.at(targetProfileName);
        
        if(runcpp2::HasValueFromPlatformMap(currentLinkProperty.AdditionalLinkOptions))
        {
            const std::vector<std::string> additionalLinkOptions = 
                *runcpp2::GetValueFromPlatformMap(currentLinkProperty.AdditionalLinkOptions);
            
            for(int k = 0; k < additionalLinkOptions.size(); ++k)
                dependenciesLinkFlags += additionalLinkOptions.at(k) + " ";
        }
    }
    
    runcpp2::TrimRight(dependenciesLinkFlags);
    
    if(!LinkScript( buildDir,
                    outputName,
                    scriptInfo,
                    dependenciesLinkFlags,
                    profile,
                    objectsFilesPaths, 
                    buildExecutable,
                    exeExt))
    {
        ssLOG_ERROR("LinkScript failed");
        return false;
    }
    
    if(!RunGlobalSteps(buildDir, profile.Cleanup))
    {
        ssLOG_ERROR("Failed to run profile global cleanup steps");
        return false;
    }
    
    return true;
}

