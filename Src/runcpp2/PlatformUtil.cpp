#include "runcpp2/PlatformUtil.hpp"

#include "ssLogger/ssLog.hpp"
#include <stdio.h>

namespace
{
    char GetAltFileSystemSeparator()
    {
        #ifdef _WIN32
            return '/';
        #elif __unix__
            return '\0';
        #else
            return '\0';
        #endif
    }

    char GetFileSystemSeparator()
    {
        #ifdef _WIN32
            return '\\';
        #elif __unix__
            return '/';
        #else
            return '/';
        #endif
    }
}

std::string runcpp2::ProcessPath(const std::string& path)
{
    std::string processedScriptPath;
    
    char altSeparator = GetAltFileSystemSeparator();
    
    //Convert alternative slashes to native slashes
    if(altSeparator != '\0')
    {
        char separator = GetFileSystemSeparator();
        
        processedScriptPath = path;
        for(int i = 0; i < processedScriptPath.size(); ++i)
        {
            if(processedScriptPath[i] == altSeparator)
                processedScriptPath[i] = separator;
        }
        
        return processedScriptPath;
    }
    else
        return path;
}

std::vector<std::string> runcpp2::GetPlatformNames()
{
    #ifdef _WIN32
        return {"Windows", "Default"};
    #elif __linux__
        return {"Linux", "Unix", "Default"};
    #elif __APPLE__
        return {"MacOS", "Unix", "Default"};
    #elif __unix__
        return {"Unix", "Default"};
    #else
        return {"Unknown", "Default"};
    #endif
}

bool runcpp2::RunCommandAndGetOutput(   const std::string& command, 
                                        std::string& outOutput, 
                                        std::string runDirectory)
{
    int returnCode;
    return RunCommandAndGetOutput(command, outOutput, returnCode, runDirectory);
}

bool runcpp2::RunCommandAndGetOutput(   const std::string& command, 
                                        std::string& outOutput, 
                                        int& outReturnCode,
                                        std::string runDirectory)
{
    ssLOG_FUNC_DEBUG();
    ssLOG_DEBUG("Running: " << command);
    System2CommandInfo commandInfo = {};
    if(!runDirectory.empty())
        commandInfo.RunDirectory = runDirectory.c_str();
    
    commandInfo.RedirectOutput = true;
    SYSTEM2_RESULT sys2Result = System2Run(command.c_str(), &commandInfo);
    
    if(sys2Result != SYSTEM2_RESULT_SUCCESS)
    {
        ssLOG_ERROR("System2Run failed with result: " << sys2Result);
        return false;
    }
    outOutput.clear();
    
    do
    {
        uint32_t byteRead = 0;
        outOutput.resize(outOutput.size() + 4096);
        
        sys2Result = 
            System2ReadFromOutput(  &commandInfo, 
                                    const_cast<char*>(outOutput.data()) + outOutput.size() - 4096,
                                    4096, 
                                    &byteRead);

        outOutput.resize(outOutput.size() + byteRead);
    }
    while(sys2Result == SYSTEM2_RESULT_READ_NOT_FINISHED);
    
    sys2Result = System2GetCommandReturnValueSync(&commandInfo, &outReturnCode);
    if(sys2Result != SYSTEM2_RESULT_SUCCESS)
    {
        ssLOG_ERROR("System2GetCommandReturnValueSync failed with result: " << sys2Result);
        return false;
    }
    
    if(outReturnCode != 0)
    {
        ssLOG_DEBUG("Failed when running command");
        ssLOG_DEBUG("outOutput: \n" << outOutput.c_str());
        return false;
    }
    
    return true;
}
