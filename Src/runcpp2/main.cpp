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
    #if 0
        System2CommandInfo commandInfo;
        const char* command = "ls -lah";
        
        SYSTEM2_RESULT system2Result;
        
        {
            system2Result = System2Run(command, &commandInfo);
            if(system2Result != SYSTEM2_RESULT_SUCCESS)
            {
                ssLOG_ERROR("System2Run failed: " << system2Result);
                return 1;
            }
        }
        
        {
            char outputBuffer[1024] = {0};
            uint32_t bytesRead = 0;
            system2Result = System2ReadFromOutput(&commandInfo, outputBuffer, 1023, &bytesRead);
            
            if(system2Result != SYSTEM2_RESULT_SUCCESS)
            {
                ssLOG_ERROR("System2ReadFromOutput failed: " << system2Result);
                return 2;
            }
            
            //Make sure to null terminate the outputBuffer after reading the data.
            outputBuffer[bytesRead] = '\0';
            
            ssLOG_LINE("Output of command: \n" << outputBuffer);
        }
        
        {
            int returnCode = 0;
            system2Result = System2GetCommandReturnValueSync(&commandInfo, &returnCode);
            
            if(system2Result != SYSTEM2_RESULT_SUCCESS)
            {
                ssLOG_ERROR("System2GetCommandReturnValueSync failed: " << system2Result);
                return 3;
            }
            
            ssLOG_LINE("Return value of command: " << returnCode);
        }
        
        return 0;
    #elif 0
        //Test string split
        
        std::string testString = "This.is.a.test.string.to.split.";
        
        std::vector<std::string> splittedStrings;
        
        runcpp2::Internal::SplitString( testString, 
                                        ".", 
                                        splittedStrings);
        
        for(int i = 0; i < splittedStrings.size(); ++i)
        {
            ssLOG_LINE("splittedStrings[" << i << "]: " << splittedStrings[i]);
        }
        return 0;
    #endif
    
    std::vector<runcpp2::CompilerProfile> compilerProfiles;
    std::string preferredProfile;
    
    if(!runcpp2::ReadUserConfig(compilerProfiles, preferredProfile))
    {
        return -1;
    }
    
    ssLOG_LINE("\nCompilerProfiles:");
    for(int i = 0; i < compilerProfiles.size(); ++i)
        ssLOG_LINE("\n" << compilerProfiles[i].ToString("    "));
    
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
    
    std::string scriptArgs = "";
    if(currentArgIndex >= argc)
    {
        ssLOG_FATAL("An input file is required");
        return 1;
    }
    
    std::string script = argv[currentArgIndex++];
    
    for(; currentArgIndex < argc; ++currentArgIndex)
        scriptArgs += " " + std::string(argv[currentArgIndex]);
    
    if(!runcpp2::RunScript(script, compilerProfiles, preferredProfile, scriptArgs))
    {
        ssLOG_FATAL("Failed to run script: " << script);
        return 2;
    }
    
    return 0;
}