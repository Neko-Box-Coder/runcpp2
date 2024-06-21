#include "runcpp2/runcpp2.hpp"

#include "runcpp2/ProfileHelper.hpp"
#include "runcpp2/CompilingLinking.hpp"
#include "runcpp2/ConfigParsing.hpp"
#include "runcpp2/DependenciesSetupHelper.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "runcpp2/PlatformUtil.hpp"
#include "System2.h"
#include "ssLogger/ssLog.hpp"
#include "ghc/filesystem.hpp"
#include "dylib.hpp"

#include <fstream>
#include <chrono>

namespace
{
    bool CreateRuncpp2ScriptDirectory(const std::string& scriptPath)
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

    bool RunCompiledScript( const std::string& scriptDirectory,
                            const std::string& scriptName,
                            const std::string& exeExt,
                            const std::vector<std::string>& runArgs)
    {
        INTERNAL_RUNCPP2_SAFE_START();
        ssLOG_FUNC_DEBUG();
        
        std::error_code _;
        
        std::string runCommand = runcpp2::ProcessPath(scriptDirectory + "/" + scriptName + exeExt);
        
        if(!runArgs.empty())
        {
            for(int i = 0; i < runArgs.size(); ++i)
                runCommand += " \"" + runArgs[i] + "\"";
        }
        
        ssLOG_INFO("Running: " << runCommand);
        
        System2CommandInfo runCommandInfo = {};
        runCommandInfo.RedirectOutput = true;
        SYSTEM2_RESULT result = System2Run(runCommand.c_str(), &runCommandInfo);
        
        if(result != SYSTEM2_RESULT_SUCCESS)
        {
            ssLOG_ERROR("System2Run failed with result: " << result);
            ghc::filesystem::remove(scriptDirectory + "/" + std::string(scriptName + exeExt), _);
            return false;
        }
        
        int statusCode = 0;
        do
        {
            uint32_t byteRead = 0;
            const int bufferSize = 32;
            char output[bufferSize] = {0};
            
            result = System2ReadFromOutput( &runCommandInfo, 
                                            output, 
                                            bufferSize - 1, 
                                            &byteRead);

            output[byteRead] = '\0';
            
            //Log the output and continue fetching output
            if(result == SYSTEM2_RESULT_READ_NOT_FINISHED)
            {
                std::cout << output << std::flush;
                continue;
            }
            
            //If we have finished reading the output, check if the command has finished
            if(result == SYSTEM2_RESULT_SUCCESS)
            {
                std::cout << output << std::flush;
                
                result = System2GetCommandReturnValueAsync(&runCommandInfo, &statusCode);
                
                //If the command is not finished, continue reading output
                if(result == SYSTEM2_RESULT_COMMAND_NOT_FINISHED)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    continue;
                }
                else
                    break;
            }
            else
            {
                ssLOG_ERROR("Failed to read from output with result: " << result);
                ghc::filesystem::remove(scriptDirectory + "/" + std::string(scriptName + exeExt), _);
                return false;
            }
        }
        while(true);
        
        if(result != SYSTEM2_RESULT_SUCCESS)
        {
            ssLOG_ERROR("System2GetCommandReturnValueASync failed with result: " << result);
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
        return true;
        INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
    }
    
    bool CopyCompiledExecutable(const std::string& scriptDirectory,
                                const std::string& scriptName,
                                const std::string& target,
                                const std::string& exeExt,
                                const std::vector<std::string>& copiedBinariesPaths,
                                const std::string& targetSharedLibExt)
    {
        ssLOG_FUNC_DEBUG();
        
        std::error_code _;
        std::error_code copyErrorCode;
        ghc::filesystem::copy(  target, 
                                scriptDirectory + "/" + scriptName + 
                                exeExt, 
                                copyErrorCode);
        
        if(copyErrorCode)
        {
            ssLOG_ERROR("Failed to copy file from " << target << " to " << scriptDirectory);
            ssLOG_ERROR("Error code: " << copyErrorCode.message());
            return -1;
        }
        
        //Copy library files as well
        for(int i = 0; i < copiedBinariesPaths.size(); ++i)
        {
            if(!ghc::filesystem::exists(copiedBinariesPaths[i], _))
            {
                ssLOG_ERROR("Failed to find copied file: " << copiedBinariesPaths[i]);
                return -1;
            }
            
            size_t foundIndex = copiedBinariesPaths[i].find_last_of(".");
            if(foundIndex != std::string::npos)
            {
                std::string ext = copiedBinariesPaths[i].substr(foundIndex);
                
                if(ext == targetSharedLibExt)
                {
                    const std::string targetFileName =
                        ghc::filesystem::path(copiedBinariesPaths[i]).filename().string();
                    
                    ghc::filesystem::copy(  copiedBinariesPaths[i], 
                                            scriptDirectory + "/" + targetFileName,
                                            copyErrorCode);
                }
            }
        }
        
        if(copyErrorCode)
        {
            ssLOG_ERROR("Failed to copy file from " << target << " to " << scriptDirectory);
            ssLOG_ERROR("Error code: " << copyErrorCode.message());
            return -1;
        }
        
        return 0;
    }
    
