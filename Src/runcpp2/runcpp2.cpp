#include "runcpp2/runcpp2.hpp"

#include "runcpp2/CompilerProfileHelper.hpp"
#include "runcpp2/CompilingLinking.hpp"
#include "runcpp2/ConfigParsing.hpp"
#include "runcpp2/DependenciesSetupHelper.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "runcpp2/PlatformUtil.hpp"
#include "System2.h"
#include "runcpp2/StringUtil.hpp"
#include "ssLogger/ssLog.hpp"
#include "yaml-cpp/yaml.h"
#include "ghc/filesystem.hpp"

#include <fstream>


bool runcpp2::CreateRuncpp2ScriptDirectory(const std::string& scriptPath)
{
    std::string scriptDirectory = ghc::filesystem::path(scriptPath).parent_path().string();
    
    //Create the runcpp2 directory
    std::string runcpp2Dir = scriptDirectory + "/.runcpp2";
    
    if(!ghc::filesystem::exists(runcpp2Dir))
    {
        std::error_code _;
        if(!ghc::filesystem::create_directory(runcpp2Dir, _))
        {
            ssLOG_ERROR("Failed to create runcpp2 directory");
            return false;
        }
    }
    
    return true;
}

bool runcpp2::RunScript(const std::string& scriptPath, 
                        const std::vector<CompilerProfile>& profiles,
                        const std::string& configPreferredProfile,
                        const std::unordered_map<CmdOptions, std::string> currentOptions,
                        const std::vector<std::string>& runArgs)
{
    if(profiles.empty())
    {
        ssLOG_ERROR("No compiler profiles found");
        return false;
    }
    
    //Check if input file exists
    {
        std::error_code _;
        
        if(!ghc::filesystem::exists(scriptPath, _))
        {
            ssLOG_ERROR("Failed to check if file exists: " << scriptPath);
            return false;
        }
        
        if(ghc::filesystem::is_directory(scriptPath, _))
        {
            ssLOG_ERROR("The input file must not be a directory: " << scriptPath);
            return false;
        }
    }
    
    std::string absoluteScriptPath = ghc::filesystem::absolute(scriptPath).string();
    std::string scriptDirectory = ghc::filesystem::path(absoluteScriptPath).parent_path().string();
    std::string scriptName = ghc::filesystem::path(absoluteScriptPath).stem().string();

    ssLOG_DEBUG("scriptPath: " << scriptPath);
    ssLOG_DEBUG("absoluteScriptPath: " << absoluteScriptPath);
    ssLOG_DEBUG("scriptDirectory: " << scriptDirectory);
    ssLOG_DEBUG("scriptName: " << scriptName);
    
    ssLOG_DEBUG("is_directory: " << ghc::filesystem::is_directory(scriptDirectory));
    

    //Read from a c/cpp file
    //TODO: Check is it c or cpp

    std::ifstream inputFile(absoluteScriptPath);
    if (!inputFile)
    {
        ssLOG_ERROR("Failed to open file: " << absoluteScriptPath);
        return false;
    }

    std::stringstream buffer;
    buffer << inputFile.rdbuf();
    std::string source(buffer.str());

    std::string parsableInfo;
    if(!GetParsableInfo(source, parsableInfo))
    {
        ssLOG_ERROR("An error has been encountered when parsing info: " << absoluteScriptPath);
        return false;
    }
    
    //TODO: Check if there's script info as yaml file instead

    //Try to parse the runcpp2 info
    ScriptInfo scriptInfo;
    if(!ParseScriptInfo(parsableInfo, scriptInfo))
    {
        ssLOG_ERROR("Failed to parse info");
        ssLOG_ERROR("Content trying to parse: " << "\n" << parsableInfo);
        return false;
    }
    
    if(!parsableInfo.empty())
    {
        ssLOG_INFO("Parsed script info YAML:\n");
        ssLOG_INFO(scriptInfo.ToString(""));
    }

    if(!CreateRuncpp2ScriptDirectory(absoluteScriptPath))
    {
        ssLOG_ERROR("Failed to create runcpp2 script directory: " << absoluteScriptPath);
        return false;
    }

    int profileIndex = GetPreferredProfileIndex(absoluteScriptPath, 
                                                scriptInfo, 
                                                profiles, 
                                                configPreferredProfile);

    if(profileIndex == -1)
    {
        ssLOG_ERROR("Failed to find a profile to run");
        return false;
    }

    std::vector<std::string> dependenciesLocalCopiesPaths;
    std::vector<std::string> dependenciesSourcePaths;
    if(!SetupScriptDependencies(profiles[profileIndex].Name, 
                                absoluteScriptPath, 
                                scriptInfo, 
                                currentOptions.find(CmdOptions::SETUP) != currentOptions.end(),
                                dependenciesLocalCopiesPaths,
                                dependenciesSourcePaths))
    {
        ssLOG_ERROR("Failed to setup script dependencies");
        return false;
    }

    std::vector<std::string> copiedBinariesNames;

    if(!CopyDependenciesBinaries(   absoluteScriptPath, 
                                    scriptInfo,
                                    dependenciesLocalCopiesPaths,
                                    profiles[profileIndex],
                                    copiedBinariesNames))
    {
        ssLOG_ERROR("Failed to copy dependencies binaries");
        return false;
    }

    if(!CompileAndLinkScript(   absoluteScriptPath, 
                                scriptInfo,
                                profiles[profileIndex],
                                copiedBinariesNames))
    {
        ssLOG_ERROR("Failed to compile or link script");
        return false;
    }

    //Copying the compiled file to script directory
    std::string exeExt = "";
    std::error_code _;
    {
        #ifdef _WIN32
            exeExt = ".exe";
        #endif
        
        std::string exeToCopy = scriptDirectory + "/.runcpp2/" + scriptName + exeExt;
        
        if(!ghc::filesystem::exists(exeToCopy))
        {
            ssLOG_ERROR("Failed to find the compiled file: " << exeToCopy);
            return false;
        }
        
        if(!ghc::filesystem::copy_file(exeToCopy, scriptDirectory + "/" + scriptName + exeExt, _))
        {
            ssLOG_ERROR("Failed to copy file from " << exeToCopy << " to " << scriptDirectory);
            ssLOG_ERROR("Error code: " << _.message());
            return false;
        }
    }

    //Running the script
    {
        std::string runCommand = ProcessPath(   "cd " + scriptDirectory + 
                                               " && ./" + scriptName + exeExt);
        
        if(!runArgs.empty())
        {
            //TODO(NOW): Test this double quote wrapping on windows
            for(int i = 0; i < runArgs.size(); ++i)
                runCommand += " \"" + runArgs[i] + "\"";
        }
        
        ssLOG_INFO("Running: " << runCommand);
        
        System2CommandInfo runCommandInfo;
        SYSTEM2_RESULT result = System2Run(runCommand.c_str(), &runCommandInfo);
        
        if(result != SYSTEM2_RESULT_SUCCESS)
        {
            ssLOG_ERROR("System2Run failed with result: " << result);
            ghc::filesystem::remove(scriptDirectory + "/" + std::string(scriptName + exeExt), _);
            return false;
        }
        
        std::vector<char> output;
        do
        {
            uint32_t byteRead = 0;
            output.resize(output.size() + 4096);
            
            result = System2ReadFromOutput( &runCommandInfo, 
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
            ghc::filesystem::remove(scriptDirectory + "/" + std::string(scriptName + exeExt), _);
            return false;
        }
        
        ssLOG_SIMPLE(output.data());
        
        int statusCode = 0;
        result = System2GetCommandReturnValueSync(&runCommandInfo, &statusCode);
        
        if(result != SYSTEM2_RESULT_SUCCESS)
        {
            ssLOG_ERROR("System2GetCommandReturnValueSync failed with result: " << result);
            ghc::filesystem::remove(scriptDirectory + "/" + std::string(scriptName + exeExt), _);
            return false;
        }
        
        if(statusCode != 0)
        {
            ssLOG_ERROR("Run command returned with non-zero status code: " << statusCode);
            ghc::filesystem::remove(scriptDirectory + "/" + std::string(scriptName + exeExt), _);
            return false;
        }
        
        ghc::filesystem::remove(scriptDirectory + "/" + std::string(scriptName + exeExt), _);
    }
    return true;
}

