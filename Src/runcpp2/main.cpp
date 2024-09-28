#include "runcpp2/ConfigParsing.hpp"
#include "runcpp2/StringUtil.hpp"
#include "runcpp2/runcpp2.hpp"

#include "ssLogger/ssLog.hpp"

#include "ghc/filesystem.hpp"


//TODO: Merge long and short options into a single structure
int ParseArgs(  const std::unordered_map<std::string, runcpp2::OptionInfo>& longOptionsMap,
                const std::unordered_map<std::string, const runcpp2::OptionInfo&>& shortOptionsMap,
                std::unordered_map<runcpp2::CmdOptions, std::string>& outOptions,
                int argc, 
                char* argv[])
{
    int currentArgIndex = 0;
    runcpp2::CmdOptions optionForCapturingValue = runcpp2::CmdOptions::NONE;
    
    for(int i = 1; i < argc; ++i)
    {
        std::string currentArg = std::string(argv[i]);
        
        //Storing value for last option
        if(optionForCapturingValue != runcpp2::CmdOptions::NONE)
        {
            //If the current argument matches one of the options, error out
            if(longOptionsMap.count(currentArg) || shortOptionsMap.count(currentArg))
            {
                //Find the string for the option to error out
                for(auto it = longOptionsMap.begin(); it != longOptionsMap.end(); ++it)
                {
                    if(it->second.Option == optionForCapturingValue)
                    {
                        ssLOG_ERROR("Missing value for option: " << it->first);
                        return -1;
                    }
                }
                
                return -1;
            }
            
            outOptions[optionForCapturingValue] = currentArg;
            optionForCapturingValue = runcpp2::CmdOptions::NONE;
            currentArgIndex = i;
            ssLOG_DEBUG("currentArgIndex: " << currentArgIndex);
            ssLOG_DEBUG("argv: " << argv[i]);
            continue;
        }
        
        //Checking for options
        if(longOptionsMap.count(currentArg) || shortOptionsMap.count(currentArg))
        {
            currentArgIndex = i;
            ssLOG_DEBUG("currentArgIndex: " << currentArgIndex);
            ssLOG_DEBUG("argv: " << argv[i]);
            
            const runcpp2::OptionInfo& currentInfo =    longOptionsMap.count(currentArg) ?
                                                        longOptionsMap.at(currentArg) :
                                                        shortOptionsMap.at(currentArg);
            
            if(currentInfo.HasValue)
            {
                optionForCapturingValue = currentInfo.Option;
                continue;
            }
            else
            {
                outOptions[currentInfo.Option] = "";
                continue;
            }
        }
        else if(!currentArg.empty() && currentArg[0] == '-')
        {
            ssLOG_ERROR("Invalid option: " << currentArg);
            return -1;
        }
        else
            break;
    }
    
    ssLOG_DEBUG("returning currentArgIndex: " << currentArgIndex);
    return currentArgIndex;
}

bool GenerateScriptTemplate(const std::string& outputFilePathStr)
{
    if(outputFilePathStr.empty())
    {
        ssLOG_ERROR("Missing output file path for -t/--create-script-template option");
        return false;
    }
    
    std::string defaultScriptInfo;
    runcpp2::GetDefaultScriptInfo(defaultScriptInfo);
    
    //Check if output filepath exists, if so check if it is a directory
    std::error_code e;
    if(ghc::filesystem::exists(outputFilePathStr, e))
    {
        if(ghc::filesystem::is_directory(outputFilePathStr, e))
        {
            ssLOG_ERROR(outputFilePathStr << " is a directory. " << 
                        "Cannot output script template to a directory");
            return false;
        }
        
        //If exists, check if it is a cpp/cc file.
        ghc::filesystem::path outputFilePath = outputFilePathStr;
        std::ifstream readOutputFile(outputFilePath);
        std::stringstream buffer;
        
        if(!readOutputFile)
        {
            ssLOG_ERROR("Failed to open file: " << outputFilePathStr);
            return false;
        }
        
        if(outputFilePath.extension() == ".cpp" || outputFilePath.extension() == ".cc")
        {
            //If so, prepend the script info template but wrapped in block comment
            buffer << "/* runcpp2" << std::endl << std::endl;
            buffer << defaultScriptInfo << std::endl;
            buffer << "*/" << std::endl << std::endl;
            buffer << readOutputFile.rdbuf();
        }
        //If not, check if it is yaml/yml. 
        else if(outputFilePath.extension() == ".yaml" || outputFilePath.extension() == ".yml")
        {
            //If so just prepend it normally
            buffer << defaultScriptInfo << std::endl << std::endl;
            buffer << readOutputFile.rdbuf();
        }
        //If not prepend it still but output a warning
        else
        {
            ssLOG_WARNING("Outputing script info template to non yaml file, is the intended?");
            buffer << defaultScriptInfo << std::endl << std::endl;
            buffer << readOutputFile.rdbuf();
        }
        
        readOutputFile.close();
        
        std::ofstream writeOutputFile(outputFilePath);
        if(!writeOutputFile)
        {
            ssLOG_ERROR("Failed to open file: " << outputFilePathStr);
            return false;
        }

        writeOutputFile << buffer.rdbuf();
    }
    //Otherwise write it to the file
    else
    {
        std::ofstream writeOutputFile(outputFilePathStr);
        if(!writeOutputFile)
        {
            ssLOG_ERROR("Failed to open file: " << outputFilePathStr);
            return false;
        }
        
        writeOutputFile << defaultScriptInfo;
    }
    
    return true;
}


