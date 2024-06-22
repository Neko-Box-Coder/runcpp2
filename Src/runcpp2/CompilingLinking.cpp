#include "runcpp2/CompilingLinking.hpp"
#include "System2.h"
#include "ghc/filesystem.hpp"
#include "runcpp2/DependenciesSetupHelper.hpp"
#include "runcpp2/PlatformUtil.hpp"
#include "runcpp2/StringUtil.hpp"
#include "ssLogger/ssLog.hpp"

namespace
{
    bool CompileScript( const std::string& scriptPath, 
                        const runcpp2::Data::ScriptInfo& scriptInfo,
                        const runcpp2::Data::Profile& profile,
                        bool compileAsExecutable,
                        std::string& outScriptObjectFilePath)
    {
        ssLOG_FUNC_DEBUG();
        
        std::string scriptDirectory = ghc::filesystem::path(scriptPath).parent_path().string();
        std::string scriptName = ghc::filesystem::path(scriptPath).stem().string();
        std::string runcpp2ScriptDir = runcpp2::ProcessPath(scriptDirectory + "/.runcpp2");
        std::string compileCommand;
        
        //Getting EnvironmentSetup command
        {
            compileCommand = 
                runcpp2::HasValueFromPlatformMap(profile.Compiler.EnvironmentSetup) ?
                *runcpp2::GetValueFromPlatformMap(profile.Compiler.EnvironmentSetup) : "";
            
            if(!compileCommand.empty())
                compileCommand += " && ";
        }
        
        compileCommand +=   profile.Compiler.Executable + " " + 
                            profile.Compiler.CompileArgs.CompilePart;

        //Replace for {CompileFlags} for compile part
        const std::string compileFlagSubstitution = "{CompileFlags}";
        std::size_t foundIndex = compileCommand.find(compileFlagSubstitution);
        if(foundIndex == std::string::npos)
        {
            ssLOG_ERROR("'" + compileFlagSubstitution + "' missing in CompileArgs.CompilePart");
            return false;
        }
        
        std::string compileArgs = 
            runcpp2::HasValueFromPlatformMap(profile.Compiler.DefaultCompileFlags) ? 
            *runcpp2::GetValueFromPlatformMap(profile.Compiler.DefaultCompileFlags) : "";
        
        std::string additionalCompileFlags = "";
        
        if(compileAsExecutable)
        {
            if(runcpp2::HasValueFromPlatformMap(profile.Compiler.ExecutableCompileFlags))
            {
                additionalCompileFlags = 
                    *runcpp2::GetValueFromPlatformMap(profile.Compiler.ExecutableCompileFlags);
            }
        }
        else
        {
            if(runcpp2::HasValueFromPlatformMap(profile.Compiler.SharedLibCompileFlags))
            {
                additionalCompileFlags = 
                    *runcpp2::GetValueFromPlatformMap(profile.Compiler.SharedLibCompileFlags);
            }
        }
        
        compileArgs +=  (compileArgs.empty() ? "" : " ") + additionalCompileFlags;
        
        //Override the compile flags from the script info
        {
            if(runcpp2::HasValueFromPlatformMap(scriptInfo.OverrideCompileFlags))
            {
                using namespace runcpp2::Data;
                
                const std::unordered_map<ProfileName, FlagsOverrideInfo>& 
                currentOverrideCompileFlags = 
                    runcpp2::GetValueFromPlatformMap(scriptInfo.OverrideCompileFlags)->FlagsOverrides;
                
                if( currentOverrideCompileFlags.find(profile.Name) != 
                    currentOverrideCompileFlags.end())
                {
                    std::vector<std::string> compileArgsToRemove; 
                    runcpp2::SplitString(   currentOverrideCompileFlags.at(profile.Name).Remove, 
                                            " ", 
                                            compileArgsToRemove);
                    
                    for(int i = 0; i < compileArgsToRemove.size(); ++i)
                    {
                        const std::string currentArgToRemove = compileArgsToRemove.at(i);
                        std::size_t foundIndex = compileArgs.find(currentArgToRemove);
                        
                        if(foundIndex != std::string::npos)
                        {
                            if(foundIndex + currentArgToRemove.size() + 1 <= compileArgs.size())
                                compileArgs.erase(foundIndex, currentArgToRemove.size() + 1);
                            else
                                compileArgs.erase(foundIndex, currentArgToRemove.size());
                            
                            continue;
                        }
                        
                        ssLOG_WARNING(  "Compile flag to remove not found: " << currentArgToRemove);
                    }
                    
                    runcpp2::TrimRight(compileArgs);
                    compileArgs += " " + currentOverrideCompileFlags.at(profile.Name).Append;
                }
            }
        }
        
        compileCommand.replace( foundIndex, 
                                compileFlagSubstitution.size(), 
                                compileArgs);
        
        //Add include paths for all the dependencies
        //Replace {IncludeDirectoryPath}
        const std::string includePathSubstitution = "{IncludeDirectoryPath}";
        foundIndex = profile.Compiler.CompileArgs.IncludePart.find(includePathSubstitution);
        if(foundIndex == std::string::npos)
        {
            ssLOG_ERROR("'" + includePathSubstitution + "' missing in CompileArgs");
            return false;
        }
        
        for(int i = 0; i < scriptInfo.Dependencies.size(); ++i)
        {
            if(!runcpp2::IsDependencyAvailableForThisPlatform(scriptInfo.Dependencies.at(i)))
                continue;
        
            for(int j = 0; j < scriptInfo.Dependencies[i].AbsoluteIncludePaths.size(); ++j)
            {
                std::string currentIncludePart = profile.Compiler.CompileArgs.IncludePart;
                currentIncludePart.replace( foundIndex, 
                                            includePathSubstitution.size(), 
                                            scriptInfo.Dependencies[i].AbsoluteIncludePaths[j]);
                
                compileCommand += " " + currentIncludePart;
            }
        }
        
        //Replace {InputFilePath} for input part
        const std::string inputFileSubstitution = "{InputFilePath}";
        compileCommand += " " + profile.Compiler.CompileArgs.InputPart;
        foundIndex = compileCommand.find(inputFileSubstitution);
        if(foundIndex == std::string::npos)
        {
            ssLOG_ERROR("'" + inputFileSubstitution + "' missing in CompileArgs");
            return false;
        }
        compileCommand.replace(foundIndex, inputFileSubstitution.size(), scriptPath);
        
        //Replace {ObjectFilePath} for output part
        const std::string objectFileSubstitution = "{ObjectFilePath}";
        std::string objectFileExt = 
            *runcpp2::GetValueFromPlatformMap(profile.ObjectLinkFile.Extension);
        
        if(objectFileExt.empty())
            ssLOG_WARNING("Object file extension is empty");
        
        compileCommand += " " + profile.Compiler.CompileArgs.OutputPart;
        foundIndex = compileCommand.find(objectFileSubstitution);
        if(foundIndex == std::string::npos)
        {
            ssLOG_ERROR("'" + objectFileSubstitution + "' missing in CompileArgs");
            return false;
        }
        
        std::string objectFilePath = runcpp2::ProcessPath(  runcpp2ScriptDir + "/" + 
                                                            scriptName + objectFileExt);
        
        compileCommand.replace(foundIndex, objectFileSubstitution.size(), objectFilePath);
        
        //Compile the script
        ssLOG_INFO("running compile command: " << compileCommand);
        
        System2CommandInfo compileCommandInfo = {};
        compileCommandInfo.RedirectOutput = true;
        SYSTEM2_RESULT result = System2Run(compileCommand.c_str(), &compileCommandInfo);
        
        if(result != SYSTEM2_RESULT_SUCCESS)
        {
            ssLOG_ERROR("System2Run failed with result: " << result);
            return false;
        }
        
        std::vector<char> output;
        
        do
        {
            uint32_t byteRead = 0;
            output.resize(output.size() + 4096);
            
            result = System2ReadFromOutput( &compileCommandInfo, 
                                            output.data() + output.size() - 4096, 
                                            4096 - 1, 
                                            &byteRead);

            output.resize(output.size() - 4096 + byteRead + 1);
            output.back() = '\0';
        }
        while(result == SYSTEM2_RESULT_READ_NOT_FINISHED);
        
        if(result != SYSTEM2_RESULT_SUCCESS)
        {
            ssLOG_ERROR("Failed to read from output with result: " << result);
            return false;
        }
        
        ssLOG_DEBUG("Compile Output: \n" << output.data());
        
        int statusCode = 0;
        result = System2GetCommandReturnValueSync(&compileCommandInfo, &statusCode);
        
        if(result != SYSTEM2_RESULT_SUCCESS)
        {
            ssLOG_ERROR("System2GetCommandReturnValueSync failed with result: " << result);
            return false;
        }
        
        if(statusCode != 0)
        {
            ssLOG_ERROR("Compile command returned with non-zero status code: " << statusCode);
            ssLOG_BASE(output.data());
            return false;
        }
        
        outScriptObjectFilePath = objectFilePath;
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
        
        std::string scriptName = ghc::filesystem::path(scriptPath).stem().string();
        std::string linkCommand;
        
        //Getting EnvironmentSetup command
        {
            linkCommand =   runcpp2::HasValueFromPlatformMap(profile.Linker.EnvironmentSetup) ? 
                            *runcpp2::GetValueFromPlatformMap(profile.Linker.EnvironmentSetup) : "";
            
            if(!linkCommand.empty())
                linkCommand += " && ";
        }
        
        linkCommand += profile.Linker.Executable + " ";
        std::string linkFlags = 
            runcpp2::HasValueFromPlatformMap(profile.Linker.DefaultLinkFlags) ? 
            *runcpp2::GetValueFromPlatformMap(profile.Linker.DefaultLinkFlags) : "";
        
        std::string outputFile;
        
        //Populate link flags and output file depending on link type
        {
            std::string additionalLinkFlags = "";
            
            if(linkAsExecutable)
            {
                if(!runcpp2::HasValueFromPlatformMap(profile.Linker.ExecutableLinkFlags))
                {
                    ssLOG_ERROR("No Link flags for executable");
                    return false;
                }
                
                additionalLinkFlags = 
                    *runcpp2::GetValueFromPlatformMap(profile.Linker.ExecutableLinkFlags);
            }
            else
            {
                if(!runcpp2::HasValueFromPlatformMap(profile.Linker.SharedLibLinkFlags))
                {
                    ssLOG_ERROR("No Link flags for shared library");
                    return false;
                }
                
                additionalLinkFlags = 
                    *runcpp2::GetValueFromPlatformMap(profile.Linker.SharedLibLinkFlags);
            }
            
            linkFlags += (linkFlags.empty() ? "" : " ") + additionalLinkFlags;
            
            const std::string sharedLibPrefix = 
                *runcpp2::GetValueFromPlatformMap(profile.SharedLinkFile.Prefix);
            
            const std::string sharedLibExtension =
                *runcpp2::GetValueFromPlatformMap(profile.SharedLinkFile.Extension);

            outputFile =    linkAsExecutable ? 
                            scriptName + exeExt : 
                            sharedLibPrefix + scriptName + sharedLibExtension;
        }
        
        //Override the default link flags from the script info
        {
            if(runcpp2::HasValueFromPlatformMap(scriptInfo.OverrideLinkFlags))
            {
                using namespace runcpp2::Data;
                
                const std::unordered_map<ProfileName, FlagsOverrideInfo>& 
                currentOverrideLinkFlags =  
                    runcpp2::GetValueFromPlatformMap(scriptInfo.OverrideLinkFlags)->FlagsOverrides;

                if(currentOverrideLinkFlags.find(profile.Name) != currentOverrideLinkFlags.end())
                {
                    std::vector<std::string> linkFlagsToRemove;
                    
                    runcpp2::SplitString(   currentOverrideLinkFlags.at(profile.Name).Remove, 
                                            " ", 
                                            linkFlagsToRemove);
                    
                    for(int i = 0; i < linkFlagsToRemove.size(); ++i)
                    {
                        const std::string currentArgToRemove = linkFlagsToRemove.at(i);
                        std::size_t foundIndex = linkFlags.find(linkFlagsToRemove.at(i));
                        
                        if(foundIndex != std::string::npos)
                        {
                            if(foundIndex + currentArgToRemove.size() + 1 <= linkFlags.size())
                                linkFlags.erase(foundIndex, currentArgToRemove.size() + 1);
                            else
                                linkFlags.erase(foundIndex, currentArgToRemove.size());
                            
                            continue;
                        }
                        
                        ssLOG_WARNING("Link flag to remove not found: " << currentArgToRemove);
                    }
                    
                    runcpp2::TrimRight(linkFlags);
                    linkFlags += " " + currentOverrideLinkFlags.at(profile.Name).Append;
                }
            }
        }
        
        //Add link flags for the dependencies
        {
            for(int i = 0; i < scriptInfo.Dependencies.size(); ++i)
            {
                if(!runcpp2::IsDependencyAvailableForThisPlatform(scriptInfo.Dependencies.at(i)))
                    continue;
                
                if( scriptInfo.Dependencies[i].LinkProperties.find(profile.Name) == 
                    scriptInfo.Dependencies[i].LinkProperties.end())
                {
                    continue;
                }
                
                const runcpp2::Data::DependencyLinkProperty currentLinkProperty = 
                    scriptInfo.Dependencies[i].LinkProperties.at(profile.Name);
                
                if(runcpp2::HasValueFromPlatformMap(currentLinkProperty.AdditionalLinkOptions))
                {
                    const std::vector<std::string> additionalLinkOptions = 
                        *runcpp2::GetValueFromPlatformMap(currentLinkProperty.AdditionalLinkOptions);
                    
                    for(int k = 0; k < additionalLinkOptions.size(); ++k)
                        linkFlags += " " + additionalLinkOptions[k];
                }
            }
        }
        
        std::string currentOutputPart = profile.Linker.LinkArgs.OutputPart;
        
        //Replace for {LinkFlags} for output part
        {
            const std::string linkFlagsSubstitution = "{LinkFlags}";
            std::size_t foundIndex = currentOutputPart.find(linkFlagsSubstitution);
            if(foundIndex == std::string::npos)
            {
                ssLOG_ERROR("'" + linkFlagsSubstitution + "' missing in LinkerArgs");
                return false;
            }
            currentOutputPart.replace(foundIndex, linkFlagsSubstitution.size(), linkFlags);
        }
        
        //Replace {OutputFile} for output part
        {
            const std::string outputFileSubstitution = "{OutputFilePath}";
            std::size_t foundIndex = currentOutputPart.find(outputFileSubstitution);
            if(foundIndex == std::string::npos)
            {
                ssLOG_ERROR("'" + outputFileSubstitution + "' missing in LinkerArgs");
                return false;
            }
            currentOutputPart.replace(foundIndex, outputFileSubstitution.size(), outputFile);
            
            runcpp2::Trim(currentOutputPart);
            linkCommand += currentOutputPart;
        }
        
        //Replace {LinkFilePath} for LinkPart part
        {
            const std::string linkPart = profile.Linker.LinkArgs.LinkPart;
            const std::string linkSubstitution = "{LinkFilePath}";
            
            std::size_t foundIndex = linkPart.find(linkSubstitution);
            if(foundIndex == std::string::npos)
            {
                ssLOG_ERROR("'" + linkSubstitution + "' missing in LinkerArgs");
                return false;
            }
        
            //LinkPart for script object file
            std::string currentLinkPart = linkPart;
            currentLinkPart.replace(foundIndex, linkSubstitution.size(), scriptObjectFilePath);
            runcpp2::Trim(currentLinkPart);
            linkCommand += " " + currentLinkPart;
            
            //LinkPart for dependencies
            for(int i = 0; i < copiedDependenciesBinariesPaths.size(); ++i)
            {
                size_t extensionFoundIndex = copiedDependenciesBinariesPaths[i].find_last_of(".");
                
                //Check if this is a file we can link
                std::string extension;
                if(extensionFoundIndex != std::string::npos)
                    extension = copiedDependenciesBinariesPaths[i].substr(extensionFoundIndex);
                
                using namespace runcpp2;
                
                if(!linkAsExecutable)
                {
                    if(!HasValueFromPlatformMap(profile.SharedLinkFile.Extension))
                    {
                        ssLOG_WARNING(  "profile " << profile.Name << " missing extension for " <<
                                        "shared link file");
                        
                        continue;
                    }
                    
                    if( extension != *GetValueFromPlatformMap(profile.SharedLinkFile.Extension))
                        continue;
                }
                else
                {
                    if(!HasValueFromPlatformMap(profile.SharedLinkFile.Extension))
                    {
                        ssLOG_WARNING(  "profile " << profile.Name << " missing extension for " <<
                                        "shared link file");
                        
                        continue;
                    }
                    
                    if(!HasValueFromPlatformMap(profile.StaticLinkFile.Extension))
                    {
                        ssLOG_WARNING(  "profile " << profile.Name << " missing extension for " <<
                                        "static link file");
                        
                        continue;
                    }
                    
                    if( extension != 
                        *runcpp2::GetValueFromPlatformMap(profile.SharedLinkFile.Extension) &&
                        extension != 
                        *runcpp2::GetValueFromPlatformMap(profile.StaticLinkFile.Extension))
                    {
                        continue;
                    }
                }
                
                std::string currentLinkPart = linkPart;
                currentLinkPart.replace(foundIndex, 
                                        linkSubstitution.size(), 
                                        copiedDependenciesBinariesPaths[i]);
                
                runcpp2::Trim(currentLinkPart);
                linkCommand += " " + currentLinkPart;
            }
        }
        
        std::string scriptDirectory = ghc::filesystem::path(scriptPath).parent_path().string();
        std::string runcpp2ScriptDir = runcpp2::ProcessPath(scriptDirectory + "/.runcpp2");
        
        //Do Linking
        System2CommandInfo linkCommandInfo = {};
        linkCommandInfo.RedirectOutput = true;
        linkCommandInfo.RunDirectory = runcpp2ScriptDir.c_str();
        SYSTEM2_RESULT result = System2Run(linkCommand.c_str(), &linkCommandInfo);
        
        ssLOG_INFO("running link command: " << linkCommand);
        ssLOG_INFO("in " << runcpp2ScriptDir);
        
        if(result != SYSTEM2_RESULT_SUCCESS)
        {
            ssLOG_ERROR("System2Run failed with result: " << result);
            return false;
        }
        
        std::string output;
        do
        {
            uint32_t byteRead = 0;
            char tempBuffer[4096];
            
            result = System2ReadFromOutput( &linkCommandInfo, 
                                            tempBuffer, 
                                            4096 - 1, 
                                            &byteRead);
            
            tempBuffer[byteRead] = '\0';
            output += tempBuffer;
        }
        while(result == SYSTEM2_RESULT_READ_NOT_FINISHED);
        
        if(result != SYSTEM2_RESULT_SUCCESS)
        {
            ssLOG_ERROR("Failed to read from output with result: " << result);
            return false;
        }
        
        ssLOG_DEBUG("Link Output: \n" << output.data());
        
        int statusCode = 0;
        result = System2GetCommandReturnValueSync(&linkCommandInfo, &statusCode);
        
        if(result != SYSTEM2_RESULT_SUCCESS)
        {
            ssLOG_ERROR("System2GetCommandReturnValueSync failed with result: " << result);
            return false;
        }
        
        if(statusCode != 0)
        {
            ssLOG_ERROR("Link command returned with non-zero status code: " << statusCode);
            ssLOG_BASE(output.data());
            return false;
        }
        
        return true;
    }
    
