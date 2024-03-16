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
                        const runcpp2::Data::CompilerProfile& profile,
                        std::string& outScriptObjectFilePath)
    {
        std::string scriptDirectory = ghc::filesystem::path(scriptPath).parent_path().string();
        std::string scriptName = ghc::filesystem::path(scriptPath).stem().string();
        std::string runcpp2ScriptDir = runcpp2::ProcessPath(scriptDirectory + "/.runcpp2");
        std::vector<std::string> platformNames = runcpp2::GetPlatformNames();
        
        std::string compileCommand =    profile.Compiler.Executable + " " + 
                                        profile.Compiler.CompileArgs.CompilePart;

        //Replace for {CompileFlags} for compile part
        const std::string compileFlagSubstitution = "{CompileFlags}";
        std::size_t foundIndex = compileCommand.find(compileFlagSubstitution);
        std::string compileArgs = profile.Compiler.DefaultCompileFlags;
        if(foundIndex == std::string::npos)
        {
            ssLOG_ERROR("'" + compileFlagSubstitution + "' missing in CompileArgs.CompilePart");
            return false;
        }
        
        //Override the compile flags from the script info
        if(scriptInfo.OverrideCompileFlags.find(profile.Name) != scriptInfo.OverrideCompileFlags.end())
        {
            std::vector<std::string> compileArgsToRemove; 
            runcpp2::SplitString(   scriptInfo.OverrideCompileFlags.at(profile.Name).Remove, 
                                    " ", 
                                    compileArgsToRemove);
            
            for(int i = 0; i < compileArgsToRemove.size(); ++i)
            {
                std::size_t foundIndex = compileArgs.find(compileArgsToRemove.at(i));
                
                if(foundIndex != std::string::npos)
                {
                    compileArgs.erase(foundIndex, compileArgsToRemove.at(i).size() + 1);
                    continue;
                }
                
                ssLOG_WARNING("Compile flag to remove not found: " << compileArgsToRemove.at(i));
            }
            
            runcpp2::TrimRight(compileArgs);
            compileArgs += " " + scriptInfo.OverrideCompileFlags.at(profile.Name).Append;
        }
        
        compileCommand.replace( foundIndex, 
                                compileFlagSubstitution.size(), 
                                compileArgs);
        
        //Add include paths for all the dependencies
        //Replace {IncludePath}
        const std::string includePathSubstitution = "{IncludePath}";
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
        
        //Replace {InputFile} for input part
        const std::string inputFileSubstitution = "{InputFile}";
        compileCommand += " " + profile.Compiler.CompileArgs.InputPart;
        foundIndex = compileCommand.find(inputFileSubstitution);
        if(foundIndex == std::string::npos)
        {
            ssLOG_ERROR("'" + inputFileSubstitution + "' missing in CompileArgs");
            return false;
        }
        compileCommand.replace(foundIndex, inputFileSubstitution.size(), scriptPath);
        
        //Replace {ObjectFile} for output part
        const std::string objectFileSubstitution = "{ObjectFile}";
        std::string objectFileExt; 
        
        for(int i = 0; i < platformNames.size(); ++i)
        {
            if(profile.ObjectFileExtensions.find(platformNames.at(i)) != profile.ObjectFileExtensions.end())
            {
                objectFileExt = profile.ObjectFileExtensions.at(platformNames.at(i));
                break;
            }
        }
        
        compileCommand += " " + profile.Compiler.CompileArgs.OutputPart;
        foundIndex = compileCommand.find(objectFileSubstitution);
        if(foundIndex == std::string::npos)
        {
            ssLOG_ERROR("'" + objectFileSubstitution + "' missing in CompileArgs");
            return false;
        }
        
        std::string objectFilePath = runcpp2::ProcessPath(  runcpp2ScriptDir + "/" + 
                                                            scriptName + "." + objectFileExt);
        
        compileCommand.replace(foundIndex, objectFileSubstitution.size(), objectFilePath);
        
        //Compile the script
        ssLOG_INFO("running compile command: " << compileCommand);
        
        System2CommandInfo compileCommandInfo;
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

            output.resize(output.size() - 4096+ byteRead + 1);
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
                    const runcpp2::Data::CompilerProfile& profile,
                    const std::string& scriptObjectFilePath,
                    const std::vector<std::string>& copiedDependenciesBinariesPaths)
    {
        std::string scriptName = ghc::filesystem::path(scriptPath).stem().string();
        
        //Link the script to the dependencies
        std::string linkCommand = profile.Linker.Executable + " ";
        std::string currentOutputPart = profile.Linker.LinkerArgs.OutputPart;
        const std::string linkFlagsSubstitution = "{LinkFlags}";
        const std::string outputFileSubstitution = "{OutputFile}";
        const std::string objectFileSubstitution = "{ObjectFile}";
        
        std::string linkFlags = profile.Linker.DefaultLinkFlags;
        std::string outputName = scriptName;
        
        //Override the default link flags from the script info
        if(scriptInfo.OverrideLinkFlags.find(profile.Name) != scriptInfo.OverrideLinkFlags.end())
        {
            std::vector<std::string> linkFlagsToRemove;
            
            runcpp2::SplitString(  scriptInfo.OverrideLinkFlags.at(profile.Name).Remove, 
                                    " ", 
                                    linkFlagsToRemove);
            
            for(int i = 0; i < linkFlagsToRemove.size(); ++i)
            {
                std::size_t foundIndex = linkFlags.find(linkFlagsToRemove.at(i));
                
                if(foundIndex != std::string::npos)
                {
                    linkFlags.erase(foundIndex, linkFlagsToRemove.at(i).size() + 1);
                    continue;
                }
                
                ssLOG_WARNING("Link flag to remove not found: " << linkFlagsToRemove.at(i));
            }
            
            runcpp2::TrimRight(linkFlags);
            linkFlags += " " + scriptInfo.OverrideLinkFlags.at(profile.Name).Append;
        }
        
        //Add linker flags for the dependencies
        {
            std::vector<std::string> platformNames = runcpp2::GetPlatformNames();
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
                
                for(int j = 0; j < platformNames.size(); ++j)
                {
                    if( currentLinkProperty.AdditionalLinkOptions.find(platformNames.at(j)) ==
                        currentLinkProperty.AdditionalLinkOptions.end())
                    {
                        continue;
                    }
                    
                    const std::vector<std::string> additionalLinkOptions = 
                        currentLinkProperty.AdditionalLinkOptions.at(platformNames.at(j));
                    
                    for(int k = 0; k < additionalLinkOptions.size(); ++k)
                        linkFlags += " " + additionalLinkOptions[k];
                }
            }
        }
        
        //Replace for {LinkFlags} for output part
        std::size_t foundIndex = currentOutputPart.find(linkFlagsSubstitution);
        if(foundIndex == std::string::npos)
        {
            ssLOG_ERROR("'" + linkFlagsSubstitution + "' missing in LinkerArgs");
            return false;
        }
        currentOutputPart.replace(foundIndex, linkFlagsSubstitution.size(), linkFlags);
        
        //Replace {OutputFile} for output part
        foundIndex = currentOutputPart.find(outputFileSubstitution);
        if(foundIndex == std::string::npos)
        {
            ssLOG_ERROR("'" + outputFileSubstitution + "' missing in LinkerArgs");
            return false;
        }
        currentOutputPart.replace(foundIndex, outputFileSubstitution.size(), outputName);
        
        //Replace {ObjectFile} for output part
        foundIndex = currentOutputPart.find(objectFileSubstitution);
        if(foundIndex == std::string::npos)
        {
            ssLOG_ERROR("'" + objectFileSubstitution + "' missing in LinkerArgs");
            return false;
        }
        
        currentOutputPart.replace(foundIndex, objectFileSubstitution.size(), scriptObjectFilePath);
        runcpp2::Trim(currentOutputPart);
        linkCommand += currentOutputPart;
        
        //Replace {DependencyFile} for Dependencies part
        const std::string dependencySubstitution = "{DependencyFile}";
        const std::string dependencyPart = profile.Linker.LinkerArgs.DependenciesPart;
        
        foundIndex = dependencyPart.find(dependencySubstitution);
        if(foundIndex == std::string::npos)
        {
            ssLOG_ERROR("'" + dependencySubstitution + "' missing in LinkerArgs");
            return false;
        }
        
        for(int i = 0; i < copiedDependenciesBinariesPaths.size(); ++i)
        {
            std::string currentDependencyPart = dependencyPart;
            currentDependencyPart.replace(  foundIndex, 
                                            dependencySubstitution.size(), 
                                            copiedDependenciesBinariesPaths[i]);
            
            linkCommand += " " + currentDependencyPart;
        }
        
        std::string scriptDirectory = ghc::filesystem::path(scriptPath).parent_path().string();
        std::string runcpp2ScriptDir = runcpp2::ProcessPath(scriptDirectory + "/.runcpp2");
        linkCommand = "cd " + runcpp2ScriptDir + " && " + linkCommand;
        
        //Do Linking
        System2CommandInfo linkCommandInfo;
        SYSTEM2_RESULT result = System2Run(linkCommand.c_str(), &linkCommandInfo);
        
        ssLOG_INFO("running link command: " << linkCommand);
        
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
}


bool runcpp2::CompileAndLinkScript( const std::string& scriptPath, 
                                    const Data::ScriptInfo& scriptInfo,
                                    const Data::CompilerProfile& profile,
                                    const std::vector<std::string>& copiedDependenciesBinariesPaths)
{
    std::string scriptObjectFilePath;

    if(!CompileScript(scriptPath, scriptInfo, profile, scriptObjectFilePath))
    {
        ssLOG_ERROR("CompileScript failed");
        return false;
    }
    
    if(!LinkScript( scriptPath, 
                    scriptInfo,
                    profile,
                    scriptObjectFilePath,
                    copiedDependenciesBinariesPaths))
    {
        ssLOG_ERROR("LinkScript failed");
        return false;
    }
    
    return true;
}