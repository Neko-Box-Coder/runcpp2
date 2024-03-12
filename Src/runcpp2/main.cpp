#include "runcpp2/ConfigParsing.hpp"
#include "runcpp2/StringUtil.hpp"
#include "runcpp2/runcpp2.hpp"

#include "ssLogger/ssLog.hpp"


//Usage: runcpp2 [--setup] input_file [args]
int main(int argc, char* argv[])
{
    std::vector<runcpp2::CompilerProfile> compilerProfiles;
    std::string preferredProfile;
    
    if(!runcpp2::ReadUserConfig(compilerProfiles, preferredProfile))
    {
        return -1;
    }
    
    ssLOG_DEBUG("\nCompilerProfiles:");
    for(int i = 0; i < compilerProfiles.size(); ++i)
        ssLOG_DEBUG("\n" << compilerProfiles[i].ToString("    "));
    
    if(argc < 2)
    {
        ssLOG_FATAL("An input file is required");
        return 1;
    }
    
    std::unordered_map<std::string, runcpp2::CmdOptions> optionsMap =
    {
        {"--setup", runcpp2::CmdOptions::SETUP}
    };
    
    std::unordered_map<runcpp2::CmdOptions, std::string> currentOptions;
    int currentArgIndex = 0;
    for(int i = 1; i < argc; ++i)
    {
        if(optionsMap.find(std::string(argv[i])) != optionsMap.end())
        {
            currentArgIndex = i;
            
            static_assert((int)runcpp2::CmdOptions::COUNT == 2, "Add a case for the new runcpp2_CmdOptions");
            switch(optionsMap[std::string(argv[1])])
            {
                case runcpp2::CmdOptions::SETUP:
                    currentOptions[runcpp2::CmdOptions::SETUP] = "";
                    break;
                default:
                    break;
            }
        }
        else
            break;
    }
    
    ++currentArgIndex;
    std::vector<std::string> scriptArgs;
    if(currentArgIndex >= argc)
    {
        ssLOG_FATAL("An input file is required");
        return 1;
    }
    
    std::string script = argv[currentArgIndex++];
    
    for(; currentArgIndex < argc; ++currentArgIndex)
    {
        ssLOG_LINE("argv[" << currentArgIndex << "]: " << argv[currentArgIndex]);
        scriptArgs.push_back(std::string(argv[currentArgIndex]));
    }
    
    if(!runcpp2::RunScript(script, compilerProfiles, preferredProfile, currentOptions, scriptArgs))
    {
        ssLOG_FATAL("Failed to run script: " << script);
        return 2;
    }
    
    return 0;
}