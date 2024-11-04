#include "runcpp2/Data/ScriptInfo.hpp"
#include "ssTest.hpp"
#include "runcpp2/YamlLib.hpp"
#include "runcpp2/runcpp2.hpp"

int main(int argc, char** argv)
{
    runcpp2::SetLogLevel("Warning");
    
    ssTEST_INIT_TEST_GROUP();
    
    ssTEST("ScriptInfo Should Parse Valid YAML")
    {
        ssTEST_OUTPUT_SETUP
        (
            const char* yamlStr = R"(
                Language: C++
                PassScriptPath: true
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
                Dependencies:
                -   Name: MyLib
                    Platforms:
                    -   MSVC
                    -   GCC
                    Source:
                        Type: Git
                        Value: https://github.com/user/mylib.git
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
                Defines:
                    Windows:
                        MSVC:
                        -   _DEBUG
                        -   VERSION=1.0.0
                        GCC:
                        -   NDEBUG
            )";
            
            ryml::Tree tree = ryml::parse_in_arena(c4::to_csubstr(yamlStr));
            ryml::ConstNodeRef root = tree.rootref();
            
            runcpp2::Data::ScriptInfo scriptInfo;
        );
        
        ssTEST_OUTPUT_EXECUTION
        (
            ryml::ConstNodeRef nodeRef = root;
            bool parseResult = scriptInfo.ParseYAML_Node(nodeRef);
        );
        
        ssTEST_OUTPUT_ASSERT("ParseYAML_Node should succeed", parseResult);
        
        //Verify basic fields
        ssTEST_OUTPUT_ASSERT("Language", scriptInfo.Language == "C++");
        ssTEST_OUTPUT_ASSERT("PassScriptPath", scriptInfo.PassScriptPath == true);
        
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
            const std::vector<ghc::filesystem::path>& msvcFiles = 
                scriptInfo.OtherFilesToBeCompiled.at("Windows").CompilesFiles.at("MSVC");
        );
        ssTEST_OUTPUT_ASSERT("MSVC files count", msvcFiles.size() == 2);
        ssTEST_OUTPUT_ASSERT("MSVC first file", msvcFiles.at(0) == "src/extra.cpp");
        
        //Verify Dependencies
        ssTEST_OUTPUT_ASSERT("Dependencies count", scriptInfo.Dependencies.size(), 1);
        ssTEST_OUTPUT_ASSERT("Dependency name", scriptInfo.Dependencies.at(0).Name == "MyLib");
        ssTEST_OUTPUT_ASSERT(   "Dependency type", 
                                scriptInfo.Dependencies.at(0).LibraryType == 
                                runcpp2::Data::DependencyLibraryType::SHARED);
        
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

        //Test ToString() and Equals()
        ssTEST_OUTPUT_EXECUTION
        (
            std::string yamlOutput = scriptInfo.ToString("");
            ryml::Tree outputTree = ryml::parse_in_arena(ryml::to_csubstr(yamlOutput));
            
            runcpp2::Data::ScriptInfo parsedOutput;
            nodeRef = outputTree.rootref();
            parsedOutput.ParseYAML_Node(nodeRef);
        );
        
        ssTEST_OUTPUT_ASSERT(   "Parsed output should equal original", 
                                scriptInfo.Equals(parsedOutput));
    };
    
    ssTEST_END_TEST_GROUP();
    return 0;
}
