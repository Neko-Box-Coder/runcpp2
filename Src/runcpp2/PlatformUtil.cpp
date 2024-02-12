#include "runcpp2/PlatformUtil.hpp"

#include "tinydir.h"

#ifdef __unix__
    #include <unistd.h>
    #include <sys/wait.h>
#endif

#include <stdio.h>

namespace runcpp2
{

namespace Internal
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

    std::string ProcessPath(const std::string& path)
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

    bool FileOrDirectoryExists(const std::string& path, bool& outIsDir)
    {
        tinydir_file inputFile;
            
        if(tinydir_file_open(&inputFile, path.c_str()) != 0)
            return false;
        
        outIsDir = inputFile.is_dir;
    
        return true;
    }
    
    std::string GetFileDirectory(const std::string& filePath)
    {
        char separator = GetFileSystemSeparator();
        
        std::size_t lastSlashIndex = filePath.rfind(separator);
        std::string scriptDirectory;
        if(lastSlashIndex == std::string::npos)
            scriptDirectory = ".";
        else
        {
            while(lastSlashIndex > 1 && filePath.at(lastSlashIndex - 1) == separator)
                --lastSlashIndex;
            
            scriptDirectory = filePath.substr(0, lastSlashIndex);
        }
        
        scriptDirectory += separator;
        return scriptDirectory;
    }
    
    std::string GetFileNameWithExtension(const std::string& filePath)
    {
        char separator = GetFileSystemSeparator();
        
        std::size_t lastSlashIndex = filePath.rfind(separator);
        
        std::string filename;
        if(lastSlashIndex != std::string::npos)
            filename = filename.substr(lastSlashIndex + 1);
        
        return filename;
    }

    std::string GetFileNameWithoutExtension(const std::string& filePath)
    {
        std::string filename = GetFileNameWithExtension(filePath);
        
        char separator = GetFileSystemSeparator();
        
        std::size_t lastDotIndex = filePath.rfind(".");
        
        if(lastDotIndex == std::string::npos)
            filename = filePath;
        else
            filename = filePath.substr(0, lastDotIndex);
        
        return filename;
    }
    
    std::string GetFileExtension(const std::string& filePath)
    {
        std::size_t lastDotIndex = filePath.rfind(".");
        std::string extension;
        
        if(lastDotIndex == std::string::npos)
            extension = "";
        else
            extension = filePath.substr(lastDotIndex+1);
        
        return extension;
    }
    
    
    std::vector<std::string> GetPlatformNames()
    {
        #ifdef _WIN32
            return {"Windows"};
        #elif __linux__
            return {"Linux", "Unix"};
        #elif __APPLE__
            return {"MacOS", "Unix"};
        #elif __unix__
            return {"Unix"};
        #else
            return {"Unknown"};
        #endif
    }
}

}







SYSTEM2_RESULT System2Run(const char* command, System2CommandInfo* outCommandInfo)
{
    int result = pipe(outCommandInfo->ParentToChildPipes);
    if(result != 0)
        return SYSTEM2_RESULT_PIPE_CREATE_FAILED;
    
    result = pipe(outCommandInfo->ChildToParentPipes);
    if(result != 0)
        return SYSTEM2_RESULT_PIPE_CREATE_FAILED;
    
    pid_t pid = fork();
    
    if(pid < 0)
        return SYSTEM2_RESULT_FORK_FAILED;
    //Child
    else if(pid == 0)
    {
        if(close(outCommandInfo->ParentToChildPipes[SYSTEM2_FD_WRITE]) != 0)
            exit(1);
        
        if(close(outCommandInfo->ChildToParentPipes[SYSTEM2_FD_READ]) != 0)
            exit(1);
        
        //close(STDIN_FILENO);
        //close(STDOUT_FILENO);
        //close(STDERR_FILENO);
        
        result = dup2(outCommandInfo->ParentToChildPipes[0], STDIN_FILENO);
        if(result == -1)
            exit(1);

        result = dup2(outCommandInfo->ChildToParentPipes[1], STDOUT_FILENO);
        if(result == -1)
            exit(1);
        
        result = dup2(outCommandInfo->ChildToParentPipes[1], STDERR_FILENO);
        if(result == -1)
            exit(1);
        
        execlp("sh", "sh", "-c", command, NULL);
        
        //Should never be reached
        
        exit(1);
    }
    //Parent
    else
    {
        if(close(outCommandInfo->ParentToChildPipes[SYSTEM2_FD_READ]) != 0)
            return SYSTEM2_RESULT_PIPE_FD_CLOSE_FAILED;
        
        if(close(outCommandInfo->ChildToParentPipes[SYSTEM2_FD_WRITE]) != 0)
            return SYSTEM2_RESULT_PIPE_FD_CLOSE_FAILED;
        
        outCommandInfo->ChildProcessID = pid;
    }
    
    return SYSTEM2_RESULT_SUCCESS;
}

