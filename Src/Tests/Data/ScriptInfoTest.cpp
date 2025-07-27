#include "runcpp2/Data/ScriptInfo.hpp"
#include "ssTest.hpp"
#include "runcpp2/YamlLib.hpp"
#include "runcpp2/runcpp2.hpp"

int main(int argc, char** argv)
{
    runcpp2::SetLogLevel("Warning");
    
    ssTEST_INIT_TEST_GROUP();
    
    ssTEST_PARSE_ARGS(argc, argv);
    
    ssTEST("ScriptInfo Should Parse Valid YAML")
    {
        ssTEST_OUTPUT_SETUP
        (
            //NOTE: This is just a test YAML for validating parsing, don't use it for actual config
            const char* yamlStr = R"(
                Language: C++
                PassScriptPath: true
                BuildType: Static
                RequiredProfiles:
                    Windows: [MSVC]
                    Unix: [GCC]
                OverrideCompileFlags:
                    Windows:
                        MSVC:
                            Remove: /W4
                            Append: /W3
                    Unix:
                        GCC:
                            Append: -g
                OverrideLinkFlags:
                    Windows:
                        MSVC:
                            Remove: /SUBSYSTEM:CONSOLE
                            Append: /SUBSYSTEM:WINDOWS
                OtherFilesToBeCompiled:
                    Windows:
                        MSVC:
                        -   src/extra.cpp
                        -   src/debug.cpp
                IncludePaths:
                    Windows:
                        MSVC:
                        -   include
                Defines:
                    Windows:
                        MSVC:
                        -   _DEBUG
                        -   VERSION=1.0.0
                        GCC:
                        -   NDEBUG
                Setup:
                    Windows:
                        MSVC:
                        -   echo 1
                        -   echo 2
                    Unix:
                        GCC:
                        -   echo 3
                        -   echo 4
                PreBuild:
                    Windows:
                        MSVC:
                        -   echo 2
                        -   echo 3
                    Unix:
                        GCC:
                        -   echo 4
                        -   echo 5
                PostBuild:
                    Windows:
                        MSVC:
                        -   echo 3
                        -   echo 4
                    Unix:
                        GCC:
                        -   echo 5
                        -   echo 6
                Cleanup:
                    Windows:
                        MSVC:
                        -   echo 4
                        -   echo 5
                    Unix:
                        GCC:
                        -   echo 6
                        -   echo 7
                Dependencies:
                -   Name: MyLib
                    Platforms:
                    -   MSVC
                    -   GCC
                    Source:
                        Git:
                            URL: https://github.com/user/mylib.git
                    LibraryType: Shared
                    IncludePaths:
                    -   include
                    LinkProperties:
                        Windows:
                            MSVC:
                                SearchLibraryNames:
                                -   mylib
                                SearchDirectories:
                                -   lib/Debug
            )";
            
            ryml::Tree tree = ryml::parse_in_arena(c4::to_csubstr(yamlStr));
            ryml::ConstNodeRef root = tree.rootref();
            
            runcpp2::Data::ScriptInfo scriptInfo;
        );
        
        ssTEST_OUTPUT_EXECUTION
        (
            bool parseResult = scriptInfo.ParseYAML_Node(root);
        );
        
        ssTEST_OUTPUT_ASSERT("ParseYAML_Node should succeed", parseResult);
        
        //Verify basic fields
        ssTEST_OUTPUT_ASSERT("Language", scriptInfo.Language == "C++");
        ssTEST_OUTPUT_ASSERT("PassScriptPath", scriptInfo.PassScriptPath == true);
        ssTEST_OUTPUT_ASSERT(   "BuildType", 
                                scriptInfo.CurrentBuildType == runcpp2::Data::BuildType::STATIC);
        
        //Verify RequiredProfiles
        ssTEST_OUTPUT_ASSERT(   "Windows profiles count", 
                                scriptInfo.RequiredProfiles.at("Windows").size() == 1);
        ssTEST_OUTPUT_ASSERT(   "Windows first profile", 
                                scriptInfo.RequiredProfiles.at("Windows").at(0) == "MSVC");
        
        ssTEST_OUTPUT_ASSERT(   "Unix profiles count", 
                                scriptInfo.RequiredProfiles.at("Unix").size() == 1);
        ssTEST_OUTPUT_ASSERT(   "Unix first profile", 
                                scriptInfo.RequiredProfiles.at("Unix").at(0) == "GCC");
        
        //Verify OverrideCompileFlags
        ssTEST_OUTPUT_SETUP
        (
            const runcpp2::Data::FlagsOverrideInfo& msvcCompileFlags = 
                scriptInfo.OverrideCompileFlags.at("Windows").FlagsOverrides.at("MSVC");
        );
        ssTEST_OUTPUT_ASSERT("MSVC compile remove flag", msvcCompileFlags.Remove == "/W4");
        ssTEST_OUTPUT_ASSERT("MSVC compile append flag", msvcCompileFlags.Append == "/W3");
        
        //Verify OverrideLinkFlags
        ssTEST_OUTPUT_SETUP
        (
            const runcpp2::Data::FlagsOverrideInfo& msvcLinkFlags = 
                scriptInfo.OverrideLinkFlags.at("Windows").FlagsOverrides.at("MSVC");
        );
        ssTEST_OUTPUT_ASSERT("MSVC link remove flag", 
                            msvcLinkFlags.Remove == "/SUBSYSTEM:CONSOLE");
        ssTEST_OUTPUT_ASSERT("MSVC link append flag", 
                            msvcLinkFlags.Append == "/SUBSYSTEM:WINDOWS");
        
        //Verify OtherFilesToBeCompiled
        ssTEST_OUTPUT_SETUP
        (
            const std::vector<ghc::filesystem::path>& msvcCompileFiles = 
                scriptInfo.OtherFilesToBeCompiled.at("Windows").Paths.at("MSVC");
        );
        ssTEST_OUTPUT_ASSERT("MSVC files count", msvcCompileFiles.size() == 2);
        ssTEST_OUTPUT_ASSERT("MSVC first file", msvcCompileFiles.at(0) == "src/extra.cpp");
        
        //Verify IncludePaths
        ssTEST_OUTPUT_SETUP
        (
            const std::vector<ghc::filesystem::path>& msvcIncludeFiles = 
                scriptInfo.IncludePaths.at("Windows").Paths.at("MSVC");
        );
        ssTEST_OUTPUT_ASSERT("IncludePaths count", msvcIncludeFiles.size() == 1);
        ssTEST_OUTPUT_ASSERT("IncludePaths first path", msvcIncludeFiles.at(0) == "include");

        //Verify Defines
        ssTEST_OUTPUT_SETUP
        (
            const std::vector<runcpp2::Data::Define>& msvcDefines = 
                scriptInfo.Defines.at("Windows").Defines.at("MSVC");
        );
        ssTEST_OUTPUT_ASSERT("MSVC defines count", msvcDefines.size() == 2);
        ssTEST_OUTPUT_ASSERT("MSVC first define", msvcDefines.at(0).Name == "_DEBUG");
        ssTEST_OUTPUT_ASSERT("MSVC second define name", msvcDefines.at(1).Name == "VERSION");
        ssTEST_OUTPUT_ASSERT("MSVC second define value", msvcDefines.at(1).Value == "1.0.0");
        
        //Verify Setup
        ssTEST_OUTPUT_ASSERT(   "Setup commands count MSVC", 
                                scriptInfo.Setup.at("Windows").CommandSteps.at("MSVC").size() == 2);
        ssTEST_OUTPUT_ASSERT(   "Setup commands count GCC", 
                                scriptInfo.Setup.at("Unix").CommandSteps.at("GCC").size() == 2);
        
        ssTEST_OUTPUT_SETUP
        (
            const std::vector<std::string>& msvcSetupCommands = 
                scriptInfo.Setup.at("Windows").CommandSteps.at("MSVC");
            
            const std::vector<std::string>& gccSetupCommands = 
                scriptInfo.Setup.at("Unix").CommandSteps.at("GCC");
        );
        
        ssTEST_OUTPUT_ASSERT("MSVC setup commands count", msvcSetupCommands.size() == 2);
        ssTEST_OUTPUT_ASSERT("MSVC setup commands", msvcSetupCommands.at(0) == "echo 1");
        
        ssTEST_OUTPUT_ASSERT("GCC setup commands count", gccSetupCommands.size() == 2);
        ssTEST_OUTPUT_ASSERT("GCC setup commands", gccSetupCommands.at(0) == "echo 3");
        
        //Verify PreBuild
        ssTEST_OUTPUT_ASSERT(   "PreBuild commands count MSVC", 
                                scriptInfo.PreBuild.at("Windows").CommandSteps.at("MSVC").size() == 2);
        ssTEST_OUTPUT_ASSERT(   "PreBuild commands count GCC", 
                                scriptInfo.PreBuild.at("Unix").CommandSteps.at("GCC").size() == 2);
        
        ssTEST_OUTPUT_SETUP
        (
            const std::vector<std::string>& msvcPreBuildCommands = 
                scriptInfo.PreBuild.at("Windows").CommandSteps.at("MSVC");

            const std::vector<std::string>& gccPreBuildCommands = 
                scriptInfo.PreBuild.at("Unix").CommandSteps.at("GCC");
        );
        
        ssTEST_OUTPUT_ASSERT("MSVC prebuild commands count", msvcPreBuildCommands.size() == 2);
        ssTEST_OUTPUT_ASSERT("MSVC prebuild commands", msvcPreBuildCommands.at(0) == "echo 2");
        
        ssTEST_OUTPUT_ASSERT("GCC prebuild commands count", gccPreBuildCommands.size() == 2);
        ssTEST_OUTPUT_ASSERT("GCC prebuild commands", gccPreBuildCommands.at(1) == "echo 5");

        //Verify PostBuild
        ssTEST_OUTPUT_ASSERT(   "PostBuild commands count GCC", 
                                scriptInfo.PostBuild.at("Unix").CommandSteps.at("GCC").size() == 2);

        ssTEST_OUTPUT_SETUP
        (
            const std::vector<std::string>& gccPostBuildCommands = 
                scriptInfo.PostBuild.at("Unix").CommandSteps.at("GCC");
        );
        
        ssTEST_OUTPUT_ASSERT("GCC postbuild commands count", gccPostBuildCommands.size() == 2);
        ssTEST_OUTPUT_ASSERT("GCC postbuild commands", gccPostBuildCommands.at(1) == "echo 6");

        //Verify Cleanup
        ssTEST_OUTPUT_ASSERT(   "Cleanup commands count MSVC", 
                                scriptInfo.Cleanup.at("Windows").CommandSteps.at("MSVC").size() == 2);
        
        ssTEST_OUTPUT_SETUP
        (
            const std::vector<std::string>& msvcCleanupCommands = 
                scriptInfo.Cleanup.at("Windows").CommandSteps.at("MSVC");
        );

        ssTEST_OUTPUT_ASSERT("MSVC cleanup commands count", msvcCleanupCommands.size() == 2);
        ssTEST_OUTPUT_ASSERT("MSVC cleanup commands", msvcCleanupCommands.at(0) == "echo 4");

        //Verify Dependencies
        ssTEST_OUTPUT_ASSERT("Dependencies count", scriptInfo.Dependencies.size(), 1);
        ssTEST_OUTPUT_ASSERT("Dependency name", scriptInfo.Dependencies.at(0).Name == "MyLib");
        ssTEST_OUTPUT_ASSERT(   "Dependency type", 
                                scriptInfo.Dependencies.at(0).LibraryType == 
                                runcpp2::Data::DependencyLibraryType::SHARED);

        //Test ToString() and Equals()
        ssTEST_OUTPUT_EXECUTION
        (
            std::string yamlOutput = scriptInfo.ToString("");
            ryml::Tree outputTree = ryml::parse_in_arena(ryml::to_csubstr(yamlOutput));
            
            runcpp2::Data::ScriptInfo parsedOutput;
            parsedOutput.ParseYAML_Node(outputTree.rootref());
        );
        
        ssTEST_OUTPUT_ASSERT(   "Parsed output should equal original", 
                                scriptInfo.Equals(parsedOutput));
    };
    
    ssTEST("ScriptInfo Should Parse SourceFiles as OtherFilesToBeCompiled")
    {
        ssTEST_OUTPUT_SETUP
        (
            //NOTE: This is just a test YAML for validating parsing, don't use it for actual config
            const char* yamlStr = R"(
                SourceFiles:
                    Windows:
                        MSVC:
                        -   src/extra.cpp
                        -   src/debug.cpp
            )";
            
            ryml::Tree tree = ryml::parse_in_arena(c4::to_csubstr(yamlStr));
            ryml::ConstNodeRef root = tree.rootref();
            runcpp2::Data::ScriptInfo scriptInfo;
        );
        
        ssTEST_OUTPUT_EXECUTION
        (
            bool parseResult = scriptInfo.ParseYAML_Node(root);
        );
        
        ssTEST_OUTPUT_ASSERT("ParseYAML_Node should succeed", parseResult);
        
        //Verify SourceFiles
        ssTEST_OUTPUT_SETUP
        (
            const std::vector<ghc::filesystem::path>& msvcCompileFiles = 
                scriptInfo.OtherFilesToBeCompiled.at("Windows").Paths.at("MSVC");
        );
        ssTEST_OUTPUT_ASSERT("MSVC files count", msvcCompileFiles.size() == 2);
        ssTEST_OUTPUT_ASSERT("MSVC first file", msvcCompileFiles.at(0) == "src/extra.cpp");
    };
    
    ssTEST("ScriptInfo Should Parse Fields Without Platform And Profile As Default")
    {
        ssTEST_OUTPUT_SETUP
        (
            //NOTE: This is just a test YAML for validating parsing, don't use it for actual config
            const char* yamlStr = R"(
                Language: C++
                OverrideCompileFlags:
                    Remove: /W4
                    Append: /W3
                OverrideLinkFlags:
                    Remove: /SUBSYSTEM:CONSOLE
                    Append: /SUBSYSTEM:WINDOWS
                OtherFilesToBeCompiled:
                -   src/extra.cpp
                -   src/debug.cpp
                IncludePaths:
                -   include
                -   src/include
                Defines:
                -   _DEBUG
                -   VERSION=1.0.0
                Setup:
                -   echo 1
                -   echo 2
                PreBuild:
                -   echo 3
                -   echo 4
                PostBuild:
                -   echo 5
                -   echo 6
                Cleanup:
                -   echo 7
                -   echo 8
            )";
            
            ryml::Tree tree = ryml::parse_in_arena(c4::to_csubstr(yamlStr));
            ryml::ConstNodeRef root = tree.rootref();
            
            runcpp2::Data::ScriptInfo scriptInfo;
        );
        
        ssTEST_OUTPUT_EXECUTION
        (
            bool parseResult = scriptInfo.ParseYAML_Node(root);
        );
        
        ssTEST_OUTPUT_ASSERT("ParseYAML_Node should succeed", parseResult);
        
        //Verify basic fields
        ssTEST_OUTPUT_ASSERT("Language", scriptInfo.Language == "C++");
        
        //Verify OverrideCompileFlags with default platform and profile
        ssTEST_OUTPUT_SETUP
        (
            const runcpp2::Data::FlagsOverrideInfo& defaultCompileFlags = 
                scriptInfo  .OverrideCompileFlags
                            .at("DefaultPlatform")
                            .FlagsOverrides
                            .at("DefaultProfile");
        );
        ssTEST_OUTPUT_ASSERT("Default compile remove flag", defaultCompileFlags.Remove == "/W4");
        ssTEST_OUTPUT_ASSERT("Default compile append flag", defaultCompileFlags.Append == "/W3");
        
        //Verify OverrideLinkFlags with default platform and profile
        ssTEST_OUTPUT_SETUP
        (
            const runcpp2::Data::FlagsOverrideInfo& defaultLinkFlags = 
                scriptInfo  .OverrideLinkFlags
                            .at("DefaultPlatform")
                            .FlagsOverrides
                            .at("DefaultProfile");
        );
        ssTEST_OUTPUT_ASSERT(   "Default link remove flag", 
                                defaultLinkFlags.Remove == "/SUBSYSTEM:CONSOLE");
        ssTEST_OUTPUT_ASSERT(   "Default link append flag", 
                                defaultLinkFlags.Append == "/SUBSYSTEM:WINDOWS");
        
        //Verify OtherFilesToBeCompiled with default platform and profile
        ssTEST_OUTPUT_SETUP
        (
            const std::vector<ghc::filesystem::path>& defaultCompileFiles = 
                scriptInfo  .OtherFilesToBeCompiled.at("DefaultPlatform").Paths.at("DefaultProfile");
        );
        ssTEST_OUTPUT_ASSERT("Default files count", defaultCompileFiles.size() == 2);
        ssTEST_OUTPUT_ASSERT("Default first file", defaultCompileFiles.at(0) == "src/extra.cpp");
        ssTEST_OUTPUT_ASSERT("Default second file", defaultCompileFiles.at(1) == "src/debug.cpp");
        
        //Verify IncludePaths with default platform and profile
        ssTEST_OUTPUT_SETUP
        (
            const std::vector<ghc::filesystem::path>& defaultIncludePaths = 
                scriptInfo.IncludePaths.at("DefaultPlatform").Paths.at("DefaultProfile");
        );
        ssTEST_OUTPUT_ASSERT("Default include paths count", defaultIncludePaths.size() == 2);
        ssTEST_OUTPUT_ASSERT("Default first include path", defaultIncludePaths.at(0) == "include");
        ssTEST_OUTPUT_ASSERT(   "Default second include path", 
                                defaultIncludePaths.at(1) == "src/include");
        
        //Verify Defines with default platform and profile
        ssTEST_OUTPUT_SETUP
        (
            const std::vector<runcpp2::Data::Define>& defaultDefines = 
                scriptInfo.Defines.at("DefaultPlatform").Defines.at("DefaultProfile");
        );
        ssTEST_OUTPUT_ASSERT("Default defines count", defaultDefines.size() == 2);
        ssTEST_OUTPUT_ASSERT("Default first define", defaultDefines.at(0).Name == "_DEBUG");
        ssTEST_OUTPUT_ASSERT("Default second define name", defaultDefines.at(1).Name == "VERSION");
        ssTEST_OUTPUT_ASSERT("Default second define value", defaultDefines.at(1).Value == "1.0.0");
        
        //Verify Setup with default platform and profile
        ssTEST_OUTPUT_SETUP
        (
            const std::vector<std::string>& defaultSetupCommands = 
                scriptInfo.Setup.at("DefaultPlatform").CommandSteps.at("DefaultProfile");
        );
        ssTEST_OUTPUT_ASSERT("Default setup commands count", defaultSetupCommands.size() == 2);
        ssTEST_OUTPUT_ASSERT(   "Default first setup command", 
                                defaultSetupCommands.at(0) == "echo 1");
        ssTEST_OUTPUT_ASSERT(   "Default second setup command", 
                                defaultSetupCommands.at(1) == "echo 2");
        
        //Verify PreBuild with default platform and profile
        ssTEST_OUTPUT_SETUP
        (
            const std::vector<std::string>& defaultPreBuildCommands = 
                scriptInfo.PreBuild.at("DefaultPlatform").CommandSteps.at("DefaultProfile");
        );
        ssTEST_OUTPUT_ASSERT("Default prebuild commands count", defaultPreBuildCommands.size() == 2);
        ssTEST_OUTPUT_ASSERT(   "Default first prebuild command", 
                                defaultPreBuildCommands.at(0) == "echo 3");
        ssTEST_OUTPUT_ASSERT(   "Default second prebuild command", 
                                defaultPreBuildCommands.at(1) == "echo 4");
        
        //Verify PostBuild with default platform and profile
        ssTEST_OUTPUT_SETUP
        (
            const std::vector<std::string>& defaultPostBuildCommands = 
                scriptInfo.PostBuild.at("DefaultPlatform").CommandSteps.at("DefaultProfile");
        );
        ssTEST_OUTPUT_ASSERT(   "Default postbuild commands count", 
                                defaultPostBuildCommands.size() == 2);
        ssTEST_OUTPUT_ASSERT(   "Default first postbuild command", 
                                defaultPostBuildCommands.at(0) == "echo 5");
        ssTEST_OUTPUT_ASSERT(   "Default second postbuild command", 
                                defaultPostBuildCommands.at(1) == "echo 6");
        
        //Verify Cleanup with default platform and profile
        ssTEST_OUTPUT_SETUP
        (
            const std::vector<std::string>& defaultCleanupCommands = 
                scriptInfo.Cleanup.at("DefaultPlatform").CommandSteps.at("DefaultProfile");
        );
        ssTEST_OUTPUT_ASSERT(   "Default cleanup commands count", 
                                defaultCleanupCommands.size() == 2);
        ssTEST_OUTPUT_ASSERT(   "Default first cleanup command", 
                                defaultCleanupCommands.at(0) == "echo 7");
        ssTEST_OUTPUT_ASSERT(   "Default second cleanup command", 
                                defaultCleanupCommands.at(1) == "echo 8");
    };
    
    ssTEST_END_TEST_GROUP();
    return 0;
}
