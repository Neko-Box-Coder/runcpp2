#include "runcpp2/ConfigParsing.hpp"
#include "runcpp2/StringUtil.hpp"
#include "runcpp2/runcpp2.hpp"

#include "ssLogger/ssLog.hpp"



enum class runcpp2_CmdOptions
{
    NONE,
    SETUP,
    COUNT
};


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
    
    std::unordered_map<std::string, runcpp2_CmdOptions> optionsMap =
    {
        {"--setup", runcpp2_CmdOptions::SETUP}
    };
    
    runcpp2_CmdOptions currentOption = runcpp2_CmdOptions::NONE;
    (void)currentOption;
    
    int currentArgIndex = 1;
    if(optionsMap.find(std::string(argv[currentArgIndex])) != optionsMap.end())
    {
        static_assert((int)runcpp2_CmdOptions::COUNT == 2, "Add a case for the new runcpp2_CmdOptions");
        
        switch(optionsMap.at(std::string(argv[1])))
        {
            case runcpp2_CmdOptions::NONE:
            case runcpp2_CmdOptions::COUNT:
                break;
            case runcpp2_CmdOptions::SETUP:
                currentOption = runcpp2_CmdOptions::SETUP;
                ++currentArgIndex;
                break;
        }
    }
    
    //TODO(NOW): Use command line option
    
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
    
    if(!runcpp2::RunScript(script, compilerProfiles, preferredProfile, scriptArgs))
    {
        ssLOG_FATAL("Failed to run script: " << script);
        return 2;
    }
    
    return 0;
}