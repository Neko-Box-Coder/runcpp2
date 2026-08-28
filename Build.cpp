/* runcpp2

PassScriptPath: true

OverrideCompileFlags:
    DefaultPlatform:
        msvc:
            Append: ""
        DefaultProfile:
            Append: "-Wno-return-local-addr -Wno-sign-compare -Wno-unused-parameter -Wno-switch"

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
//#include "./Src/runcpp2/StringUtil.hpp"

#include <string>
#include <stdio.h>
#include <stdint.h>

DS::Result<void> RunCommand(const std::string& command, bool noWait = false)
{
    System2CommandInfo commandInfo = {};
    
    printf("Running command %s\n", command.c_str());
    SYSTEM2_RESULT system2Result = System2Run(command.c_str(), &commandInfo);
    DS_ASSERT_EQ(system2Result, SYSTEM2_RESULT_SUCCESS);
    
    if(!noWait)
    {
        int returnCode = -1;
        system2Result = System2GetCommandReturnValue(&commandInfo, 60, &returnCode);
        DS_ASSERT_EQ(system2Result, SYSTEM2_RESULT_SUCCESS);
        if(returnCode != 0)
        {
            return DS_ERROR_MSG("Failed to run command " + command + " with return code " + 
                                DS_STR(returnCode));
        }
    }
    
    System2CleanupCommand(&commandInfo);
    return {};
}

DS::Result<std::string> RunAndGetOutput(const std::string& command)
{
    System2CommandInfo commandInfo = {};
    commandInfo.RedirectOutput = true;
    
    printf("Running command %s\n", command.c_str());
    SYSTEM2_RESULT system2Result = System2Run(command.c_str(), &commandInfo);
    DS_ASSERT_EQ(system2Result, SYSTEM2_RESULT_SUCCESS);
    
    std::string output = std::string(" ", 1024);
    uint32_t outputSize = 0;
    int returnCode = -1;
    do
    {
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
        
        system2Result = System2GetCommandReturnValue(&commandInfo, 5, &returnCode);
    }
    while(system2Result == SYSTEM2_RESULT_COMMAND_NOT_FINISHED);
    
    System2CleanupCommand(&commandInfo);
    DS_ASSERT_EQ(system2Result, SYSTEM2_RESULT_SUCCESS);
    if(returnCode != 0)
    {
        return DS_ERROR_MSG("Failed to run command " + command + " \nWith output: " + output + 
                            "\nWith return code " + DS_STR(returnCode));
    }
    return output;
}

std::string EscapePath(const std::string& path)
{
    std::string ret = path;
    #if defined(_WIN32)
        for(int i = 0; i < ret.size(); ++i)
        {
            if(ret[i] == '/')
                ret[i] = '\\';
        }
    #endif
    return ret;
}

DS::Result<void> GenerateDefaultYAMLs(const std::string runcpp2Path, const std::string& runcpp2Args)
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
    ghc::filesystem::file_time_type lastWriteTime = ghc::filesystem::last_write_time(   yamlPaths[0], 
                                                                                        ec);
    for(int i = 0; i < sizeof(yamlPaths) / sizeof(yamlPaths[0]); ++i)
    {
        if(!ghc::filesystem::exists(yamlPaths[i], ec))
            return DS_ERROR_MSG(yamlPaths[i].string() + " does not exist.");
    
        if(ghc::filesystem::last_write_time(yamlPaths[i], ec) > lastWriteTime)
            lastWriteTime = ghc::filesystem::last_write_time(yamlPaths[i], ec);
    }
    
    if(ghc::filesystem::exists("./Src/runcpp2/DefaultYAMLs.c", ec))
    {
        if(ghc::filesystem::last_write_time("./Src/runcpp2/DefaultYAMLs.c", ec) > lastWriteTime)
        {
            printf("No changes found, default YAML regeneration skipped\n");
            return {};
        }
    }
    std::string cmd =   runcpp2Path + " run --log-level error " + runcpp2Args + embed2CPath.string() +
                        " ./DefaultYAMLs/DefaultScriptInfo.yaml DefaultScriptInfo " +
                        " ./DefaultYAMLs/DefaultUserConfig.yaml DefaultUserConfig " +
                        " ./DefaultYAMLs/Default/AnnotatedG++.yaml AnnotatedG_PlusPlus " +
                        " ./DefaultYAMLs/Default/CommonFileTypes.yaml CommonFileTypes " +
                        " ./DefaultYAMLs/Default/clang++.yaml ClangPlusPlus " +
                        " ./DefaultYAMLs/Default/g++.yaml G_PlusPlus " +
                        " ./DefaultYAMLs/Default/vs2022_v17+.yaml Vs2022_v17Plus ";
    
    std::string output = RunAndGetOutput(cmd).DS_TRY();
    FILE* defaultYamlFile = fopen("./Src/runcpp2/DefaultYAMLs.c", "w");
    if(!defaultYamlFile)
        return DS_ERROR_MSG("Failed to open ./Src/runcpp2/DefaultYAMLs.c");
    
    DS_ASSERT_EQ(fwrite(output.c_str(), 1, output.size(), defaultYamlFile), output.size());
    DS_ASSERT_EQ(fclose(defaultYamlFile), 0);
    printf("Default YAMLs generation successful\n");
    return {};
}

DS::Result<void> Main(int argc, char** argv)
{
    if(argc <= 2 || strcmp(argv[1], "--help") == 0)
    {
        printf("runcpp2 run Build.cpp [--runcpp2-path <path>]\n");
        return {};
    }
    
    #if defined(_WIN32)
        std::string runcpp2Path = "runcpp2.exe";
    #else
        std::string runcpp2Path = "runcpp2";
    #endif
    
    std::string configVersion = "5";
    bool warnError = true;
    bool info = false;
    bool rebuild = false;
    bool buildTest = true;
    for(int i = 2; i < argc; ++i)
    {
        if(strcmp(argv[i], "--runcpp2-path") == 0)
        {
            if(i + 1 >= argc)
                return DS_ERROR_MSG("Runcpp2 path expected");
            
            if(!ghc::filesystem::exists(argv[i + 1]))
                return DS_ERROR_MSG(std::string(argv[i + 1]) + " does not exist");
            runcpp2Path = argv[i + 1];
            ++i;
        }
        else if(strcmp(argv[i], "--no-werror") == 0)
            warnError = false;
        else if(strcmp(argv[i], "--info") == 0)
            info = true;
        else if(strcmp(argv[i], "--rebuild") == 0)
            rebuild = true;
        else if(strcmp(argv[i], "--no-test") == 0)
            buildTest = false;
        else
            break;
    }
    
    if(!ghc::filesystem::exists("./External/cfgpath/cfgpath.h"))
        return DS_ERROR_MSG("./External/cfgpath/cfgpath.h doesn't exist");
    
    std::error_code ec;
    ghc::filesystem::copy_file( "./External/cfgpath/cfgpath.h", 
                                "./Src/cfgpath.h", 
                                ghc::filesystem::copy_options::update_existing,
                                ec);
    
    std::string runcpp2Args = "-l -c " + EscapePath("./DefaultYAMLs/DefaultUserConfig.yaml ");
    GenerateDefaultYAMLs(runcpp2Path, runcpp2Args).DS_TRY();
    
    RunCommand("git tag -d nightly");
    DS::Result<std::string> gitTagResult = RunAndGetOutput("git describe --tags --abbrev=0");
    std::string gitHash = RunAndGetOutput("git rev-parse --short HEAD").DS_TRY();
    if(gitHash.back() == '\n')
        gitHash.erase(gitHash.end() - 1);
    
    std::string versionString;
    if(gitTagResult.HasValue())
    {
        std::string& gitTag = gitTagResult.Value();
        if(gitTag.back() == '\n')
            gitTag.erase(gitTag.end() - 1);
        
        DS::Result<void> tagExact = RunCommand("git describe --tags --exact-match");
        if(tagExact.HasValue())
            versionString = gitTagResult.Value();
        else
            versionString = gitTagResult.Value() + "-" + gitHash;
    }
    else
    {
        //versionString = "v0.0.0-" + gitHash;
        return DS_ERROR_MSG("Failed to get git tag");
    }
    
    ghc::filesystem::path rootDir = ghc::filesystem::path(argv[1]).parent_path();
    
    std::string params = "RUNCPP2_VERSION:" + versionString + ":RootPath:" + rootDir.string();
    if(warnError)
    {
        //TODO: Replace this with platform/profile map in parameter/variable
        #if defined(_WIN32)
            params += ":ExtraFlags:/WX";
        #elif defined (__unix__) || (defined (__APPLE__) && defined (__MACH__))
            params += ":ExtraFlags:-Werror";
        #else
            #error "Unsupported platform..."
        #endif
    }
    
    const std::string commonBuildCommand =  runcpp2Path + 
                                            " build -o " + EscapePath("./TempBuild ") +
                                            (rebuild ? "--rebuild " : "") +
                                            (info ? "--log-level info " : "") +
                                            runcpp2Args + 
                                            "--parameters \'" + params + "\' ";
    
    RunCommand(commonBuildCommand + EscapePath("./Src/runcpp2/runcpp2.cpp")).DS_TRY();
    
    if(buildTest)
    {
        RunCommand(commonBuildCommand + EscapePath("./Src/Tests/BuildsManagerTest.cpp")).DS_TRY();
        RunCommand(commonBuildCommand + EscapePath("./Src/Tests/IncludeManagerTest.cpp")).DS_TRY();
        RunCommand(commonBuildCommand + EscapePath("./Src/Tests/ConfigParsingTest.cpp")).DS_TRY();
        
        RunCommand(commonBuildCommand + EscapePath("./Src/Tests/Data/BuildTypeTest.cpp")).DS_TRY();
        RunCommand( commonBuildCommand + 
                    EscapePath("./Src/Tests/Data/DependencyInfoTest.cpp")).DS_TRY();
        RunCommand( commonBuildCommand + 
                    EscapePath("./Src/Tests/Data/DependencySourceTest.cpp")).DS_TRY();
        RunCommand(commonBuildCommand + EscapePath("./Src/Tests/Data/ProfileTest.cpp")).DS_TRY();
        RunCommand(commonBuildCommand + EscapePath("./Src/Tests/Data/ScriptInfoTest.cpp")).DS_TRY();
    }
    
    
    ghc::filesystem::path buildDir =    ghc::filesystem::exists(runcpp2Path) ? 
                                        ghc::filesystem::path(runcpp2Path).parent_path() :
                                        "./Build";
    
    RunCommand( "sleep 1s && cp -rf ./TempBuild/* " + buildDir.string() + 
                " && rm -Rf ./TempBuild && printf \"Build Done\\n\"", true);
    
    return {};
}

int main(int argc, char** argv)
{
    Main(argc, argv).DS_TRY_ACT(printf("%s\n", DS_TMP_ERROR.ToString().c_str()); return 1);
    return 0;
}