#include "ssLogger/ssLog.hpp"

SYSTEM2_RESULT System2ReadFromOutput(   System2CommandInfo* info, 
                                        char* outputBuffer, 
                                        uint32_t outputBufferSize,
                                        uint32_t* outBytesRead)
{
    uint32_t readResult;
    
    while (true)
    {
        readResult = read(  info->ChildToParentPipes[SYSTEM2_FD_READ], 
                            outputBuffer, 
                            outputBufferSize - *outBytesRead);
        
        if(readResult == 0)
            break;
        
        if(readResult == -1)
            return SYSTEM2_RESULT_READ_FAILED;

        outputBuffer += readResult;
        *outBytesRead += readResult;
        
        if(outputBufferSize - *outBytesRead == 0)
            return SYSTEM2_RESULT_READ_NOT_FINISHED;
    }
    
    return SYSTEM2_RESULT_SUCCESS;
}

SYSTEM2_RESULT System2WriteToInput( System2CommandInfo* info, 
                                    const char* inputBuffer, 
                                    const uint32_t inputBufferSize)
{
    uint32_t currentWriteLengthLeft = inputBufferSize;
    
    while(true)
    {
        uint32_t writeResult = write(   info->ParentToChildPipes[SYSTEM2_FD_WRITE], 
                                        inputBuffer, 
                                        inputBufferSize);

        if(writeResult == -1)
            return SYSTEM2_RESULT_WRITE_FAILED;
        
        inputBuffer += writeResult;
        currentWriteLengthLeft -= writeResult;
        
        if(currentWriteLengthLeft == 0)
            break;
    }
    
    return SYSTEM2_RESULT_SUCCESS;
}

SYSTEM2_RESULT System2GetCommandReturnValueAsync(   System2CommandInfo* info, 
                                                    int* outReturnCode)
{
    int status;
    pid_t pidResult = waitpid(info->ChildProcessID, &status, WNOHANG);
    
    if(pidResult == 0)
        return SYSTEM2_RESULT_COMMAND_NOT_FINISHED;
    else if(pidResult == -1)
        return SYSTEM2_RESULT_COMMAND_NOT_FINISHED;
    
    if(close(info->ChildToParentPipes[SYSTEM2_FD_READ]) != 0)
        return SYSTEM2_RESULT_PIPE_FD_CLOSE_FAILED;

    if(close(info->ParentToChildPipes[SYSTEM2_FD_WRITE]) != 0)
        return SYSTEM2_RESULT_PIPE_FD_CLOSE_FAILED;
    
    if(!WIFEXITED(status))
    {
        *outReturnCode = -1;
        return SYSTEM2_RESULT_COMMAND_TERMINATED;
    }

    *outReturnCode = WEXITSTATUS(status);
    return SYSTEM2_RESULT_SUCCESS;
}

SYSTEM2_RESULT System2GetCommandReturnValueSync(System2CommandInfo* info, 
                                                int* outReturnCode)
{
    int status;
    pid_t pidResult = waitpid(info->ChildProcessID, &status, 0);
    
    if(pidResult == -1)
        return SYSTEM2_RESULT_COMMAND_WAIT_FAILED;
    
    if(close(info->ChildToParentPipes[SYSTEM2_FD_READ]) != 0)
        return SYSTEM2_RESULT_PIPE_FD_CLOSE_FAILED;

    if(close(info->ParentToChildPipes[SYSTEM2_FD_WRITE]) != 0)
        return SYSTEM2_RESULT_PIPE_FD_CLOSE_FAILED;
    
    if(!WIFEXITED(status))
    {
        *outReturnCode = -1;
        return SYSTEM2_RESULT_COMMAND_TERMINATED;
    }

    *outReturnCode = WEXITSTATUS(status);
    return SYSTEM2_RESULT_SUCCESS;
}


