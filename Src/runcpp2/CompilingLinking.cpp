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
        
        if(currentFlagsOverride.FlagsOverrides.count(profile.Name) > 0)
            foundProfileName = profile.Name;
        else
        {
            for(auto it = profile.NameAliases.begin(); it != profile.NameAliases.end(); ++it)
            {
                if(currentFlagsOverride.FlagsOverrides.count(*it) > 0)
                {
                    foundProfileName = *it;
                    break;
                }
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
    
    bool CompileScript( const std::string& scriptPath, 
                        const runcpp2::Data::ScriptInfo& scriptInfo,
                        const runcpp2::Data::Profile& profile,
                        bool compileAsExecutable,
                        std::string& outScriptObjectFilePath)
    {
        ssLOG_FUNC_DEBUG();
        
        const runcpp2::Data::StageInfo::OutputTypeInfo* currentOutputTypeInfo = 
            compileAsExecutable ? 
            runcpp2::GetValueFromPlatformMap(profile.Compiler.OutputTypes.Executable) :
            runcpp2::GetValueFromPlatformMap(profile.Compiler.OutputTypes.Shared);
        
        if(currentOutputTypeInfo == nullptr)
        {
            ssLOG_ERROR("Failed to find current platform for Compiler in OutputTypes");
            return false;
        }
        std::unordered_map<std::string, std::vector<std::string>> substitutionMap;
        
        substitutionMap["{Executable}"] = {(*currentOutputTypeInfo).Executable};
        
        //Compile flags
        {
            std::string compileFlags = (*currentOutputTypeInfo).Flags;
            AppendAndRemoveFlags(profile, scriptInfo.OverrideCompileFlags, compileFlags);
            substitutionMap["{CompileFlags}"] = {compileFlags};
        }
        
        //Include Directories
        {
            for(int i = 0; i < scriptInfo.Dependencies.size(); ++i)
            {
                if(!runcpp2::IsDependencyAvailableForThisPlatform(scriptInfo.Dependencies.at(i)))
                    continue;
            
                for(int j = 0; j < scriptInfo.Dependencies.at(i).AbsoluteIncludePaths.size(); ++j)
                {
                    const std::string& currentIncludePath = 
                        scriptInfo.Dependencies.at(i).AbsoluteIncludePaths.at(j);
                    
                    substitutionMap["{IncludeDirectoryPath}"].push_back(currentIncludePath);
                }
            }
        }
        
        std::string scriptDirectory = ghc::filesystem::path(scriptPath).parent_path().string();
        std::string scriptName = ghc::filesystem::path(scriptPath).stem().string();
        std::string scriptExt = ghc::filesystem::path(scriptPath).extension().string();
        std::string runcpp2ScriptDir = runcpp2::ProcessPath(scriptDirectory + "/.runcpp2");
        //Input File
        {
            substitutionMap["{InputFileName}"] = {scriptName};
            substitutionMap["{InputFileExtension}"] = {scriptExt};
            substitutionMap["{InputFileDirectory}"] = {scriptDirectory};
            substitutionMap["{InputFilePath}"] = {scriptPath};
        }
        //Output File
        {
            std::string objectFileExt = 
                *runcpp2::GetValueFromPlatformMap(profile.FilesTypes.ObjectLinkFile.Extension);
            
            if(objectFileExt.empty())
                ssLOG_WARNING("Object file extension is empty");
            
            const std::string outputFilePath = 
                runcpp2::ProcessPath(runcpp2ScriptDir + "/" + scriptName + objectFileExt);
            
            substitutionMap["{OutputFileName}"] = {scriptName};
            substitutionMap["{OutputFileExtension}"] = {objectFileExt};
            substitutionMap["{OutputFileDirectory}"] = {runcpp2ScriptDir};
            substitutionMap["{OutputFilePath}"] = {outputFilePath};
            outScriptObjectFilePath = outputFilePath;
        }
        
        //Compile the script
        {
            //Getting PreRun command
            std::string preRun =    runcpp2::HasValueFromPlatformMap(profile.Compiler.PreRun) ?
                                    *runcpp2::GetValueFromPlatformMap(profile.Compiler.PreRun) : "";
            
            //Run setup first if any
            for(int i = 0; i < currentOutputTypeInfo->Setup.size(); ++i)
            {
                std::string setupStep = currentOutputTypeInfo->Setup.at(i);
                
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
                                                        runcpp2ScriptDir) || 
                    setupResult != 0)
                {
                    ssLOG_ERROR("Setup command \"" << setupStep << "\" failed");
                    ssLOG_ERROR("Failed with result " << setupResult);
                    ssLOG_ERROR("Failed with output: " << setupOutput);
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
                             " in " << runcpp2ScriptDir);
                
                std::string commandOutput;
                int resultCode = 0;
                if( !runcpp2::RunCommandAndGetOutput(   compileCommand, 
                                                        commandOutput, 
                                                        resultCode,
                                                        runcpp2ScriptDir) || 
                    resultCode != 0)
                {
                    ssLOG_ERROR("Compile command failed with result " << resultCode);
                    ssLOG_ERROR("Compile output: " << commandOutput);
                    return false;
                }
            }
            
            //Run cleanup if any
            for(int i = 0; i < currentOutputTypeInfo->Cleanup.size(); ++i)
            {
                std::string cleanupStep = currentOutputTypeInfo->Cleanup.at(i);
                
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
                                                        runcpp2ScriptDir) || 
                    cleanupResult != 0)
                {
                    ssLOG_ERROR("Cleanup command \"" << cleanupStep << "\" failed");
                    ssLOG_ERROR("Failed with result " << cleanupResult);
                    ssLOG_ERROR("Failed with output: " << cleanupOutput);
                    return false;
                }
            }
        }
        
        return true;
    }

    bool LinkScript(const std::string& scriptPath, 
                    const runcpp2::Data::ScriptInfo& scriptInfo,
                    const runcpp2::Data::Profile& profile,
                    const std::string& scriptObjectFilePath,
                    const std::vector<std::string>& copiedDependenciesBinariesPaths,
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
            
            //Add link flags for the dependencies
            for(int i = 0; i < scriptInfo.Dependencies.size(); ++i)
            {
                if(!runcpp2::IsDependencyAvailableForThisPlatform(scriptInfo.Dependencies.at(i)))
                    continue;
                
                std::string targetProfileName;
                
                //Check for profile name first
                if( scriptInfo.Dependencies.at(i).LinkProperties.find(profile.Name) != 
                    scriptInfo.Dependencies.at(i).LinkProperties.end())
                {
                    targetProfileName = profile.Name;
                }
                else
                {
                    //If not check for profile name aliases
                    for(const auto& alias : profile.NameAliases)
                    {
                        if( scriptInfo.Dependencies.at(i).LinkProperties.find(alias) != 
                            scriptInfo.Dependencies.at(i).LinkProperties.end())
                        {
                            targetProfileName = alias;
                            break;
                        }
                    }
                    
                    //Final check for "All"
                    if( targetProfileName.empty() &&  
                        scriptInfo.Dependencies.at(i).LinkProperties.find("All") != 
                        scriptInfo.Dependencies.at(i).LinkProperties.end())
                    {
                        targetProfileName = "All";
                    }
                }
                
                if(targetProfileName.empty())
                    continue;
                
                const runcpp2::Data::DependencyLinkProperty& currentLinkProperty = 
                    scriptInfo.Dependencies.at(i).LinkProperties.at(targetProfileName);
                
                if(runcpp2::HasValueFromPlatformMap(currentLinkProperty.AdditionalLinkOptions))
                {
                    const std::vector<std::string> additionalLinkOptions = 
                        *runcpp2::GetValueFromPlatformMap(currentLinkProperty.AdditionalLinkOptions);
                    
                    for(int k = 0; k < additionalLinkOptions.size(); ++k)
                        linkFlags += " " + additionalLinkOptions.at(k);
                }
            }
            
            substitutionMap["{LinkFlags}"] = {linkFlags};
        }
        
        std::string runcpp2ScriptDir;
        //Output File
        {
            const std::string scriptName = ghc::filesystem::path(scriptPath).stem().string();
            const std::string sharedLibPrefix =
                *runcpp2::GetValueFromPlatformMap(profile.FilesTypes.SharedLinkFile.Prefix);
            const std::string sharedLibExtension =
                    *runcpp2::GetValueFromPlatformMap(profile.FilesTypes.SharedLibraryFile.Extension);
            const std::string scriptDirectory = 
                ghc::filesystem::path(scriptPath).parent_path().string();
            
            runcpp2ScriptDir = runcpp2::ProcessPath(scriptDirectory + "/.runcpp2");
            std::string outputFile =    linkAsExecutable ?
                                        scriptName + exeExt :
                                        sharedLibPrefix + scriptName + sharedLibExtension;
            
            outputFile = runcpp2::ProcessPath(runcpp2ScriptDir + "/" + outputFile);
        
            substitutionMap["{OutputFileName}"] = {scriptName};
            substitutionMap["{OutputFileExtension}"] = 
            {
                (linkAsExecutable ? exeExt : sharedLibExtension)
            };
            substitutionMap["{OutputFileDirectory}"] = {runcpp2ScriptDir};
            substitutionMap["{OutputFilePath}"] = {outputFile};
        }
        //Link Files
        {
            auto scriptParsedPath = ghc::filesystem::path(scriptObjectFilePath);
            std::string scriptObjDirectory = scriptParsedPath.parent_path().string();
            std::string scriptObjName = scriptParsedPath.stem().string();
            std::string scriptObjExt = scriptParsedPath.extension().string();
            
            substitutionMap["{LinkFileName}"].push_back(scriptObjName);
            substitutionMap["{LinkFileExt}"].push_back(scriptObjExt);
            substitutionMap["{LinkFileDirectory}"].push_back(scriptObjDirectory);
            substitutionMap["{LinkFilePath}"].push_back(scriptObjectFilePath);
            
            substitutionMap["{LinkObjectFileName}"].push_back(scriptObjName);
            substitutionMap["{LinkObjectFileExt}"].push_back(scriptObjExt);
            substitutionMap["{LinkObjectFileDirectory}"].push_back(scriptObjDirectory);
            substitutionMap["{LinkObjectFilePath}"].push_back(scriptObjectFilePath);
        
            for(int i = 0; i < copiedDependenciesBinariesPaths.size(); ++i)
            {
                size_t extensionFoundIndex = copiedDependenciesBinariesPaths.at(i).find_last_of(".");
                
                //Check if this is a file we can link
                std::string extension;
                if(extensionFoundIndex != std::string::npos)
                    extension = copiedDependenciesBinariesPaths.at(i).substr(extensionFoundIndex);
                
                using namespace runcpp2;
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
                
                processLinkFile:;
                if(currentLinkType == Data::DependencyLibraryType::COUNT)
                {
                    ssLOG_DEBUG("Skip linking " << copiedDependenciesBinariesPaths.at(i));
                    continue;
                }
                
                auto depLinkParsedPath = ghc::filesystem::path(copiedDependenciesBinariesPaths.at(i));
                std::string depLinkDirectory = depLinkParsedPath.parent_path().string();
                std::string depLinkName = depLinkParsedPath.stem().string();
                std::string depLinkExt = depLinkParsedPath.extension().string();
                
                substitutionMap["{LinkFileName}"].push_back(depLinkName);
                substitutionMap["{LinkFileExt}"].push_back(depLinkExt);
                substitutionMap["{LinkFileDirectory}"].push_back(depLinkDirectory);
                substitutionMap["{LinkFilePath}"].push_back(copiedDependenciesBinariesPaths.at(i));
                
                static_assert(  static_cast<int>(Data::DependencyLibraryType::COUNT) == 4, 
                                "Add new type to be processed");
                switch(currentLinkType)
                {
                    case Data::DependencyLibraryType::STATIC:
                    {
                        substitutionMap["{LinkStaticFileName}"].push_back(depLinkName);
                        substitutionMap["{LinkStaticFileExt}"].push_back(depLinkExt);
                        substitutionMap["{LinkStaticFileDirectory}"].push_back(depLinkDirectory);
                        substitutionMap["{LinkStaticFilePath}"].push_back(copiedDependenciesBinariesPaths.at(i));
                        break;
                    }
                    case Data::DependencyLibraryType::SHARED:
                    {
                        substitutionMap["{LinkSharedFileName}"].push_back(depLinkName);
                        substitutionMap["{LinkSharedFileExt}"].push_back(depLinkExt);
                        substitutionMap["{LinkSharedFileDirectory}"].push_back(depLinkDirectory);
                        substitutionMap["{LinkSharedFilePath}"].push_back(copiedDependenciesBinariesPaths.at(i));
                        break;
                    }
                    case Data::DependencyLibraryType::OBJECT:
                    {
                        substitutionMap["{LinkObjectFileName}"].push_back(depLinkName);
                        substitutionMap["{LinkObjectFileExt}"].push_back(depLinkExt);
                        substitutionMap["{LinkObjectFileDirectory}"].push_back(depLinkDirectory);
                        substitutionMap["{LinkObjectFilePath}"].push_back(copiedDependenciesBinariesPaths.at(i));
                        break;
                    }
                    case Data::DependencyLibraryType::HEADER:
                    case Data::DependencyLibraryType::COUNT:
                    {
                        ssLOG_WARNING(  "Unexpected currentLinkType: " << 
                                        static_cast<int>(currentLinkType) << 
                                        " for " << copiedDependenciesBinariesPaths.at(i));
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
                                                        runcpp2ScriptDir) || 
                    setupResult != 0)
                {
                    ssLOG_ERROR("Setup command \"" << setupStep << "\" failed");
                    ssLOG_ERROR("Failed with result " << setupResult);
                    ssLOG_ERROR("Failed with output: " << setupOutput);
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
                
                ssLOG_INFO("running link command: " << linkCommand << " in " << runcpp2ScriptDir);
                std::string linkOutput;
                int resultCode = 0;
                if( !runcpp2::RunCommandAndGetOutput(   linkCommand, 
                                                        linkOutput, 
                                                        resultCode, 
                                                        runcpp2ScriptDir) ||
                    resultCode != 0)
                {
                    ssLOG_ERROR("Link command failed with result " << resultCode);
                    ssLOG_ERROR("Link output: " << linkOutput);
                    return false;
                }
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
                                                        runcpp2ScriptDir) || 
                    cleanupResult != 0)
                {
                    ssLOG_ERROR("Cleanup command \"" << cleanupStep << "\" failed");
                    ssLOG_ERROR("Failed with result " << cleanupResult);
                    ssLOG_ERROR("Failed with output: " << cleanupOutput);
                    return false;
                }
            }
        }
        
        return true;
    }
    
    bool RunGlobalSteps(const std::string& scriptPath,
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
        
        std::string scriptDirectory = ghc::filesystem::path(scriptPath) .parent_path()
                                                                        .string();
        std::string runcpp2ScriptDir = runcpp2::ProcessPath(scriptDirectory + "/.runcpp2");
        const std::vector<std::string>& steps = *runcpp2::GetValueFromPlatformMap(platformSteps);
        
        for(int i = 0; i < steps.size(); ++i)
        {
            std::string commandOutput;
            int commandResult = 0;
            
            if( !runcpp2::RunCommandAndGetOutput(   steps.at(i), 
                                                    commandOutput,
                                                    commandResult,
                                                    runcpp2ScriptDir) ||
                commandResult != 0)
            {
                ssLOG_ERROR("Command \"" << steps.at(i) << "\" failed");
                ssLOG_ERROR("Failed with result " << commandResult);
                ssLOG_ERROR("Failed with output: " << commandOutput);
                return false;
            }
        }
        
        return true;
    }
}

bool runcpp2::CompileAndLinkScript( const std::string& scriptPath, 
                                    const Data::ScriptInfo& scriptInfo,
                                    const Data::Profile& profile,
                                    const std::vector<std::string>& copiedDependenciesBinariesPaths,
                                    bool buildExecutable,
                                    const std::string exeExt)
{
    if(!RunGlobalSteps(scriptPath, profile.Setup))
    {
        ssLOG_ERROR("Failed to run profile global setup steps");
        return false;
    }
    
    std::string scriptObjectFilePath;

    if(!CompileScript(scriptPath, scriptInfo, profile, buildExecutable, scriptObjectFilePath))
    {
        ssLOG_ERROR("CompileScript failed");
        return false;
    }
    
    if(!LinkScript( scriptPath, 
                    scriptInfo,
                    profile,
                    scriptObjectFilePath,
                    copiedDependenciesBinariesPaths,
                    buildExecutable,
                    exeExt))
    {
        ssLOG_ERROR("LinkScript failed");
        return false;
    }
    
    if(!RunGlobalSteps(scriptPath, profile.Cleanup))
    {
        ssLOG_ERROR("Failed to run profile global cleanup steps");
        return false;
    }
    
    return true;
}
