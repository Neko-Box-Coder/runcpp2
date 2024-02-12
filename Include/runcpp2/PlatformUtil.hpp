#ifndef RUNCPP2_PLATFORM_UTIL_HPP
#define RUNCPP2_PLATFORM_UTIL_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace runcpp2
{

namespace Internal
{
    char GetAltFileSystemSeparator();
    
    char GetFileSystemSeparator();

    std::string ProcessPath(const std::string& path);

    bool FileOrDirectoryExists(const std::string& path, bool& outIsDir);
    
    std::string GetFileDirectory(const std::string& filePath);
    
    std::string GetFileNameWithExtension(const std::string& filePath);
    
    std::string GetFileNameWithoutExtension(const std::string& filePath);
    
    std::string GetFileExtension(const std::string& filePath);
    
    std::vector<std::string> GetPlatformNames();
}

}

#ifdef __unix__
    struct System2CommandInfo
    {
        int ParentToChildPipes[2];
        int ChildToParentPipes[2];
        pid_t ChildProcessID;
    };
    
    enum SYSTEM2_PIPE_FILE_DESCRIPTORS
    {
        SYSTEM2_FD_READ = 0,
        SYSTEM2_FD_WRITE = 1
    };
    
    enum SYSTEM2_RESULT
    {
        SYSTEM2_RESULT_COMMAND_TERMINATED = 3,
        SYSTEM2_RESULT_COMMAND_NOT_FINISHED = 2,
        SYSTEM2_RESULT_READ_NOT_FINISHED = 1,
        SYSTEM2_RESULT_SUCCESS = 0,
        SYSTEM2_RESULT_PIPE_CREATE_FAILED = -1,
        SYSTEM2_RESULT_PIPE_FD_CLOSE_FAILED = -2,
        SYSTEM2_RESULT_FORK_FAILED = -3,
        SYSTEM2_RESULT_READ_FAILED = -4,
        SYSTEM2_RESULT_WRITE_FAILED = -5,
        SYSTEM2_RESULT_COMMAND_WAIT_FAILED = -6,
    };
#endif

SYSTEM2_RESULT System2Run(const char* command, System2CommandInfo* outCommandInfo);
SYSTEM2_RESULT System2ReadFromOutput(   System2CommandInfo* info, 
                                        char* outputBuffer, 
                                        uint32_t outputBufferSize,
                                        uint32_t* outBytesRead);

SYSTEM2_RESULT System2WriteToInput( System2CommandInfo* info, 
                                    const char* inputBuffer, 
                                    const uint32_t inputBufferSize);

SYSTEM2_RESULT System2GetCommandReturnValueAsync(   System2CommandInfo* info, 
                                                    int* outReturnCode);

SYSTEM2_RESULT System2GetCommandReturnValueSync(System2CommandInfo* info, 
                                                int* outReturnCode);




#endif