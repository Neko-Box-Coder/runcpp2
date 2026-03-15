#include "runcpp2/Data/Profile.hpp"
#include "runcpp2/runcpp2.hpp"
#include "runcpp2/LibYAML_Wrapper.hpp"
#include "runcpp2/DeferUtil.hpp"
#include "ssLogger/ssLog.hpp"

DS::Result<void> TestMain()
{
    //NOTE: This is just a test YAML for validating parsing, don't use it for actual config
    const char* yamlStr = R"(
        Name: "g++"
        NameAliases: ["mingw"]
        FileExtensions: [.cpp, .cc, .cxx]
        Languages: ["c++"]
        Setup:
            DefaultPlatform: ["setup command 1", "setup command 2"]
        Cleanup:
            DefaultPlatform: []
        FilesTypes:
            ObjectLinkFile:
                Prefix:
                    DefaultPlatform: ""
                Extension:
                    Windows: ".obj"
                    Unix: ".o"
            SharedLinkFile:
                Prefix:
                    Windows: ""
                    Linux: "lib"
                    MacOS: ""
                Extension:
                    Windows: ".lib"
                    Linux: ".so"
                    MacOS: ".dylib"
            SharedLibraryFile:
                Prefix:
                    Windows: ""
                    Linux: "lib"
                    MacOS: ""
                Extension:
                    Windows: ".dll"
                    Linux: ".so"
                    MacOS: ".dylib"
            StaticLinkFile:
                Prefix:
                    Unix: "lib"
                    Windows: ""
                Extension:
                    Windows: ".lib"
                    Unix: ".a"
            DebugSymbolFile:
                Prefix:
                    Windows: ""
                    Unix: ""
                Extension:
                    Windows: .pdb
                    Unix: .debug
        Compiler:
            PreRun:
                DefaultPlatform: ""
            CheckExistence:
                DefaultPlatform: "g++ -v"
            CompileTypes:
                Executable:
                    DefaultPlatform:
                        Flags: "-std=c++17 -Wall -g"
                        Executable: "g++"
                        RunParts:
                        -   Type: Once
                            CommandPart: "{Executable} -c {CompileFlags}"
                        -   Type: Repeats
                            CommandPart: " -I\"{IncludeDirectoryPath}\""
                        -   Type: Once
                            CommandPart: " \"{InputFilePath}\" -o \"{OutputFilePath}\""
                        ExpectedOutputFiles: ["TestOutputFile", "TestOutputFile2"]
                ExecutableShared:
                    DefaultPlatform:
                        Flags: "-std=c++17 -Wall -g -fpic"
                        Executable: "g++"
                        RunParts:
                        -   Type: Once
                            CommandPart: "{Executable} -c {CompileFlags}"
                        -   Type: Repeats
                            CommandPart: " -I\"{IncludeDirectoryPath}\""
                        -   Type: Once
                            CommandPart: " \"{InputFilePath}\" -o \"{OutputFilePath}\""
                        ExpectedOutputFiles: ["TestOutputFile", "TestOutputFile2"]
                Static:
                    DefaultPlatform:
                        Flags: "-std=c++17 -Wall -g"
                        Executable: "g++"
                        RunParts:
                        -   Type: Once
                            CommandPart: "{Executable} -c {CompileFlags}"
                        ExpectedOutputFiles: ["TestOutputFile", "TestOutputFile2"]
                Shared:
                    DefaultPlatform:
                        Flags: "-std=c++17 -Wall -g -fpic"
                        Executable: "g++"
                        RunParts:
                        -   Type: Once
                            CommandPart: "{Executable} -c {CompileFlags}"
                        ExpectedOutputFiles: ["TestOutputFile", "TestOutputFile2"]
        Linker:
            PreRun:
                DefaultPlatform: ""
            CheckExistence:
                DefaultPlatform: "g++ -v"
            LinkTypes:
                Executable:
                    Unix:
                        Flags: "-Wl,-rpath,\\$ORIGIN"
                        Executable: "g++"
                        RunParts:
                        -   Type: Once
                            CommandPart: "{Executable} {LinkFlags} -o \"{OutputFilePath}\""
                        ExpectedOutputFiles: ["TestOutputFile", "TestOutputFile2"]
                    Windows:
                        Flags: "-Wl,-rpath,\\$ORIGIN"
                        Executable: "g++"
                        RunParts:
                        -   Type: Once
                            CommandPart: "{Executable} {LinkFlags} -o \"{OutputFilePath}\""
                        ExpectedOutputFiles: ["TestOutputFile", "TestOutputFile2"]
                Static:
                    DefaultPlatform:
                        Flags: ""
                        Executable: "g++"
                        RunParts:
                        -   Type: Once
                            CommandPart: "{Executable} {LinkFlags} -o \"{OutputFilePath}\""
                        ExpectedOutputFiles: ["TestOutputFile", "TestOutputFile2"]
                Shared:
                    Unix:
                        Flags: "-shared -Wl,-rpath,\\$ORIGIN"
                        Executable: "g++"
                        RunParts:
                        -   Type: Once
                            CommandPart: "{Executable} {LinkFlags} -o \"{OutputFilePath}\""
                        ExpectedOutputFiles: ["TestOutputFile", "TestOutputFile2"]
                ExecutableShared:
                    Unix:
                        Flags: "-shared -Wl,-rpath,\\$ORIGIN"
                        Executable: "g++"
                        RunParts:
                        -   Type: Once
                            CommandPart: "{Executable} {LinkFlags} -o \"{OutputFilePath}\""
                        ExpectedOutputFiles: ["TestOutputFile", "TestOutputFile2"]
    )";
    
    runcpp2::YAML::ResourceHandle resource;
    std::vector<runcpp2::YAML::NodePtr> roots = runcpp2::YAML::ParseYAML(yamlStr, resource).DS_TRY();
    DEFER { FreeYAMLResource(resource); };
    
    DS_ASSERT_EQ(roots.size(), 1);
    runcpp2::YAML::NodePtr root = roots.front();
    runcpp2::Data::Profile profile;
    
    DS_ASSERT_TRUE(profile.ParseYAML_Node(root));
    
    //Verify basic fields
    DS_ASSERT_EQ(profile.Name, "g++");
    DS_ASSERT_EQ(profile.NameAliases.size(), 1);
    DS_ASSERT_EQ(profile.NameAliases.count("mingw"), 1);
    
    //Verify FileExtensions
    DS_ASSERT_EQ(profile.FileExtensions.size(), 3);
    DS_ASSERT_EQ(profile.FileExtensions.count(".cpp"), 1);
    DS_ASSERT_EQ(profile.FileExtensions.count(".cc"), 1);
    DS_ASSERT_EQ(profile.FileExtensions.count(".cxx"), 1);
    
    //Verify Languages
    DS_ASSERT_EQ(profile.Languages.size(), 1);
    DS_ASSERT_EQ(profile.Languages.count("c++"), 1);
    
    //Verify Setup
    DS_ASSERT_EQ(profile.Setup.count("DefaultPlatform"), 1);
    const std::vector<std::string>& setupCommands = profile.Setup.at("DefaultPlatform");
    DS_ASSERT_EQ(setupCommands.size(), 2);
    DS_ASSERT_EQ(setupCommands.at(0), "setup command 1");
    DS_ASSERT_EQ(setupCommands.at(1), "setup command 2");
    
    //Verify FilesTypes
    const auto& objectLinkFile = profile.FilesTypes.ObjectLinkFile;
    DS_ASSERT_EQ(objectLinkFile.Extension.at("Windows"), ".obj");
    DS_ASSERT_EQ(objectLinkFile.Extension.at("Unix"), ".o");
    
    const auto& sharedLinkFile = profile.FilesTypes.SharedLinkFile;
    DS_ASSERT_EQ(sharedLinkFile.Prefix.at("Linux"), "lib");
    DS_ASSERT_EQ(sharedLinkFile.Extension.at("Linux"), ".so");
    
    const auto& debugSymbolFile = profile.FilesTypes.DebugSymbolFile;
    DS_ASSERT_EQ(debugSymbolFile.Extension.at("Windows"), ".pdb");
    DS_ASSERT_EQ(debugSymbolFile.Extension.at("Unix"), ".debug");
    
    //Verify Compiler
    DS_ASSERT_EQ(profile.Compiler.CheckExistence.at("DefaultPlatform"), "g++ -v");
    const auto& executableCompile = profile.Compiler.OutputTypes.Executable.at("DefaultPlatform");
    DS_ASSERT_EQ(executableCompile.Flags, "-std=c++17 -Wall -g");
    DS_ASSERT_EQ(executableCompile.Executable, "g++");
    DS_ASSERT_EQ(executableCompile.RunParts.size(), 3);
    DS_ASSERT_EQ(executableCompile.ExpectedOutputFiles.size(), 2);
    
    //Verify Compiler ExecutableShared
    const auto& executableSharedCompile = 
        profile.Compiler.OutputTypes.ExecutableShared.at("DefaultPlatform");
    DS_ASSERT_EQ(executableSharedCompile.Flags, "-std=c++17 -Wall -g -fpic");
    DS_ASSERT_EQ(executableSharedCompile.Executable, "g++");
    DS_ASSERT_EQ(executableSharedCompile.RunParts.size(), 3);
    DS_ASSERT_EQ(executableSharedCompile.ExpectedOutputFiles.size(), 2);
    
    //Verify Linker
    DS_ASSERT_EQ(profile.Linker.CheckExistence.at("DefaultPlatform"), "g++ -v");
    const auto& executableLink = profile.Linker.OutputTypes.Executable.at("Unix");
    DS_ASSERT_EQ(executableLink.Flags, "-Wl,-rpath,\\$ORIGIN");
    DS_ASSERT_EQ(executableLink.Executable, "g++");
    DS_ASSERT_EQ(executableLink.RunParts.size(), 1);
    DS_ASSERT_EQ(executableLink.ExpectedOutputFiles.size(), 2);
    
    //Verify Linker ExecutableShared
    const auto& executableSharedLink = profile.Linker.OutputTypes.ExecutableShared.at("Unix");
    DS_ASSERT_EQ(executableSharedLink.Flags, "-shared -Wl,-rpath,\\$ORIGIN");
    DS_ASSERT_EQ(executableSharedLink.Executable, "g++");
    DS_ASSERT_EQ(executableSharedLink.RunParts.size(), 1);
    DS_ASSERT_EQ(executableSharedLink.ExpectedOutputFiles.size(), 2);
    
    //Test ToString() and Equals()
    std::string yamlOutput = profile.ToString("");
    roots = runcpp2::YAML::ParseYAML(yamlOutput, resource).DS_TRY();
    DS_ASSERT_EQ(roots.size(), 1);
    
    runcpp2::Data::Profile parsedOutput;
    parsedOutput.ParseYAML_Node(roots.front());
    DS_ASSERT_TRUE(profile.Equals(parsedOutput));
    
    return {};
}

int main(int argc, char** argv)
{
    try
    {
        TestMain().DS_TRY_ACT(ssLOG_LINE(DS_TMP_ERROR.ToString()); return 1);
        return 0;
    }
    catch(std::exception& ex)
    {
        ssLOG_LINE(ex.what());
        return 1;
    }
    return 1;
}
