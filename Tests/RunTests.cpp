/* runcpp2
Dependencies:
-   Name: System2
    Platforms: [DefaultPlatform]
    Source:
        Git:
            URL: "https://github.com/Neko-Box-Coder/System2.git"
    LibraryType: Header
    IncludePaths: ["."]
*/

#include "System2.h"

#include <string>
#include <stdio.h>


bool RunCommand(std::string command)
{
    System2CommandInfo commandInfo = {};
    
    #if defined(_WIN32)
        for(int i = 0; i < command.size(); ++i)
            command[i] = command[i] == '/' ? '\\' : command[i];
    #endif
    
    SYSTEM2_RESULT res = System2Run(command.c_str(), &commandInfo);
    if(res != SYSTEM2_RESULT_SUCCESS)
    {
        printf("Command %s failed with result %i\n", command.c_str(), (int)res);
        return false;
    }
    int retVal; 
    res = System2GetCommandReturnValue(&commandInfo, -1, &retVal);
    if(res != SYSTEM2_RESULT_SUCCESS)
    {
        printf("Command %s failed to get return value with result %i\n", command.c_str(), (int)res);
        return false;
    }
    
    if(retVal != 0)
    {
        printf("Non zero return code %i for command %s\n", retVal, command.c_str());
        return false;
    }
    
    return true;
}

bool ListDir(const std::string& dirLoc = ".")
{
    #if defined(_WIN32)
        return RunCommand(std::string("dir ") + dirLoc);
    #elif defined(__unix__) || defined(__APPLE__)
        return RunCommand(std::string("ls -lah ") + dirLoc);
    #endif
}

#define CH(x) do { if(!(x)) { printf("Line %d failed.\n", __LINE__); exit(1); } } while(0)

#define SH(x) CH( RunCommand(x) )


#if defined(_WIN32)
    #define RUNCPP2_EXE "runcpp2.exe"
#else
    #define RUNCPP2_EXE "runcpp2"
#endif

#define COMMON_RUNCPP2_ARGS "-l -c ../DefaultYAMLs/DefaultUserConfig.yaml --log-level info"

#define RUN_TEST(testName) SH( "cd ./Build && ./" RUNCPP2_EXE " run " COMMON_RUNCPP2_ARGS " " testName);
#define BUILD_TEST(testName, outputLoc) \
    SH( "cd ./Build && ./" RUNCPP2_EXE " build -o " outputLoc " " COMMON_RUNCPP2_ARGS " " testName);

int main(int, char**)
{
    CH(ListDir());
    RUN_TEST("../Tests/MultiSourcesAndStatic/Test.cpp");
    
    BUILD_TEST("../Tests/MultiSourcesAndStatic/TestStatic.cpp", "TestStaticOutput");
    CH(ListDir("./Build/TestStaticOutput"));
    
    RUN_TEST("../Tests/LocalDependency/TestLocalDependency.cpp");
    RUN_TEST("../Tests/SeparateYaml/TestSeparateYaml.cpp");
    if(RunCommand(  "cd ./Build && ./" RUNCPP2_EXE " run " COMMON_RUNCPP2_ARGS 
                    " ../Tests/MissingSource/TestMissingSource.yaml"))
    {
        printf("We expect this to failed, somehow passed?...\n");
        return 1;
    }
    RUN_TEST("../Tests/YamlOnly/YamlOnlyTest.yaml");
    RUN_TEST("../Tests/C_ProfileTest/System2Test.c");
    RUN_TEST(   "../Examples/InteractiveTutorial.cpp --test ./" RUNCPP2_EXE 
                " ../DefaultYAMLs/DefaultUserConfig.yaml");
    
    
    printf("Done\n");
    return 0;
}
