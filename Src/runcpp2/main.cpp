#include "runcpp2/ConfigParsing.hpp"
#include "runcpp2/StringUtil.hpp"
#include "runcpp2/runcpp2.hpp"

#include "ssLogger/ssLog.hpp"

int ParseArgs(  const std::unordered_map<std::string, runcpp2::OptionInfo>& optionsMap,
                std::unordered_map<runcpp2::CmdOptions, std::string>& outOptions,
                int argc, 
                char* argv[])
{
    int currentArgIndex = 0;
    runcpp2::CmdOptions currentOption = runcpp2::CmdOptions::NONE;
    
    for(int i = 1; i < argc; ++i)
    {
        //Storing value for last option
        if(currentOption != runcpp2::CmdOptions::NONE)
        {
            if(optionsMap.find(std::string(argv[i])) != optionsMap.end())
            {
                for(auto it = optionsMap.begin(); it != optionsMap.end(); ++it)
                {
                    if(it->second.Option == currentOption)
                    {
                        ssLOG_ERROR("Missing value for option: " << it->first);
                        return -1;
                    }
                }
                
                return -1;
            }
            
            outOptions[currentOption] = std::string(argv[i]);
            currentOption = runcpp2::CmdOptions::NONE;
            currentArgIndex = i;
            ssLOG_DEBUG("currentArgIndex: " << currentArgIndex);
            ssLOG_DEBUG("argv: " << argv[i]);
            continue;
        }
        
        //Checking for options
        if(optionsMap.find(std::string(argv[i])) != optionsMap.end())
        {
            currentArgIndex = i;
            ssLOG_DEBUG("currentArgIndex: " << currentArgIndex);
            ssLOG_DEBUG("argv: " << argv[i]);
            
            static_assert(  (int)runcpp2::CmdOptions::COUNT == 4, 
                            "Add a case for the new runcpp2_CmdOptions");
            
            if(optionsMap.at(std::string(argv[i])).HasValue)
            {
                currentOption = optionsMap.at(std::string(argv[i])).Option;
                continue;
            }
            else
            {
                outOptions[optionsMap.at(std::string(argv[i])).Option] = "";
                continue;
            }
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
    {
        std::unordered_map<std::string, runcpp2::OptionInfo> optionsMap =
        {
            {
                "--reset-dependencies", 
                runcpp2::OptionInfo(runcpp2::CmdOptions::RESET_DEPENDENCIES, false, "")
            },
            {
                "--reset-user-config", 
                runcpp2::OptionInfo(runcpp2::CmdOptions::RESET_USER_CONFIG, false)
            },
            {
                "--executable", 
                runcpp2::OptionInfo(runcpp2::CmdOptions::EXECUTABLE, false)
            },
        };
        
        currentArgIndex = ParseArgs(optionsMap, currentOptions, argc, argv);
        
        if(currentArgIndex == -1)
        {
            ssLOG_FATAL("Invalid option");
            return -1;
        }
        
        ++currentArgIndex;
    }
    
    //Resetting user config
    if(currentOptions.find(runcpp2::CmdOptions::RESET_USER_CONFIG) != currentOptions.end())
    {
        ssLOG_INFO("Resetting user config");
        if(!runcpp2::WriteDefaultConfig(runcpp2::GetConfigFilePath()))
        {
            ssLOG_FATAL("Failed reset user config");
            return -1;
        }
        
        return 0;
    }
    
    std::vector<runcpp2::Data::Profile> profiles;
    std::string preferredProfile;
    
    if(!runcpp2::ReadUserConfig(profiles, preferredProfile))
    {
        ssLOG_FATAL("Failed read user config");
        return -1;
    }

    ssLOG_DEBUG("\nprofiles:");
    for(int i = 0; i < profiles.size(); ++i)
        ssLOG_DEBUG("\n" << profiles[i].ToString("    "));
    
    std::vector<std::string> scriptArgs;
    if(currentArgIndex >= argc)
    {
        ssLOG_FATAL("An input file is required");
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