    bool RunSetupSteps( const std::string& scriptPath,
                        const runcpp2::Data::Profile& profile)
    {
        if(!profile.SetupSteps.empty())
        {
            if( runcpp2::HasValueFromPlatformMap(profile.SetupSteps) &&
                !runcpp2::GetValueFromPlatformMap(profile.SetupSteps)->empty())
            {
                std::string scriptDirectory = ghc::filesystem::path(scriptPath) .parent_path()
                                                                                .string();
                
                std::string runcpp2ScriptDir = runcpp2::ProcessPath(scriptDirectory + "/.runcpp2");
                std::string setupCommand;
                
                const std::vector<std::string>& currentSetupSteps =
                    *runcpp2::GetValueFromPlatformMap(profile.SetupSteps);
                
                for(int i = 0; i < currentSetupSteps.size(); ++i)
                {
                    setupCommand += currentSetupSteps.at(i);
                    if(i != currentSetupSteps.size() - 1)
                        setupCommand += " && ";
                }
                
                System2CommandInfo setupCommandInfo = {};
                setupCommandInfo.RunDirectory = runcpp2ScriptDir.c_str();
                setupCommandInfo.RedirectOutput = true;
                
                ssLOG_INFO("running setup command: " << setupCommand);
                ssLOG_INFO("in " << runcpp2ScriptDir);
                SYSTEM2_RESULT result = System2Run(setupCommand.c_str(), &setupCommandInfo);
                
                if(result != SYSTEM2_RESULT_SUCCESS)
                {
                    ssLOG_ERROR("System2Run failed with result: " << result);
                    return false;
                }
                
                std::vector<char> output;
                
                do
                {
                    uint32_t byteRead = 0;
                    output.resize(output.size() + 4096);
                    
                    result = System2ReadFromOutput( &setupCommandInfo, 
                                                    output.data() + output.size() - 4096, 
                                                    4096 - 1, 
                                                    &byteRead);
                                                    
                    output.resize(output.size() - 4096 + byteRead + 1);
                    output.back() = '\0';
                }
                while(result == SYSTEM2_RESULT_READ_NOT_FINISHED);
                
                if(result != SYSTEM2_RESULT_SUCCESS)
                {
                    ssLOG_ERROR("Failed to read from output with result: " << result);
                    return false;
                }
                
                ssLOG_DEBUG("Setup Output: \n" << output.data());
                
                int statusCode = 0;
                
                result = System2GetCommandReturnValueSync(&setupCommandInfo, &statusCode);
                
                if(result != SYSTEM2_RESULT_SUCCESS)
                {
                    ssLOG_ERROR("System2GetCommandReturnValueSync failed with result: " << result);
                    return false;
                }
                
                if(statusCode != 0)
                {
                    ssLOG_ERROR("Setup command returned with non-zero status code: " << statusCode);
                    ssLOG_BASE(output.data());
                    return false;
                }
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
    if(!RunSetupSteps(scriptPath, profile))
    {
        ssLOG_ERROR("RunSetupSteps failed");
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
    
    return true;
}
