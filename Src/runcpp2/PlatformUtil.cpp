#include "runcpp2/PlatformUtil.hpp"

#include "ssLogger/ssLog.hpp"
#include "ghc/filesystem.hpp"
#include <stdio.h>
#include <cctype>

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
    
    ssLOG_DEBUG("outOutput: \n" << outOutput.c_str());
    
    if(outReturnCode != 0)
    {
        ssLOG_DEBUG("Failed when running command with return code: " << outReturnCode);
        return false;
    }
    
    return true;
}


#if defined(_WIN32)
    //Credit: https://stackoverflow.com/a/17387176
    std::string runcpp2::GetWindowsError()
    {
        //Get the error message ID, if any.
        DWORD errorMessageID = ::GetLastError();
        if(errorMessageID == 0) {
            return std::string(); //No error message has been recorded
        }
        
        LPSTR messageBuffer = nullptr;
    
        //Ask Win32 to give us the string version of that message ID.
        //The parameters we pass in, tell Win32 to create the buffer that holds the message for us (because we don't yet know how long the message string will be).
        size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                     NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);
        
        //Copy the error message into a std::string.
        std::string message(messageBuffer, size);
        
        //Free the Win32's string's buffer.
        LocalFree(messageBuffer);
                
        return message;
    }
#endif

std::string runcpp2::GetFileExtensionWithoutVersion(const ghc::filesystem::path& path)
{
    std::string filename = path.filename().string();
    bool inNumericPart = true;
    int lastDotPos = filename.length();
    
    for(int i = filename.length() - 1; i >= 0; --i)
    {
        if(filename[i] == '.')
        {
            if(!inNumericPart)
                return filename.substr(i, lastDotPos - i);
            
            inNumericPart = true;
            lastDotPos = i;
        }
        else if(!std::isdigit(filename[i]))
            inNumericPart = false;
    }
    
    return "";
}