    int RunCompiledSharedLib(   const std::string& scriptDirectory,
                                const std::string& scriptPath,
                                const std::string& compiledSharedLibPath,
                                const std::vector<std::string>& runArgs)
    {
        INTERNAL_RUNCPP2_SAFE_START();
        ssLOG_FUNC_DEBUG();
        
        std::error_code _;
        if(!ghc::filesystem::exists(compiledSharedLibPath, _))
        {
            ssLOG_ERROR("Failed to find shared library: " << compiledSharedLibPath);
            return -1;
        }
        
        //Load it
        dylib sharedLib(compiledSharedLibPath, dylib::no_filename_decorations);
        
        //Get main as entry point
        if(sharedLib.has_symbol("main") == false)
        {
            ssLOG_ERROR("The shared library does not have a main function");
            return -1;
        }
        
        int (*scriptFullMain)(int, char**) = nullptr;
        int (*scriptMain)() = nullptr;
        
        try
        {
            scriptFullMain = sharedLib.get_function<int(int, char**)>("main");
        }
        catch(const dylib::exception& ex)
        {
            ssLOG_DEBUG("Failed to get full main function from shared library: " << ex.what());
        }
        catch(...)
        {
            ssLOG_ERROR("Failed to get entry point function");
            return -1;
        }
        
        if(scriptFullMain == nullptr)
        {
            try
            {
                scriptMain = sharedLib.get_function<int()>("_main");
            }
            catch(const dylib::exception& ex)
            {
                ssLOG_DEBUG("Failed to get main function from shared library: " << ex.what());
            }
            catch(...)
            {
                ssLOG_ERROR("Failed to get entry point function");
                return -1;
            }
        }
        
        if(scriptMain == nullptr && scriptFullMain == nullptr)
        {
            ssLOG_ERROR("Failed to load function");
            return -1;
        }
        
        //Run the entry point
        int result = 0;
        if(scriptFullMain != nullptr)
        {
            std::vector<std::string> runArgsCopy = runArgs;
            runArgsCopy.insert(runArgsCopy.begin(), scriptPath);
            
            std::vector<char*> runArgsCStr(runArgsCopy.size());
            for(int i = 0; i < runArgsCopy.size(); ++i)
                runArgsCStr[i] = &runArgsCopy[i][0];
            
            result = scriptFullMain(runArgsCStr.size(), runArgsCStr.data());
        }
        else if(scriptMain != nullptr)
            result = scriptMain();
        
        return result;
        
        INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(-1);
    }
    
