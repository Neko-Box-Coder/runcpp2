#include "runcpp2/Data/Profile.hpp"
#include "ssTest.hpp"
#include "runcpp2/runcpp2.hpp"
#include "runcpp2/YamlLib.hpp"

int main(int argc, char** argv)
{
    runcpp2::SetLogLevel("Warning");
    
    ssTEST_INIT_TEST_GROUP();
    
    ssTEST("Profile Should Parse Valid YAML")
    {
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
                    Static:
                        DefaultPlatform:
                            Flags: "-std=c++17 -Wall -g"
                            Executable: "g++"
                            RunParts:
                            -   Type: Once
                                CommandPart: "{Executable} -c {CompileFlags}"
                    Shared:
                        DefaultPlatform:
                            Flags: "-std=c++17 -Wall -g -fpic"
                            Executable: "g++"
                            RunParts:
                            -   Type: Once
                                CommandPart: "{Executable} -c {CompileFlags}"
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
                        Windows:
                            Flags: "-Wl,-rpath,\\$ORIGIN"
                            Executable: "g++"
                            RunParts:
                            -   Type: Once
                                CommandPart: "{Executable} {LinkFlags} -o \"{OutputFilePath}\""
                    Static:
                        DefaultPlatform:
                            Flags: ""
                            Executable: "g++"
                            RunParts:
                            -   Type: Once
                                CommandPart: "{Executable} {LinkFlags} -o \"{OutputFilePath}\""
                    Shared:
                        Unix:
                            Flags: "-shared -Wl,-rpath,\\$ORIGIN"
                            Executable: "g++"
                            RunParts:
                            -   Type: Once
                                CommandPart: "{Executable} {LinkFlags} -o \"{OutputFilePath}\""
                    ExecutableShared:
                        Unix:
                            Flags: "-shared -Wl,-rpath,\\$ORIGIN"
                            Executable: "g++"
                            RunParts:
                            -   Type: Once
                                CommandPart: "{Executable} {LinkFlags} -o \"{OutputFilePath}\""
        )";
        
        ssTEST_OUTPUT_SETUP
        (
            ryml::Tree tree = ryml::parse_in_arena(c4::to_csubstr(yamlStr));
            ryml::ConstNodeRef root = tree.rootref();
            
            runcpp2::Data::Profile profile;
        );
        
        ssTEST_OUTPUT_EXECUTION
        (
            ryml::ConstNodeRef nodeRef = root;
            bool parseResult = profile.ParseYAML_Node(nodeRef);
        );
        
        ssTEST_OUTPUT_ASSERT("ParseYAML_Node should succeed", parseResult);
        
        //Verify basic fields
        ssTEST_OUTPUT_ASSERT("Name", profile.Name == "g++");
        ssTEST_OUTPUT_ASSERT("NameAliases size", profile.NameAliases.size() == 1);
        ssTEST_OUTPUT_ASSERT("NameAliases contains mingw", profile.NameAliases.count("mingw") == 1);
        
        //Verify FileExtensions
        ssTEST_OUTPUT_ASSERT(   "FileExtensions size", 
                                profile.FileExtensions.size() == 3);
        ssTEST_OUTPUT_ASSERT(   "FileExtensions contains .cpp", 
                                profile.FileExtensions.count(".cpp") == 1);
        ssTEST_OUTPUT_ASSERT(   "FileExtensions contains .cc", 
                                profile.FileExtensions.count(".cc") == 1);
        ssTEST_OUTPUT_ASSERT(   "FileExtensions contains .cxx", 
                                profile.FileExtensions.count(".cxx") == 1);
        
        //Verify Languages
        ssTEST_OUTPUT_ASSERT("Languages size", profile.Languages.size() == 1);
        ssTEST_OUTPUT_ASSERT("Languages contains c++", profile.Languages.count("c++") == 1);
        
        //Verify Setup
        ssTEST_OUTPUT_ASSERT("Setup has Default platform", profile.Setup.count("DefaultPlatform") == 1);
        ssTEST_OUTPUT_SETUP
        (
            const std::vector<std::string>& setupCommands = profile.Setup.at("DefaultPlatform");
        );
        ssTEST_OUTPUT_ASSERT("Setup DefaultPlatform has 2 commands", setupCommands.size() == 2);
        ssTEST_OUTPUT_ASSERT("Setup command 1", setupCommands.at(0) == "setup command 1");
        ssTEST_OUTPUT_ASSERT("Setup command 2", setupCommands.at(1) == "setup command 2");
        
        //Verify FilesTypes
        ssTEST_OUTPUT_SETUP
        (
            const auto& objectLinkFile = profile.FilesTypes.ObjectLinkFile;
        );
        ssTEST_OUTPUT_ASSERT(   "ObjectLinkFile Windows extension", 
                                objectLinkFile.Extension.at("Windows") == ".obj");
        ssTEST_OUTPUT_ASSERT(   "ObjectLinkFile Unix extension", 
                                objectLinkFile.Extension.at("Unix") == ".o");
        
        ssTEST_OUTPUT_SETUP
        (
            const auto& sharedLinkFile = profile.FilesTypes.SharedLinkFile;
        );
        ssTEST_OUTPUT_ASSERT(   "SharedLinkFile Linux prefix", 
                                sharedLinkFile.Prefix.at("Linux") == "lib");
        ssTEST_OUTPUT_ASSERT(   "SharedLinkFile Linux extension", 
                                sharedLinkFile.Extension.at("Linux") == ".so");
        
        //Verify Compiler
        ssTEST_OUTPUT_ASSERT(   "Compiler CheckExistence DefaultPlatform", 
                                profile.Compiler.CheckExistence.at("DefaultPlatform") == "g++ -v");
        ssTEST_OUTPUT_SETUP
        (
            const auto& executableCompile = profile.Compiler.OutputTypes.Executable.at("DefaultPlatform");
        );
        ssTEST_OUTPUT_ASSERT(   "Compiler Executable flags", 
                                executableCompile.Flags == "-std=c++17 -Wall -g");
        ssTEST_OUTPUT_ASSERT(   "Compiler Executable executable", 
                                executableCompile.Executable == "g++");
        ssTEST_OUTPUT_ASSERT(   "Compiler Executable RunParts size", 
                                executableCompile.RunParts.size() == 3);
        
        //Verify Compiler ExecutableShared
        ssTEST_OUTPUT_SETUP
        (
            const auto& executableSharedCompile = 
                profile.Compiler.OutputTypes.ExecutableShared.at("DefaultPlatform");
        );
        ssTEST_OUTPUT_ASSERT(   "Compiler ExecutableShared flags", 
                                executableSharedCompile.Flags, 
                                "-std=c++17 -Wall -g -fpic");
        ssTEST_OUTPUT_ASSERT(   "Compiler ExecutableShared executable", 
                                executableSharedCompile.Executable, 
                                "g++");
        ssTEST_OUTPUT_ASSERT(   "Compiler ExecutableShared RunParts size", 
                                executableSharedCompile.RunParts.size(), 
                                3);
        
        //Verify Linker
        ssTEST_OUTPUT_ASSERT(   "Linker CheckExistence DefaultPlatform", 
                                profile.Linker.CheckExistence.at("DefaultPlatform") == "g++ -v");
        ssTEST_OUTPUT_SETUP
        (
            const auto& executableLink = profile.Linker.OutputTypes.Executable.at("Unix");
        );
        ssTEST_OUTPUT_ASSERT(   "Linker Executable flags", 
                                executableLink.Flags, "-Wl,-rpath,\\$ORIGIN");
        ssTEST_OUTPUT_ASSERT(   "Linker Executable executable", 
                                executableLink.Executable == "g++");
        ssTEST_OUTPUT_ASSERT(   "Linker Executable RunParts size", 
                                executableLink.RunParts.size() == 1);
        
        //Verify Linker ExecutableShared
        ssTEST_OUTPUT_SETUP
        (
            const auto& executableSharedLink = 
                profile.Linker.OutputTypes.ExecutableShared.at("Unix");
        );
        ssTEST_OUTPUT_ASSERT(   "Linker ExecutableShared flags", 
                                executableSharedLink.Flags, 
                                "-shared -Wl,-rpath,\\$ORIGIN");
        ssTEST_OUTPUT_ASSERT(   "Linker ExecutableShared executable", 
                                executableSharedLink.Executable, 
                                "g++");
        ssTEST_OUTPUT_ASSERT(   "Linker ExecutableShared RunParts size", 
                                executableSharedLink.RunParts.size(), 
                                1);
        
        //Test ToString() and Equals()
        ssTEST_OUTPUT_EXECUTION
        (
            std::string yamlOutput = profile.ToString("");
            ryml::Tree outputTree = ryml::parse_in_arena(ryml::to_csubstr(yamlOutput));
            
            runcpp2::Data::Profile parsedOutput;
            nodeRef = outputTree.rootref();
            parsedOutput.ParseYAML_Node(nodeRef);
        );
        
        ssTEST_OUTPUT_ASSERT("Parsed output should equal original", profile.Equals(parsedOutput));
    };
    
    ssTEST_END_TEST_GROUP();
    return 0;
} 