int main(int argc, char* argv[])
{
    //Parse command line options
    int currentArgIndex = 0;
    std::unordered_map<runcpp2::CmdOptions, std::string> currentOptions;

    {
        static_assert(static_cast<int>(runcpp2::CmdOptions::COUNT) == 11, "Update this");
        std::unordered_map<std::string, runcpp2::OptionInfo> longOptionsMap =
        {
            {
                "--reset-cache", 
                runcpp2::OptionInfo(runcpp2::CmdOptions::RESET_CACHE, false)
            },
            {
                "--reset-user-config", 
                runcpp2::OptionInfo(runcpp2::CmdOptions::RESET_USER_CONFIG, false)
            },
            {
                "--executable", 
                runcpp2::OptionInfo(runcpp2::CmdOptions::EXECUTABLE, false)
            },
            {
                "--help", 
                runcpp2::OptionInfo(runcpp2::CmdOptions::HELP, false)
            },
            {
                "--remove-dependencies", 
                runcpp2::OptionInfo(runcpp2::CmdOptions::REMOVE_DEPENDENCIES, false)
            },
            {
                "--local",
                runcpp2::OptionInfo(runcpp2::CmdOptions::LOCAL, false)
            },
            {
                "--show-config-path",
                runcpp2::OptionInfo(runcpp2::CmdOptions::SHOW_USER_CONFIG, false)
            },
            {
                "--create-script-template",
                runcpp2::OptionInfo(runcpp2::CmdOptions::SCRIPT_TEMPLATE, true)
            },
            {
                "--watch",
                runcpp2::OptionInfo(runcpp2::CmdOptions::WATCH, false)
            },
            {
                "--build",
                runcpp2::OptionInfo(runcpp2::CmdOptions::BUILD, false)
            }
        };
        
        static_assert(static_cast<int>(runcpp2::CmdOptions::COUNT) == 11, "Update this");
        std::unordered_map<std::string, const runcpp2::OptionInfo&> shortOptionsMap = 
        {
            {"-r", longOptionsMap.at("--reset-cache")},
            {"-c", longOptionsMap.at("--reset-user-config")},
            {"-e", longOptionsMap.at("--executable")},
            {"-h", longOptionsMap.at("--help")},
            {"-d", longOptionsMap.at("--remove-dependencies")},
            {"-l", longOptionsMap.at("--local")},
            {"-s", longOptionsMap.at("--show-config-path")},
            {"-t", longOptionsMap.at("--create-script-template")},
            {"-w", longOptionsMap.at("--watch")},
            {"-b", longOptionsMap.at("--build")}
        };
        
        currentArgIndex = ParseArgs(longOptionsMap, shortOptionsMap, currentOptions, argc, argv);
        
        if(currentArgIndex == -1)
        {
            ssLOG_ERROR("Invalid option");
            return -1;
        }
        
        ++currentArgIndex;
    }
    
    //Help message
    if(currentOptions.count(runcpp2::CmdOptions::HELP))
    {
        static_assert(static_cast<int>(runcpp2::CmdOptions::COUNT) == 11, "Update this");
        ssLOG_BASE("Usage: runcpp2 [options] [input_file]");
        ssLOG_BASE("Options:");
        ssLOG_BASE("    -r, --[r]eset-cache                     Deletes all cache and build everything from scratch");
        ssLOG_BASE("    -c, --reset-user-[c]onfig               Replace current user config with the default one");
        ssLOG_BASE("    -e, --[e]xecutable                      Runs as executable instead of shared library");
        ssLOG_BASE("    -h, --[h]elp                            Show this help message");
        ssLOG_BASE("    -d, --remove-[d]ependencies             Remove dependencies listed in the script");
        ssLOG_BASE("    -l, --[l]ocal                           Build the script and dependencies locally");
        ssLOG_BASE("    -s, --[s]how-config-path                Show where runcpp2 is reading the config from");
        ssLOG_BASE("    -t, --create-script-[t]emplate <file>   Creates/prepend runcpp2 script info template");
        ssLOG_BASE("    -w, --[w]atch                           Watch script changes and output any compiling errors");
        ssLOG_BASE("    -b, --[b]uild                           Build the script and copy output files to the script's directory");
        
        return 0;
    }
    
    //Show user config path
    if(currentOptions.count(runcpp2::CmdOptions::SHOW_USER_CONFIG))
    {
        ssLOG_BASE(runcpp2::GetConfigFilePath());
        return 0;
    }
    
    //Resetting user config
    if(currentOptions.count(runcpp2::CmdOptions::RESET_USER_CONFIG))
    {
        ssLOG_INFO("Resetting user config");
        if(!runcpp2::WriteDefaultConfig(runcpp2::GetConfigFilePath()))
        {
            ssLOG_ERROR("Failed reset user config");
            return -1;
        }
        
        return 0;
    }
    
    //Generate script info template
    if(currentOptions.count(runcpp2::CmdOptions::SCRIPT_TEMPLATE))
    {
        if(!GenerateScriptTemplate(currentOptions.at(runcpp2::CmdOptions::SCRIPT_TEMPLATE)))
            return -1;
        else
            return 0;
    }
    
    std::vector<runcpp2::Data::Profile> profiles;
    std::string preferredProfile;
    
    if(!runcpp2::ReadUserConfig(profiles, preferredProfile))
    {
        ssLOG_ERROR("Failed read user config");
        return -1;
    }

    ssLOG_DEBUG("\nprofiles:");
    for(int i = 0; i < profiles.size(); ++i)
        ssLOG_DEBUG("\n" << profiles.at(i).ToString("    "));
    
    std::vector<std::string> scriptArgs;
    if(currentArgIndex >= argc)
    {
        ssLOG_ERROR("An input file is required");
        return 1;
    }
    
    std::string script = argv[currentArgIndex++];
    
    if(currentOptions.count(runcpp2::CmdOptions::WATCH) && currentArgIndex < argc)
        ssLOG_WARNING("-w/--watch doesn't run the script and doesn't except any run arguments");
    else
    {
        for(; currentArgIndex < argc; ++currentArgIndex)
        {
            ssLOG_DEBUG("argv[" << currentArgIndex << "]: " << argv[currentArgIndex]);
            scriptArgs.push_back(std::string(argv[currentArgIndex]));
        }
    }
    
    if( currentOptions.count(runcpp2::CmdOptions::BUILD) > 0 &&
        currentOptions.count(runcpp2::CmdOptions::WATCH) > 0)
    {
        ssLOG_ERROR("--build option is not compatible with --watch option");
        return -1;
    }
    
    runcpp2::Data::ScriptInfo parsedScriptInfo;
    
    if(currentOptions.count(runcpp2::CmdOptions::WATCH))
    {
        std::error_code e;
        if(!ghc::filesystem::exists(script, e))
        {
            ssLOG_ERROR("Script path " << script << " doesn't exist");
            return -1;
        }
        
        ghc::filesystem::file_time_type lastScriptWriteTime {};
        while(true)
        {
            runcpp2::Data::ScriptInfo* lastParsedScriptInfo = nullptr;
            
            if(ghc::filesystem::last_write_time(script, e) > lastScriptWriteTime)
            {
                int result = 0;
    
                runcpp2::PipelineResult pipelineResult = 
                    runcpp2::StartPipeline( script, 
                                            profiles, 
                                            preferredProfile, 
                                            currentOptions, 
                                            scriptArgs,
                                            lastParsedScriptInfo,
                                            parsedScriptInfo,
                                            "",
                                            result);
            
                static_assert(static_cast<int>(runcpp2::PipelineResult::COUNT) == 12, "Update this");
                switch(pipelineResult)
                {
                    case runcpp2::PipelineResult::INVALID_SCRIPT_PATH:
                    case runcpp2::PipelineResult::INVALID_CONFIG_PATH:
                    case runcpp2::PipelineResult::EMPTY_PROFILES:
                        ssLOG_ERROR("Watch failed");
                        return -1;
                    
                    case runcpp2::PipelineResult::UNEXPECTED_FAILURE:
                    case runcpp2::PipelineResult::SUCCESS:
                    case runcpp2::PipelineResult::INVALID_BUILD_DIR:
                    case runcpp2::PipelineResult::INVALID_SCRIPT_INFO:
                    case runcpp2::PipelineResult::NO_AVAILABLE_PROFILE:
                    case runcpp2::PipelineResult::DEPENDENCIES_FAILED:
                    case runcpp2::PipelineResult::COMPILE_LINK_FAILED:
                    case runcpp2::PipelineResult::INVALID_PROFILE:
                    case runcpp2::PipelineResult::RUN_SCRIPT_FAILED:
                        break;
                }
                ssLOG_BASE("No error. Watching...");
            }
            
            lastScriptWriteTime = ghc::filesystem::last_write_time(script, e);
            lastParsedScriptInfo = &parsedScriptInfo;
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
    
    int result = 0;
    
    std::string outputDir;
    if(currentOptions.count(runcpp2::CmdOptions::BUILD) > 0)
        outputDir = ghc::filesystem::path(script).parent_path().string();
    
    if(runcpp2::StartPipeline(  script, 
                                profiles, 
                                preferredProfile, 
                                currentOptions, 
                                scriptArgs,
                                nullptr,
                                parsedScriptInfo,
                                outputDir,
                                result) != runcpp2::PipelineResult::SUCCESS)
    {
        return -1;
    }
    
    return result;
}
