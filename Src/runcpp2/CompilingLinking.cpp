#include "runcpp2/CompilingLinking.hpp"
#include "System2.h"
#include "ghc/filesystem.hpp"
#include "runcpp2/PlatformUtil.hpp"
#include "runcpp2/StringUtil.hpp"
#include "ssLogger/ssLog.hpp"

bool runcpp2::CompileAndLinkScript( const std::string& scriptPath, 
                                    const ScriptInfo& scriptInfo,
                                    const CompilerProfile& profile)
{
    std::string scriptDirectory = ghc::filesystem::path(scriptPath).parent_path().string();
    std::string scriptName = ghc::filesystem::path(scriptPath).stem().string();
    std::string runcpp2ScriptDir = Internal::ProcessPath(scriptDirectory + "/.runcpp2");
    std::vector<std::string> platformNames = Internal::GetPlatformNames();
    
    std::string compileCommand =    profile.Compiler.Executable + " " + 
                                    profile.Compiler.CompileArgs;

    //Replace for {CompileFlags}
    const std::string compileFlagSubstitution = "{CompileFlags}";
    std::size_t foundIndex = compileCommand.find(compileFlagSubstitution);
    
    std::string compileArgs = profile.Compiler.DefaultCompileFlags;
    
    if(foundIndex == std::string::npos)
    {
        ssLOG_ERROR("'{CompileFlags}' missing in CompileArgs");
        return false;
    }
    
    if(scriptInfo.OverrideCompileFlags.find(profile.Name) != scriptInfo.OverrideCompileFlags.end())
    {
        std::vector<std::string> compileArgsToRemove; 
        Internal::SplitString(  scriptInfo.OverrideCompileFlags.at(profile.Name).Remove, 
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
        
        Internal::TrimRight(compileArgs);
        compileArgs += " " + scriptInfo.OverrideCompileFlags.at(profile.Name).Append;
    }
    
    compileCommand.replace( foundIndex, 
                            compileFlagSubstitution.size(), 
                            compileArgs);
    
    //Replace {InputFile}
    const std::string inputFileSubstitution = "{InputFile}";
    foundIndex = compileCommand.find(inputFileSubstitution);
    if(foundIndex == std::string::npos)
    {
        ssLOG_ERROR("'{InputFile}' missing in CompileArgs");
        return false;
    }
    compileCommand.replace(foundIndex, inputFileSubstitution.size(), scriptPath);
    
    //Replace {ObjectFile}
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
    
    foundIndex = compileCommand.find(objectFileSubstitution);
    if(foundIndex == std::string::npos)
    {
        ssLOG_ERROR("'{ObjectFile}' missing in CompileArgs");
        return false;
    }
    
    std::string objectFileName = Internal::ProcessPath( runcpp2ScriptDir + "/" + 
                                                        scriptName + "." + objectFileExt);
    
    compileCommand.replace(foundIndex, objectFileSubstitution.size(), objectFileName);
    
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
        return false;
    }
    
    //Link the script to the dependencies
    std::string linkCommand = profile.Linker.Executable + " ";
    std::string currentOutputPart = profile.Linker.LinkerArgs.OutputPart;
    const std::string linkFlagsSubstitution = "{LinkFlags}";
    const std::string outputFileSubstitution = "{OutputFile}";
    
    std::string linkFlags = profile.Linker.DefaultLinkFlags;
    std::string outputName = scriptName;
    
    if(scriptInfo.OverrideLinkFlags.find(profile.Name) != scriptInfo.OverrideLinkFlags.end())
    {
        std::vector<std::string> linkFlagsToRemove;
        
        Internal::SplitString(  scriptInfo.OverrideLinkFlags.at(profile.Name).Remove, 
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
        
        Internal::TrimRight(linkFlags);
        linkFlags += " " + scriptInfo.OverrideLinkFlags.at(profile.Name).Append;
    }
    
    foundIndex = currentOutputPart.find(linkFlagsSubstitution);
    if(foundIndex == std::string::npos)
    {
        ssLOG_ERROR("'{LinkFlags}' missing in LinkerArgs");
        return false;
    }
    
    currentOutputPart.replace(foundIndex, linkFlagsSubstitution.size(), linkFlags);
    
    foundIndex = currentOutputPart.find(outputFileSubstitution);
    if(foundIndex == std::string::npos)
    {
        ssLOG_ERROR("'{ProgramName}' missing in LinkerArgs");
        return false;
    }
    
    currentOutputPart.replace(foundIndex, outputFileSubstitution.size(), outputName);
    
    foundIndex = currentOutputPart.find(objectFileSubstitution);
    if(foundIndex == std::string::npos)
    {
        ssLOG_ERROR("'{ObjectFile}' missing in LinkerArgs");
        return false;
    }
    
    currentOutputPart.replace(foundIndex, objectFileSubstitution.size(), objectFileName);
    Internal::Trim(currentOutputPart);
    linkCommand += currentOutputPart;
    
    const std::string dependencySubstitution = "{DependencyName}";
    const std::string dependencyPart = profile.Linker.LinkerArgs.DependenciesPart;
    
    foundIndex = dependencyPart.find(dependencySubstitution);
    if(foundIndex == std::string::npos)
    {
        ssLOG_ERROR("'{DependencyName}' missing in LinkerArgs");
        return false;
    }
    
    for(int i = 0; i < scriptInfo.Dependencies.size(); ++i)
    {
        if(scriptInfo.Dependencies.at(i).LibraryType == DependencyLibraryType::HEADER)
            continue;
        
        std::string dependencyName = scriptInfo .Dependencies
                                                .at(i)
                                                .SearchProperties
                                                .at(profile.Name)
                                                .SearchLibraryName;
        
        std::string currentDependencyPart = dependencyPart;
        currentDependencyPart.replace(foundIndex, dependencySubstitution.size(), dependencyName);
        
        if(i == 0)
            linkCommand += " ";
        
        linkCommand += currentDependencyPart;
    }
    
    linkCommand = "cd " + runcpp2ScriptDir + " && " + linkCommand;
    
    System2CommandInfo linkCommandInfo;
    result = System2Run(linkCommand.c_str(), &linkCommandInfo);
    
    ssLOG_INFO("running link command: " << linkCommand);
    
    if(result != SYSTEM2_RESULT_SUCCESS)
    {
        ssLOG_ERROR("System2Run failed with result: " << result);
        return false;
    }
    
    output.clear();
    do
    {
        uint32_t byteRead = 0;
        
        output.resize(output.size() + 4096);
        
        result = System2ReadFromOutput( &linkCommandInfo, 
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
    
    ssLOG_DEBUG("Link Output: \n" << output.data());
    
    statusCode = 0;
    result = System2GetCommandReturnValueSync(&linkCommandInfo, &statusCode);
    
    if(result != SYSTEM2_RESULT_SUCCESS)
    {
        ssLOG_ERROR("System2GetCommandReturnValueSync failed with result: " << result);
        return false;
    }
    
    if(statusCode != 0)
    {
        ssLOG_ERROR("Link command returned with non-zero status code: " << statusCode);
        return false;
    }
    
    
    return true;
}