    bool HasCompiledCache(  const std::unordered_map<   runcpp2::CmdOptions, 
                                                        std::string>& currentOptions,
                            const std::string& absoluteScriptPath,
                            const std::string& exeExt,
                            const std::vector<runcpp2::Data::Profile>& profiles,
                            int profileIndex,
                            std::vector<std::string>& outCopiedBinariesPaths)
    {
        ssLOG_FUNC_DEBUG();
        
        std::string scriptDirectory = ghc::filesystem::path(absoluteScriptPath) .parent_path()
                                                                                .string();
        
        std::string scriptName = ghc::filesystem::path(absoluteScriptPath).stem().string();
        std::error_code _;
        ghc::filesystem::file_time_type lastScriptWriteTime = 
            ghc::filesystem::last_write_time(absoluteScriptPath, _);
        
        //Check if the c/cpp file is newer than the compiled c/c++ file
        if(currentOptions.find(runcpp2::CmdOptions::RESET_DEPENDENCIES) == currentOptions.end())
        {
            //If we are compiling to an executable
            if(currentOptions.find(runcpp2::CmdOptions::EXECUTABLE) != currentOptions.end())
            {
                std::string exeToCopy = scriptDirectory + "/.runcpp2/" + scriptName + exeExt;
                const std::string* sharedLibExtToCopy = 
                    runcpp2::GetValueFromPlatformMap(profiles[profileIndex] .SharedLibraryFile
                                                                            .Extension);

                const std::string* debugExtToCopy = 
                    runcpp2::GetValueFromPlatformMap(profiles[profileIndex] .DebugSymbolFile
                                                                            .Extension);

                if(sharedLibExtToCopy == nullptr)
                {
                    ssLOG_ERROR("Shared library extension not found in compiler profile");
                    return -1;
                }

                //If the executable already exists, check if it's newer than the script
                if(ghc::filesystem::exists(exeToCopy, _))
                {
                    ghc::filesystem::file_time_type lastExecutableWriteTime = 
                        ghc::filesystem::last_write_time(exeToCopy, _);
                    
                    if(lastExecutableWriteTime < lastScriptWriteTime)
                        ssLOG_INFO("Compiled file is older than the source file");
                    else
                    {
                        using namespace ghc::filesystem;
                        
                        //Copy the shared libraries as well
                        for(auto it : directory_iterator(scriptDirectory + "/.runcpp2/", _))
                        {
                            if(it.is_directory())
                                continue;
                            
                            std::string currentFileName = it.path().stem().string();
                            std::string currentExtension = it.path().extension().string();
                            
                            ssLOG_DEBUG("currentFileName: " << currentFileName);
                            
                            if(currentExtension == *sharedLibExtToCopy)
                                outCopiedBinariesPaths.push_back(it.path().string());
                            else if(debugExtToCopy != nullptr && 
                                    currentExtension == *debugExtToCopy)
                            {
                                outCopiedBinariesPaths.push_back(it.path().string());
                            }
                        }
                        
                        return true;
                    }
                }
            }
            //If we are compiling to a shared library
            else
            {
                //Check if there's any existing shared library build that is newer than the script
                const std::string* targetSharedLibExt = 
                    runcpp2::GetValueFromPlatformMap(profiles[profileIndex] .SharedLibraryFile
                                                                            .Extension);
                
                const std::string* targetSharedLibPrefix =
                    runcpp2::GetValueFromPlatformMap(profiles[profileIndex] .SharedLibraryFile
                                                                            .Prefix);
                
                if(targetSharedLibExt == nullptr || targetSharedLibPrefix == nullptr)
                {
                    ssLOG_ERROR("Shared library extension or prefix not found in compiler profile");
                    return -1;
                }
                
                std::string sharedLibBuild =    scriptDirectory + 
                                                "/.runcpp2/" + 
                                                *targetSharedLibPrefix + 
                                                scriptName + 
                                                *targetSharedLibExt;
                
                if(ghc::filesystem::exists(sharedLibBuild, _))
                {
                    ghc::filesystem::file_time_type lastSharedLibWriteTime = 
                        ghc::filesystem::last_write_time(sharedLibBuild, _);
                    
                    if(lastSharedLibWriteTime < lastScriptWriteTime)
                        ssLOG_INFO("Compiled file is older than the source file");
                    else
                        return true;
                }
            }
        }
        
        return false;
    }
}

