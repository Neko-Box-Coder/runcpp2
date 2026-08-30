/* runcpp2
Dependencies:
-   Name: System2
    Platforms: [DefaultPlatform]
    Source:
        Local:
            Path: "../../External/System2"
    LibraryType: Header
    IncludePaths: ["."]
*/

#include "System2.h"
#include <stdio.h>

int main(int argc, char** argv) 
{
    (void)argc;
    (void)argv;
    
    //NOTE: This will fail for C++, this tests to use C Profile only
    System2CommandInfo commandInfo =    {
                                            .RedirectInput = true,
                                            .RedirectOutput = true
                                        };

    //Run the command in shell (subprocess is also available, see main.c)
    {
        #if defined(__unix__) || defined(__APPLE__)
            System2Run("read testVar && echo testVar is \\\"$testVar\\\"", &commandInfo);
        #elif defined(_WIN32)
            System2Run("set /p testVar= && echo testVar is \"!testVar!\"", &commandInfo);
        #else
            #error "Unexpected platform?"
        #endif
    }
    
    //Send input to the command
    {
        char input[] = "test content\n";
        System2WriteToInput(&commandInfo, input, sizeof(input));
    }
    
    //Wait for command to finish and get return code
    int returnCode = -1;
    System2GetCommandReturnValue(&commandInfo, -1, &returnCode);
    
    //Capture output and print it
    {
        char outputBuffer[1024];
        uint32_t bytesRead = 0;
        
        System2ReadFromOutput(&commandInfo, outputBuffer, 1023, &bytesRead);
        outputBuffer[bytesRead] = 0;
        
        printf("%s\n", outputBuffer);
        printf("%s: %d\n", "Command has executed with return value", returnCode);
    }
    
    System2CleanupCommand(&commandInfo);
    return 0;
    
    //Output: testVar is "test content"
    //Output: Command has executed with return value: 0
}
