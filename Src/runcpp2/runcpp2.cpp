#include "runcpp2/runcpp2.hpp"

#include "runcpp2/ParseUtil.hpp"
#include "runcpp2/PlatformUtil.hpp"

#include "ssLogger/ssLog.hpp"
#include "tinydir.h"
#include "cfgpath.h"
#include "yaml-cpp/yaml.h"

extern "C"
{
    #include "mkdirp.h"
}

#include <fstream>

extern const uint8_t DefaultCompilerProfiles[];
extern const size_t DefaultCompilerProfiles_size;
extern const uint8_t DefaultScriptDependencies[];
extern const size_t DefaultScriptDependencies_size;

namespace runcpp2
{
    bool ParseCompilerProfiles( const std::string& compilerProfilesString, 
                                std::vector<CompilerProfile>& outProfiles)
    {
        YAML::Node compilersProfileYAML;
        
        try
        {
            compilersProfileYAML = YAML::Load(compilerProfilesString);
        }
        catch(...)
        {
            return false;
        }

        YAML::Node compilerProfilesNode = compilersProfileYAML["CompilerProfiles"];
        if(!compilerProfilesNode || compilerProfilesNode.Type() != YAML::NodeType::Sequence)
        {
            ssLOG_ERROR("CompilerProfiles is invalid");
            return false;
        }
        
        if(compilerProfilesNode.size() == 0)
        {
            ssLOG_ERROR("No compiler profiles found");
            return false;
        }
        
        for(int i = 0; i < compilerProfilesNode.size(); ++i)
        {
            YAML::Node currentCompilerProfileNode = compilerProfilesNode[i];
            
            outProfiles.push_back({});
            if(!outProfiles.back().ParseYAML_Node(currentCompilerProfileNode))
            {
                outProfiles.erase(outProfiles.end() - 1);
                ssLOG_ERROR("Failed to parse compiler profile at index " << i);
                return false;
            }
        }
        
        return true;
    }
    
    #ifdef _WIN32
        #define INTERNAL_RUNCPP2_FS_SLASH '\\'
        #define INTERNAL_RUNNCPP2_ALT_FS_SLASH '/'
    #elif __linux__
        #define INTERNAL_RUNCPP2_FS_SLASH '/'
        #define INTERNAL_RUNNCPP2_ALT_FS_SLASH '\\'
    #elif __APPLE__
        #define INTERNAL_RUNCPP2_FS_SLASH '/'
        #define INTERNAL_RUNNCPP2_ALT_FS_SLASH '\\'
    #else
        #define INTERNAL_RUNCPP2_FS_SLASH '/'
        #define INTERNAL_RUNNCPP2_ALT_FS_SLASH '/'
    #endif

    bool ReadUserConfig(std::vector<CompilerProfile>& outProfiles)
    {
        //Check if user config exists
        char configDirC_Str[MAX_PATH] = {0};
        
        get_user_config_folder(configDirC_Str, 512, "runcpp2");
        
        if(strlen(configDirC_Str) == 0)
        {
            ssLOG_ERROR("Failed to retrieve user config path");
            return false;
        }
        
        std::string configDir = std::string(configDirC_Str);
        
        std::string compilerConfigFilePaths[2] = 
        {
            configDir + INTERNAL_RUNCPP2_FS_SLASH + "CompilerProfiles.yaml", 
            configDir + INTERNAL_RUNCPP2_FS_SLASH + "CompilerProfiles.yml"
        };
        int foundConfigFilePathIndex = -1;
        
        bool writeDefaultProfiles = false;
        
        //config directory is created by get_user_config_folder if it doesn't exist
        {
            for(int i = 0; i < sizeof(compilerConfigFilePaths) / sizeof(std::string); ++i)
            {
                //Check if the config file exists
                tinydir_file configFileInfo;
                if(tinydir_file_open(&configFileInfo, compilerConfigFilePaths[i].c_str()) == 0)
                {
                    foundConfigFilePathIndex = i;
                    writeDefaultProfiles = false;
                    break;
                }
            }
            
            writeDefaultProfiles = true;
        }
        
        //Create default compiler profiles
        if(writeDefaultProfiles)
        {
            //Create default compiler profiles
            std::ofstream configFile(compilerConfigFilePaths[0], std::ios::binary);
            if(!configFile)
            {
                ssLOG_ERROR("Failed to create default config file: " << compilerConfigFilePaths[0]);
                return false;
            }
            configFile.write((const char*)DefaultCompilerProfiles, DefaultCompilerProfiles_size);
            configFile.close();
            foundConfigFilePathIndex = 0;
        }
        
        //Read compiler profiles
        std::string compilerConfigContent;
        {
            std::ifstream compilerConfigFilePath(compilerConfigFilePaths[foundConfigFilePathIndex]);
            if(!compilerConfigFilePath)
            {
                ssLOG_ERROR("Failed to open config file: " << 
                            compilerConfigFilePaths[foundConfigFilePathIndex]);
                
                return false;
            }
            std::stringstream buffer;
            buffer << compilerConfigFilePath.rdbuf();
            compilerConfigContent = buffer.str();
        }
        
        if(!ParseCompilerProfiles(compilerConfigContent, outProfiles))
        {
            ssLOG_ERROR("Failed to parse config file: " << 
                        compilerConfigFilePaths[foundConfigFilePathIndex]);
            
            return false;
        }

        return true;
    }
    