int runcpp2::RunScript( const std::string& scriptPath, 
                        const std::vector<Data::Profile>& profiles,
                        const std::string& configPreferredProfile,
                        const std::unordered_map<CmdOptions, std::string> currentOptions,
                        const std::vector<std::string>& runArgs)
{
    if(profiles.empty())
    {
        ssLOG_ERROR("No compiler profiles found");
        return -1;
    }
    
    //Check if input file exists
    {
        std::error_code _;
        
        if(!ghc::filesystem::exists(scriptPath, _))
        {
            ssLOG_ERROR("Failed to check if file exists: " << scriptPath);
            return -1;
        }
        
        if(ghc::filesystem::is_directory(scriptPath, _))
        {
            ssLOG_ERROR("The input file must not be a directory: " << scriptPath);
            return -1;
        }
    }
    
    std::string absoluteScriptPath = ghc::filesystem::absolute(scriptPath).string();
    std::string scriptDirectory = ghc::filesystem::path(absoluteScriptPath).parent_path().string();
    std::string scriptName = ghc::filesystem::path(absoluteScriptPath).stem().string();
    std::vector<std::string> copiedBinariesPaths;
    int profileIndex = -1;

    ssLOG_DEBUG("scriptPath: " << scriptPath);
    ssLOG_DEBUG("absoluteScriptPath: " << absoluteScriptPath);
    ssLOG_DEBUG("scriptDirectory: " << scriptDirectory);
    ssLOG_DEBUG("scriptName: " << scriptName);
    ssLOG_DEBUG("is_directory: " << ghc::filesystem::is_directory(scriptDirectory));

    //Read from a c/cpp file
    //TODO: Check is it c or cpp

    std::string exeExt = "";
    #ifdef _WIN32
        exeExt = ".exe";
    #endif

    //Parsing the script, setting up dependencies, compiling and linking
    {
        std::ifstream inputFile(absoluteScriptPath);
        if (!inputFile)
        {
            ssLOG_ERROR("Failed to open file: " << absoluteScriptPath);
            return -1;
        }

        std::stringstream buffer;
        buffer << inputFile.rdbuf();
        std::string source(buffer.str());

        std::string parsableInfo;
        if(!GetParsableInfo(source, parsableInfo))
        {
            ssLOG_ERROR("An error has been encountered when parsing info: " << absoluteScriptPath);
            return -1;
        }
        
        //TODO: Check if there's script info as yaml file instead

        //Try to parse the runcpp2 info
        Data::ScriptInfo scriptInfo;
        if(!ParseScriptInfo(parsableInfo, scriptInfo))
        {
            ssLOG_ERROR("Failed to parse info");
            ssLOG_ERROR("Content trying to parse: " << "\n" << parsableInfo);
            return -1;
        }
        
        if(!parsableInfo.empty())
        {
            ssLOG_INFO("Parsed script info YAML:\n");
            ssLOG_INFO(scriptInfo.ToString(""));
        }

        if(!CreateRuncpp2ScriptDirectory(absoluteScriptPath))
        {
            ssLOG_ERROR("Failed to create runcpp2 script directory: " << absoluteScriptPath);
            return -1;
        }

        profileIndex = GetPreferredProfileIndex(absoluteScriptPath, 
                                                scriptInfo, 
                                                profiles, 
                                                configPreferredProfile);

        if(profileIndex == -1)
        {
            ssLOG_ERROR("Failed to find a profile to run");
            return -1;
        }
        
        //Check if we have already compiled before.
        if(!HasCompiledCache(   currentOptions, 
                                absoluteScriptPath, 
                                exeExt, 
                                profiles, 
                                profileIndex, 
                                copiedBinariesPaths))
        {
            std::vector<std::string> dependenciesLocalCopiesPaths;
            std::vector<std::string> dependenciesSourcePaths;
            if(!SetupScriptDependencies(profiles[profileIndex].Name, 
                                        absoluteScriptPath, 
                                        scriptInfo, 
                                        currentOptions.count(CmdOptions::RESET_DEPENDENCIES) > 0,
                                        dependenciesLocalCopiesPaths,
                                        dependenciesSourcePaths))
            {
                ssLOG_ERROR("Failed to setup script dependencies");
                return -1;
            }

            if(!CopyDependenciesBinaries(   absoluteScriptPath, 
                                            scriptInfo,
                                            dependenciesLocalCopiesPaths,
                                            profiles[profileIndex],
                                            copiedBinariesPaths))
            {
                ssLOG_ERROR("Failed to copy dependencies binaries");
                return -1;
            }

            if(!CompileAndLinkScript(   absoluteScriptPath, 
                                        scriptInfo,
                                        profiles[profileIndex],
                                        copiedBinariesPaths,
                                        (currentOptions.count(CmdOptions::EXECUTABLE) > 0),
                                        exeExt))
            {
                ssLOG_ERROR("Failed to compile or link script");
                return -1;
            }
        }
    }

    //Copying the compiled file to script directory
    {
        ssLOG_INFO("Copying script...");
        
        std::error_code _;
        std::string target = scriptDirectory + "/.runcpp2/";
        
        const std::string* targetSharedLibExt = 
            runcpp2::GetValueFromPlatformMap(profiles[profileIndex].SharedLibraryFile.Extension);
        
        const std::string* targetSharedLibPrefix =
            runcpp2::GetValueFromPlatformMap(profiles[profileIndex].SharedLibraryFile.Prefix);
        
        if(currentOptions.find(CmdOptions::EXECUTABLE) != currentOptions.end())
            target += scriptName + exeExt;
        else
        {
            if(targetSharedLibExt == nullptr || targetSharedLibPrefix == nullptr)
            {
                ssLOG_ERROR("Shared library extension or prefix not found in compiler profile");
                return -1;
            }

            target += *targetSharedLibPrefix + scriptName + *targetSharedLibExt;
        }
        
        if(!ghc::filesystem::exists(target, _))
        {
            ssLOG_ERROR("Failed to find the compiled file: " << target);
            return -1;
        }
        
        if(currentOptions.count(CmdOptions::EXECUTABLE) > 0)
        {
            if(!CopyCompiledExecutable( scriptDirectory,
                                        scriptName,
                                        target,
                                        exeExt,
                                        copiedBinariesPaths,
                                        *targetSharedLibExt))
            {
                ssLOG_ERROR("Failed to copy compiled executable: " << target);
                return -1;
            }
            
            //Run it?
            if(false)
            {
                //Running the script
                if(!RunCompiledScript(scriptDirectory, scriptName, exeExt, runArgs))
                {
                    ssLOG_ERROR("Failed to run script");
                    return false;
                }
            }
            
            return 0;
        }
        //Load the shared library and run it
        else
        {
            int result = RunCompiledSharedLib(  scriptDirectory, 
                                                absoluteScriptPath, 
                                                target, 
                                                runArgs);
        
            return result;
        }
    }

    return -1;
}

