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
        
        //Matched long or short options
        if(longOptionsMap.count(currentArg) || shortOptionsMap.count(currentArg))
        {
            currentArgIndex = i;
            ssLOG_DEBUG("currentArgIndex: " << currentArgIndex);
            ssLOG_DEBUG("argv: " << argv[i]);
            const runcpp2::OptionInfo& currentInfo =    longOptionsMap.count(currentArg) ?
                                                        longOptionsMap.at(currentArg) :
                                                        shortOptionsMap.at(currentArg);
            if(currentInfo.ValueExists)
                optionForCapturingValue = currentInfo.Option;
            else
                outOptions[currentInfo.Option] = "";
            
            continue;
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
    ssLOG_SET_CURRENT_THREAD_TARGET_LEVEL(ssLOG_LEVEL_WARNING);
    
    //Parse command line options
    int currentArgIndex = 0;
    std::unordered_map<runcpp2::CmdOptions, std::string> currentOptions;
    {
        static_assert(static_cast<int>(runcpp2::CmdOptions::COUNT) == 19, "Update this");
        std::unordered_map<std::string, runcpp2::OptionInfo> longOptionsMap =
        {
            {
                "--rebuild", 
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
                "--reset-dependencies", 
                runcpp2::OptionInfo(runcpp2::CmdOptions::RESET_DEPENDENCIES, true)
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
            },
            {
                "--output",
                runcpp2::OptionInfo(runcpp2::CmdOptions::OUTPUT, true)
            },
            {
                "--version",
                runcpp2::OptionInfo(runcpp2::CmdOptions::VERSION, false)
            },
            {
                "--log-level",
                runcpp2::OptionInfo(runcpp2::CmdOptions::LOG_LEVEL, true)
            },
            {
                "--config",
                runcpp2::OptionInfo(runcpp2::CmdOptions::CONFIG_FILE, true)
            },
            {
                "--cleanup",
                runcpp2::OptionInfo(runcpp2::CmdOptions::CLEANUP, false)
            },
            {
                "--source-only",
                runcpp2::OptionInfo(runcpp2::CmdOptions::BUILD_SOURCE_ONLY, false)
            },
            {
                "--jobs",
                runcpp2::OptionInfo(runcpp2::CmdOptions::THREADS, true)
            },
            {
                "--tutorial",
                runcpp2::OptionInfo(runcpp2::CmdOptions::TUTORIAL, false)
            },
        };
        
        static_assert(static_cast<int>(runcpp2::CmdOptions::COUNT) == 19, "Update this");
        std::unordered_map<std::string, const runcpp2::OptionInfo&> shortOptionsMap = 
        {
            {"-rb", longOptionsMap.at("--rebuild")},
            {"-ru", longOptionsMap.at("--reset-user-config")},
            {"-e", longOptionsMap.at("--executable")},
            {"-h", longOptionsMap.at("--help")},
            {"-rd", longOptionsMap.at("--reset-dependencies")},
            {"-l", longOptionsMap.at("--local")},
            {"-sc", longOptionsMap.at("--show-config-path")},
            {"-t", longOptionsMap.at("--create-script-template")},
            {"-w", longOptionsMap.at("--watch")},
            {"-b", longOptionsMap.at("--build")},
            {"-o", longOptionsMap.at("--output")},
            {"-v", longOptionsMap.at("--version")},
            {"-c", longOptionsMap.at("--config")},
            {"-cu", longOptionsMap.at("--cleanup")},
            {"-s", longOptionsMap.at("--source-only")},
            {"-j", longOptionsMap.at("--jobs")},
        };
        
        currentArgIndex = ParseArgs(longOptionsMap, shortOptionsMap, currentOptions, argc, argv);
        
        if(currentArgIndex == -1)
        {
            ssLOG_ERROR("Failed to parse arguments. See --help for details");
            return -1;
        }
        
        ++currentArgIndex;
    }
    
    //Help message
    if(currentOptions.count(runcpp2::CmdOptions::HELP))
    {
        static_assert(static_cast<int>(runcpp2::CmdOptions::COUNT) == 19, "Update this");
        ssLOG_BASE("Usage: runcpp2 [options] [input_file]");
        ssLOG_BASE("Options:");
        ssLOG_BASE("  Run/Build:");
        ssLOG_BASE("    -b,  --[b]uild                          Build the script but don't run it");
        ssLOG_BASE("    -o,  --[o]utput <Output Dir>            Output files to the directory specified, must be used with --build");
        ssLOG_BASE("    -w,  --[w]atch                          Watch script changes and output any compiling errors");
        ssLOG_BASE("    -l,  --[l]ocal                          Build in the current working directory under .runcpp2 directory");
        ssLOG_BASE("    -e,  --[e]xecutable                     Runs as executable instead of shared library");
        ssLOG_BASE("    -c,  --[c]onfig <file>                  Use specified config file instead of default");
        ssLOG_BASE("    -t,  --create-script-[t]emplate <file>  Creates/prepend runcpp2 script info template");
        ssLOG_BASE("    -s,  --[s]ource-only                    (Re)Builds source files only without building dependencies.");
        ssLOG_BASE("                                                The previous built binaries will be used for dependencies.");
        ssLOG_BASE("                                                Requires dependencies to be built already.");
        ssLOG_BASE("    -j,  --[j]obs                           Maximum number of threads running. Defaults to 8");
        ssLOG_BASE("  Reset/Cleanup:");
        ssLOG_BASE("    -rb, --[r]e[b]uild                      Deletes compiled source files cache and rebuild");
        ssLOG_BASE("    -ru, --[r]eset-[u]ser-config            Replace current user config with the default one");
        ssLOG_BASE("    -rd, --[r]eset-[d]ependencies <names>   Reset dependencies (comma-separated names, or \"all\" for all)");
        ssLOG_BASE("    -cu, --[c]lean[u]p                      Run cleanup commands and remove build directory");
        ssLOG_BASE("  Settings:");
        ssLOG_BASE("    -sc, --[s]how-[c]onfig-path             Show where runcpp2 is reading the config from");
        ssLOG_BASE("    -v,  --[v]ersion                        Show the version of runcpp2");
        ssLOG_BASE("    -h,  --[h]elp                           Show this help message");
        ssLOG_BASE("         --log-level <level>                Sets the log level (Normal, Info, Debug) for runcpp2");
        ssLOG_BASE("  Others:");
        ssLOG_BASE("         --tutorial                         Start interactive tutorial");
        return 0;
    }
    
    //Set Log level
    if(currentOptions.count(runcpp2::CmdOptions::LOG_LEVEL))
    {
        std::string level = currentOptions.at(runcpp2::CmdOptions::LOG_LEVEL);
        runcpp2::Trim(level);
        for(int i = 0; i < level.size(); ++i)
            level[i] = std::tolower(level[i]);
        
        if(level == "info")
            ssLOG_SET_CURRENT_THREAD_TARGET_LEVEL(ssLOG_LEVEL_INFO);
        else if(level == "debug")
            ssLOG_SET_CURRENT_THREAD_TARGET_LEVEL(ssLOG_LEVEL_DEBUG);
        else if(level == "normal")
            ssLOG_SET_CURRENT_THREAD_TARGET_LEVEL(ssLOG_LEVEL_WARNING);
        else
        {
            ssLOG_ERROR("Invalid level: " << level);
            return -1;
        }
    }
    
    ssLOG_FUNC_INFO();
    INTERNAL_RUNCPP2_SAFE_START();
    
    //Show user config path
    if(currentOptions.count(runcpp2::CmdOptions::SHOW_USER_CONFIG))
    {
        ssLOG_BASE(runcpp2::GetConfigFilePath());
        return 0;
    }

    //Check if the version flag is present
    if(currentOptions.count(runcpp2::CmdOptions::VERSION))
    {
        ssLOG_BASE("runcpp2 version " << RUNCPP2_VERSION);
        return 0;
    }
    
    //Download tutorial
    if(currentOptions.count(runcpp2::CmdOptions::TUTORIAL))
    {
        if(!runcpp2::DownloadTutorial(argv[0]))
            return -1;
        return 0;
    }
    
    //Resetting user config
    if(currentOptions.count(runcpp2::CmdOptions::RESET_USER_CONFIG))
    {
        ssLOG_INFO("Resetting user config");
        if(!runcpp2::WriteDefaultConfigs(runcpp2::GetConfigFilePath(), true, true))
        {
            ssLOG_ERROR("Failed reset user config");
            return -1;
        }
        ssLOG_BASE("User config reset successful");
        return 0;
    }
    
    //TODO: Rename this to build template
    //Generate script info template
    if(currentOptions.count(runcpp2::CmdOptions::SCRIPT_TEMPLATE))
    {
        if(!GenerateScriptTemplate(currentOptions.at(runcpp2::CmdOptions::SCRIPT_TEMPLATE)))
            return -1;
        else
        {
            ssLOG_BASE("Script template generated");
            return 0;
        }
    }
    
    std::vector<runcpp2::Data::Profile> profiles;
    std::string preferredProfile;
    
    std::string configPath;
    if(currentOptions.count(runcpp2::CmdOptions::CONFIG_FILE))
        configPath = currentOptions.at(runcpp2::CmdOptions::CONFIG_FILE);

    if(!runcpp2::ReadUserConfig(profiles, preferredProfile, configPath))
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
        ssLOG_ERROR("An input file is required. See --help for details");
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
    
    if( currentOptions.count(runcpp2::CmdOptions::OUTPUT) > 0 &&
        currentOptions.count(runcpp2::CmdOptions::BUILD) == 0)
    {
        ssLOG_ERROR("--build option must be supplied when specifying output directory");
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
        
        runcpp2::Data::ScriptInfo* lastParsedScriptInfo = nullptr;
        ghc::filesystem::file_time_type lastFinalSourceWriteTime;
        ghc::filesystem::file_time_type lastFinalIncludeWriteTime;
        bool needsRunning = true;  //First run always needs running
        
        while(true)
        {
            //Check if sources need update
            bool needsUpdate = false;
            if(!needsRunning)  //Skip check on first run
            {
                if(runcpp2::CheckSourcesNeedUpdate( script,
                                                    profiles,
                                                    preferredProfile,
                                                    lastParsedScriptInfo ? 
                                                        *lastParsedScriptInfo : 
                                                        parsedScriptInfo,
                                                    currentOptions,
                                                    lastFinalSourceWriteTime,
                                                    lastFinalIncludeWriteTime,
                                                    needsUpdate) != runcpp2::PipelineResult::SUCCESS)
                {
                    ssLOG_ERROR("Failed to check if sources need update");
                    return -1;
                }
                
                if(needsUpdate)
                {
                    ssLOG_INFO("Source files have changed");
                    needsRunning = true;
                }
            }

            if(needsRunning)
            {
                //Clear the screen
                if(ssLOG_GET_CURRENT_THREAD_TARGET_LEVEL() <= ssLOG_LEVEL_WARNING)
                {
                    #if defined(_WIN32)
                        system("cls");
                    #else
                        //https://stackoverflow.com/a/53925508
                        std::cout << "\033c";
                    #endif
                }
                
                ssLOG_LINE("Changes detected, running...");
                
                int result = 0;
                runcpp2::PipelineResult pipelineResult = 
                    runcpp2::StartPipeline( script, 
                                            profiles, 
                                            preferredProfile, 
                                            currentOptions, 
                                            scriptArgs,
                                            lastParsedScriptInfo,
                                            "",
                                            parsedScriptInfo,
                                            lastFinalSourceWriteTime,
                                            lastFinalIncludeWriteTime,
                                            result);
            
                static_assert(static_cast<int>(runcpp2::PipelineResult::COUNT) == 13, "Update this");
                switch(pipelineResult)
                {
                    case runcpp2::PipelineResult::INVALID_SCRIPT_PATH:
                    case runcpp2::PipelineResult::INVALID_CONFIG_PATH:
                    case runcpp2::PipelineResult::EMPTY_PROFILES:
                        ssLOG_ERROR("Watch failed");
                        return -1;
                    
                    case runcpp2::PipelineResult::UNEXPECTED_FAILURE:
                    case runcpp2::PipelineResult::INVALID_BUILD_DIR:
                    case runcpp2::PipelineResult::INVALID_SCRIPT_INFO:
                    case runcpp2::PipelineResult::NO_AVAILABLE_PROFILE:
                    case runcpp2::PipelineResult::DEPENDENCIES_FAILED:
                    case runcpp2::PipelineResult::COMPILE_LINK_FAILED:
                    case runcpp2::PipelineResult::INVALID_PROFILE:
                    case runcpp2::PipelineResult::RUN_SCRIPT_FAILED:
                    case runcpp2::PipelineResult::INVALID_OPTION:
                        ssLOG_LINE("Watching...");
                        break;
                    case runcpp2::PipelineResult::SUCCESS:
                        ssLOG_LINE("No error. Watching...");
                        break;
                }
                
                lastParsedScriptInfo = &parsedScriptInfo;
                needsRunning = false;
            }

            std::this_thread::sleep_for(std::chrono::seconds(5));
        } //while(true)
    } //if(currentOptions.count(runcpp2::CmdOptions::WATCH))
    
    int result = 0;
    
    std::string outputDir;
    if( currentOptions.count(runcpp2::CmdOptions::BUILD) > 0 &&
        currentOptions.count(runcpp2::CmdOptions::OUTPUT) > 0)
    {
        auto buildOutputDir = ghc::filesystem::path(currentOptions.at(runcpp2::CmdOptions::OUTPUT));
        if(buildOutputDir.is_absolute())
            outputDir = buildOutputDir.string();
        else
            outputDir = (ghc::filesystem::current_path() / buildOutputDir).string();
    }
    
    ghc::filesystem::file_time_type finalSourceWriteTime;
    ghc::filesystem::file_time_type finalIncludeWriteTime;
    if(runcpp2::StartPipeline(  script, 
                                profiles, 
                                preferredProfile, 
                                currentOptions, 
                                scriptArgs,
                                nullptr,
                                outputDir,
                                parsedScriptInfo,
                                finalSourceWriteTime,
                                finalIncludeWriteTime,
                                result) != runcpp2::PipelineResult::SUCCESS)
    {
        return -1;
    }
    
    std::vector<std::string> actions;
    if(currentOptions.count(runcpp2::CmdOptions::RESET_CACHE) > 0)
        actions.push_back("Rebuild");
    if(currentOptions.count(runcpp2::CmdOptions::CLEANUP) > 0)
        actions.push_back("Cleanup");
    if(currentOptions.count(runcpp2::CmdOptions::RESET_DEPENDENCIES) > 0)
        actions.push_back("Dependencies Reset");
    if(currentOptions.count(runcpp2::CmdOptions::BUILD) > 0)
        actions.push_back("Build");
    
    std::string action;
    if(!actions.empty())
    {
        action = actions.front();
        for(int i = 1; i < actions.size(); ++i)
            action += ", " + actions[i];
        
        ssLOG_BASE(action << " success");
    }
    return result;
    
    INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(-1);
}