    bool ParseScriptInfo(   const std::string& scriptInfo, 
                            ScriptInfo& outScriptInfo)
    {
        if(scriptInfo.empty())
            return true;

        YAML::Node scriptYAML;
        
        try
        {
            scriptYAML = YAML::Load(scriptInfo);
        }
        catch(...)
        {
            return false;
        }
        
        return outScriptInfo.ParseYAML_Node(scriptYAML);
    }
    
    bool RunScript(const std::string& scriptPath, const std::vector<CompilerProfile>& profiles)
    {
        if(profiles.empty())
        {
            ssLOG_ERROR("No compiler profiles found");
            return false;
        }
        
        std::string processedScriptPath = Internal::ProcessPath(scriptPath);
        
        //Check if input file exists
        {
            bool isDir = false;
            
            if(!Internal::FileExists(processedScriptPath, isDir))
            {
                ssLOG_ERROR("Failed to check if file exists: " << processedScriptPath);
                return false;
            }
            
            if(isDir)
            {
                ssLOG_ERROR("The input file must not be a directory: " << processedScriptPath);
                return false;
            }
        }
        
        std::string scriptDirectory = Internal::GetFileDirectory(processedScriptPath);
        std::string scriptName = Internal::GetFileNameWithoutExtension(processedScriptPath);

        //Read from a c/cpp file
        //TODO: Check is it c or cpp

        std::ifstream inputFile(processedScriptPath);
        if (!inputFile)
        {
            ssLOG_ERROR("Failed to open file: " << processedScriptPath);
            return false;
        }

        std::stringstream buffer;
        buffer << inputFile.rdbuf();
        std::string source(buffer.str());

        std::string parsableInfo;
        if(!runcpp2::Internal::GetParsableInfo(source, parsableInfo))
        {
            ssLOG_ERROR("An error has been encountered when parsing info: " << processedScriptPath);
            return false;
        }

        //Try to parse the runcpp2 info
        ScriptInfo scriptInfo;
        if(!runcpp2::ParseScriptInfo(parsableInfo, scriptInfo))
        {
            ssLOG_ERROR("Failed to parse info");
            ssLOG_ERROR("Content trying to parse: " << "\n" << parsableInfo);
            return false;
        }
        
        if(!parsableInfo.empty())
        {
            ssLOG_LINE("\n" << scriptInfo.ToString(""));
        }

        //TODO(NOW): Setup dependencies if any


        //Compile and execute the file
        //Check which compiler is available
        std::vector<std::pair<const CompilerProfile&, bool>> availableProfiles;
        int firstAvailableProfile = -1;
        for(int i = 0; i < profiles.size(); ++i)
        {
            //Check compiler
            std::string command = profiles.at(i).Compiler.Executable + " -v";
            
            System2CommandInfo compilerCommandInfo;
            SYSTEM2_RESULT sys2Result = System2Run(command.c_str(), &compilerCommandInfo);
            
            if(sys2Result != SYSTEM2_RESULT_SUCCESS)
            {
                ssLOG_ERROR("System2Run failed with result: " << sys2Result);
                return false;
            }
            
            int returnCode = 0;
            sys2Result = System2GetCommandReturnValueSync(&compilerCommandInfo, &returnCode);
            if(sys2Result != SYSTEM2_RESULT_SUCCESS)
            {
                ssLOG_ERROR("System2GetCommandReturnValueSync failed with result: " << sys2Result);
                return false;
            }
            
            //if(std::system(command.c_str()) != 0)
            if(returnCode != 0)
                continue;
            
            //Check linker
            command = profiles.at(i).Linker.Executable + " -v";
            
            System2CommandInfo linkerCommandInfo;
            sys2Result = System2Run(command.c_str(), &linkerCommandInfo);
            
            if(sys2Result != SYSTEM2_RESULT_SUCCESS)
            {
                ssLOG_ERROR("System2Run failed with result: " << sys2Result);
                return false;
            }
            
            returnCode = 0;
            sys2Result = System2GetCommandReturnValueSync(&linkerCommandInfo, &returnCode);
            if(sys2Result != SYSTEM2_RESULT_SUCCESS)
            {
                ssLOG_ERROR("System2GetCommandReturnValueSync failed with result: " << sys2Result);
                return false;
            }
            
            //if(std::system(command.c_str()) != 0)
            if(returnCode != 0)
                continue;
            
            availableProfiles.push_back({profiles.at(i), true});
            
            if(firstAvailableProfile == -1)
                firstAvailableProfile = i;
        }
        
        if(availableProfiles.empty())
        {
            ssLOG_ERROR("No compilers/linkers found");
            return false;
        }
        
        ssLOG_INFO("Using profile at index " << firstAvailableProfile);

        CompilerProfile currentProfile = profiles.at(firstAvailableProfile);

        std::vector<std::string> currentPlatform = Internal::GetPlatformNames();

        std::string objectFileExt;
        for(int i = 0; i < currentPlatform.size(); ++i)
        {
            if( currentProfile.ObjectFileExtensions.find(currentPlatform[i]) != 
                currentProfile.ObjectFileExtensions.end())
            {
                objectFileExt = currentProfile.ObjectFileExtensions.at(currentPlatform[i]);
                break;
            }
        }
        
        //Create the temporary directory
        std::string tempDir = scriptDirectory + ".runcpp2";
        if(mkdirp(tempDir.c_str() , 0777) != 0)
        {
            ssLOG_ERROR("Failed to create temporary directory");
            return false;
        }
        
        //Compile the script
            return true;
        
        std::string compileCommand =    currentProfile.Compiler.Executable + " " + 
                                        currentProfile.Compiler.CompileArgs;

        //Replace for {CompileFlags}
        const std::string compileFlagSubstitution = "{CompileFlags}";
        std::size_t foundIndex = compileCommand.find(compileFlagSubstitution);
        if(foundIndex != std::string::npos)
        {
            //TODO: Allow user to override compile flags
            compileCommand.replace( foundIndex, 
                                    compileFlagSubstitution.size(), 
                                    currentProfile.Compiler.DefaultCompileFlags);
        }
        else
        {
            ssLOG_ERROR("'{CompileFlags}' missing in CompileArgs");
            return false;
        }
        
        //Replace {InputFile}
        const std::string inputFileSubstitution = "{InputFile}";
        foundIndex = compileCommand.find(inputFileSubstitution);
        if(foundIndex != std::string::npos)
            compileCommand.replace(foundIndex, inputFileSubstitution.size(), processedScriptPath);
        else
        {
            ssLOG_ERROR("'{InputFile}' missing in CompileArgs");
            return false;
        }
        
        //Replace {ObjectFile}
        const std::string objectFileSubstitution = "{ObjectFile}";
        foundIndex = compileCommand.find(objectFileSubstitution);
        if(foundIndex != std::string::npos)
        {
            std::string objectFileName =    scriptDirectory + 
                                            ".runcpp2" + INTERNAL_RUNCPP2_FS_SLASH +
                                            scriptName + "." + objectFileExt;

            compileCommand.replace(foundIndex, objectFileSubstitution.size(), objectFileName);
        }
        else
        {
            ssLOG_ERROR("'{ObjectFile}' missing in CompileArgs");
            return false;
        }
        
        //Compile the script
        ssLOG_INFO("running compile command: " << compileCommand);
        //TODO(NOW): Replace this with system2
        if(std::system(compileCommand.c_str()) != 0)
        {
            ssLOG_ERROR("Failed to run compile script with command: " << compileCommand);
            return false;
        }
        
        

        return true;
    }
}