#if 0
void tempLinux()
{
    int       parentToChild[2];
    int       childToParent[2];
    pid_t     pid;
    string    dataReadFromChild;
    char      buffer[BUFFER_SIZE + 1];
    ssize_t   readResult;
    int       status;
    
    #define ASSERT_IS(shouldBe, value) \
        if (value != shouldBe) { printf("Value: %d\n", value); exit(-1); }

    #define ASSERT_NOT(notToBe, value) \
        if (value == notToBe) { printf("Value: %d\n", value); exit(-1); }

    ASSERT_IS(0, pipe(parentToChild));
    ASSERT_IS(0, pipe(childToParent));

    switch (pid = fork())
    {
        case -1:
            printf("Fork failed\n");
            exit(-1);

        case 0: /* Child */
            ASSERT_NOT(-1, dup2(parentToChild[READ_FD], STDIN_FILENO));
            ASSERT_NOT(-1, dup2(childToParent[WRITE_FD], STDOUT_FILENO));
            ASSERT_NOT(-1, dup2(childToParent[WRITE_FD], STDERR_FILENO));
            
            ASSERT_IS(0, close(parentToChild [WRITE_FD]));
            ASSERT_IS(0, close(childToParent [READ_FD]));

            /*     file, arg0, arg1,  arg2 */
            execlp("ls", "ls", "-al", "--color", (char *) 0);

            printf("This line should never be reached!!!\n");
            exit(-1);

        default: /* Parent */
            printf("Child %d process running...\n",  pid);

            ASSERT_IS(0, close(parentToChild [READ_FD]));
            ASSERT_IS(0, close(childToParent [WRITE_FD]));

            while (true)
            {
                switch (readResult = read(  childToParent[READ_FD],
                                            buffer, BUFFER_SIZE))
                {
                case 0: /* End-of-File, or non-blocking read. */
                    cout << "End of file reached..."         << endl
                        << "Data received was ("
                        << dataReadFromChild.size() << "): " << endl
                        << dataReadFromChild                << endl;

                    ASSERT_IS(pid, waitpid(pid, & status, 0));

                    cout << endl
                        << "Child exit staus is:  " << WEXITSTATUS(status) << endl
                        << endl;

                    exit(0);


                case -1:
                    if ((errno == EINTR) || (errno == EAGAIN))
                    {
                    errno = 0;
                    break;
                    }
                    else
                    {
                    FAIL("read() failed");
                    exit(-1);
                    }

                default:
                    dataReadFromChild . append(buffer, readResult);
                    break;
                }
            } /* while (true) */
    } /* switch (pid = fork())*/

}
#endif

#if 0
void tempWindows()
{
    wchar_t command[] = L"nslookup myip.opendns.com. resolver1.opendns.com";

    wchar_t cmd[MAX_PATH] ;
    wchar_t cmdline[ MAX_PATH + 50 ];
    swprintf_s( cmdline, L"%s /c %s", cmd, command );

    STARTUPINFOW startInf;
    memset( &startInf, 0, sizeof startInf );
    startInf.cb = sizeof(startInf);

    PROCESS_INFORMATION procInf;
    memset( &procInf, 0, sizeof procInf );

    BOOL b = CreateProcessW( NULL, cmdline, NULL, NULL, FALSE,
        NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW, NULL, NULL, &startInf, &procInf );

    DWORD dwErr = 0;
    if( b ) 
    {
        // Wait till process completes
        WaitForSingleObject( procInf.hProcess, INFINITE );
        // Check process’s exit code
        GetExitCodeProcess( procInf.hProcess, &dwErr );
        // Avoid memory leak by closing process handle
        CloseHandle( procInf.hProcess );
    } 
    else 
    {
        dwErr = GetLastError();
    }
    if( dwErr ) 
    {
        wprintf(_T("Command failed. Error %d\n"),dwErr);
    }

}
#endif