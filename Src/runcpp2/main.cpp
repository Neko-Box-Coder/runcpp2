#include "runcpp2/ConfigParsing.hpp"
#include "runcpp2/StringUtil.hpp"
#include "runcpp2/runcpp2.hpp"

#include "ssLogger/ssLog.hpp"

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
            if( longOptionsMap.count(currentArg) || shortOptionsMap.count(currentArg))
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
            
            static_assert(  (int)runcpp2::CmdOptions::COUNT == 5, 
                            "Add a case for the new runcpp2_CmdOptions");
            
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

//Usage: runcpp2 [--setup] input_file [args]
int main(int argc, char* argv[])
{
    //Parse command line options
    int currentArgIndex = 0;
    std::unordered_map<runcpp2::CmdOptions, std::string> currentOptions;
    //std::unordered_set<runcpp2::CmdOptions> currentOptions;

    {
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
        };
        
        std::unordered_map<std::string, const runcpp2::OptionInfo&> shortOptionsMap = 
        {
            {"-r", longOptionsMap.at("--reset-cache")},
            {"-c", longOptionsMap.at("--reset-user-config")},
            {"-e", longOptionsMap.at("--executable")},
            {"-h", longOptionsMap.at("--help")},
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
        ssLOG_BASE("Usage: runcpp2 [options] [input_file]");
        ssLOG_BASE("Options:");
        ssLOG_BASE("    -r, --reset-cache           Deletes all cache and build everything from scratch");
        ssLOG_BASE("    -c, --reset-user-config     Replace current user config with the default one");
        ssLOG_BASE("    -e, --executable            Runs as executable instead of shared library");
        ssLOG_BASE("    -h, --help                  Show this help message");
        
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
    
    std::vector<runcpp2::Data::Profile> profiles;
    std::string preferredProfile;
    
    if(!runcpp2::ReadUserConfig(profiles, preferredProfile))
    {
        ssLOG_ERROR("Failed read user config");
        return -1;
    }

    ssLOG_DEBUG("\nprofiles:");
    for(int i = 0; i < profiles.size(); ++i)
        ssLOG_DEBUG("\n" << profiles[i].ToString("    "));
    
    std::vector<std::string> scriptArgs;
    if(currentArgIndex >= argc)
    {
        ssLOG_ERROR("An input file is required");
        return 1;
    }
    
    std::string script = argv[currentArgIndex++];
    
    for(; currentArgIndex < argc; ++currentArgIndex)
    {
        ssLOG_DEBUG("argv[" << currentArgIndex << "]: " << argv[currentArgIndex]);
        scriptArgs.push_back(std::string(argv[currentArgIndex]));
    }
    
    int result = runcpp2::RunScript(script, 
                                    profiles, 
                                    preferredProfile, 
                                    currentOptions, 
                                    scriptArgs);
    
    return result;
}
