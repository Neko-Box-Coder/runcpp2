/* runcpp2

Dependencies:
-   Name: ghc_filesystem
    Platforms: [DefaultPlatform]
    Source:
        Local:
            Path: "./External/filesystem"
    LibraryType: Header
    IncludePaths: ["include"]
-   Name: System2
    Platforms: [DefaultPlatform]
    Source:
        Local:
            Path: "./External/System2"
    LibraryType: Header
    IncludePaths: ["."]
-   Name: DSResult
    Platforms: [DefaultPlatform]
    Source:
        Local:
            Path: "./External/DSResult"
    LibraryType: Header
    IncludePaths: ["Include", "./External/expected/include"]
*/

#include "ghc/filesystem.hpp"
#include "System2.h"
#include "DSResult/DSResult.hpp"

#include <string>
#include <stdio.h>
#include <stdint.h>

DS::Result<void> Main(int argc, char** argv)
{
    #if defined(_WIN32)
        ghc::filesystem::path embed2CPath = ".\\External\\Embed2C\\embed.c";
    #else
        ghc::filesystem::path embed2CPath = "./External/Embed2C/embed.c";
    #endif
    ghc::filesystem::path yamlPaths[] = 
    {
        "./DefaultYAMLs/DefaultScriptInfo.yaml",
        "./DefaultYAMLs/DefaultUserConfig.yaml",
        "./DefaultYAMLs/Default/AnnotatedG++.yaml",
        "./DefaultYAMLs/Default/CommonFileTypes.yaml",
        "./DefaultYAMLs/Default/clang++.yaml",
        "./DefaultYAMLs/Default/g++.yaml",
        "./DefaultYAMLs/Default/vs2022_v17+.yaml",
    };
    
    std::error_code ec;
    for(int i = 0; i < sizeof(yamlPaths) / sizeof(yamlPaths[0]); ++i)
    {
        if(!ghc::filesystem::exists(yamlPaths[i], ec))
            return DS_ERROR_MSG(yamlPaths[i].string() + " does not exist.");
    }
    
    #if defined(_WIN32)
        std::string runcpp2Path = "runcpp2.exe";
    #else
        std::string runcpp2Path = "runcpp2";
    #endif
    
    if(argc != 1)
    {
        if(!ghc::filesystem::exists(argv[1]))
            return DS_ERROR_MSG(std::string(argv[1]) + " does not exist");
        runcpp2Path = argv[1];
    }
    
    std::string cmd =   runcpp2Path + " run --no-warning " + embed2CPath.string() +
                        " ./DefaultYAMLs/DefaultScriptInfo.yaml DefaultScriptInfo " +
                        " ./DefaultYAMLs/DefaultUserConfig.yaml DefaultUserConfig " +
                        " ./DefaultYAMLs/Default/AnnotatedG++.yaml AnnotatedG_PlusPlus " +
                        " ./DefaultYAMLs/Default/CommonFileTypes.yaml CommonFileTypes " +
                        " ./DefaultYAMLs/Default/clang++.yaml ClangPlusPlus " +
                        " ./DefaultYAMLs/Default/g++.yaml G_PlusPlus " +
                        " ./DefaultYAMLs/Default/vs2022_v17+.yaml Vs2022_v17Plus ";
    
    
    System2CommandInfo commandInfo = {};
    commandInfo.RedirectOutput = true;
    
    printf("Running %s\n", cmd.c_str());
    SYSTEM2_RESULT system2Result = System2Run(cmd.c_str(), &commandInfo);
    DS_ASSERT_EQ(system2Result, SYSTEM2_RESULT_SUCCESS);
    
    std::string output = std::string(" ", 1024);
    uint32_t outputSize = 0;
    do
    {
        uint32_t readBytes = 0;
        system2Result = System2ReadFromOutput(  &commandInfo, 
                                                output.data() + outputSize, 
                                                output.size() - outputSize, 
                                                &readBytes);
        
        #if defined(_WIN32)
            for(int i = outputSize; i < outputSize + readBytes; ++i)
            {
                if(output[i] == '\r')
                    output[i] = ' ';
            }
        #endif
        
        outputSize += readBytes;
        if(output.size() / 2 < outputSize)
            output.resize(output.size() * 2);
        DS_ASSERT_TRUE( system2Result == SYSTEM2_RESULT_SUCCESS ||
                        system2Result == SYSTEM2_RESULT_READ_NOT_FINISHED);
    }
    while(system2Result == SYSTEM2_RESULT_READ_NOT_FINISHED);
    output.resize(outputSize);
    
    int returnCode = -1;
    system2Result = System2GetCommandReturnValue(&commandInfo, 60, &returnCode);
    DS_ASSERT_EQ(system2Result, SYSTEM2_RESULT_SUCCESS);
    if(returnCode != 0)
    {
        return DS_ERROR_MSG("Failed to run command " + cmd + " \nWith output: " + output + 
                            "\nWith return code " + DS_STR(returnCode));
    }
    System2CleanupCommand(&commandInfo);
    
    FILE* defaultYamlFile = fopen("./Src/runcpp2/DefaultYAMLs.c", "w");
    if(!defaultYamlFile)
        return DS_ERROR_MSG("Failed to open ./Src/runcpp2/DefaultYAMLs.c");
    
    DS_ASSERT_EQ(fwrite(output.c_str(), 1, output.size(), defaultYamlFile), output.size());
    printf("Default YAMLs generation successful\n");
    return {};
}

int main(int argc, char** argv)
{
    Main(argc, argv).DS_TRY_ACT(printf("%s\n", DS_TMP_ERROR.ToString().c_str()); return 1);
    return 0;
}
