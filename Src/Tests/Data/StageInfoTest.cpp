#include "runcpp2/Data/StageInfo.hpp"
#include "ssTest.hpp"
#include "runcpp2/YamlLib.hpp"
#include "runcpp2/runcpp2.hpp"

int main(int argc, char** argv)
{
    runcpp2::SetLogLevel("Warning");
    
    ssTEST_INIT_TEST_GROUP();
    
    ssTEST("StageInfo Should Parse Valid YAML")
    {
        const char* yamlStr = R"(
            PreRun:
                GCC: source env.sh
            CheckExistence:
                GCC: which gcc
            LinkTypes:
                Executable:
                    Unix:
                        Flags: "-Wl,-rpath,\\$ORIGIN"
                        Executable: "g++"
                        Setup: ["mkdir build"]
                        Cleanup: ["rm -rf build"]
                        RunParts:
                        -   Type: Once
                            CommandPart: "{Executable} {LinkFlags} -o \"{OutputFilePath}\""
                        -   Type: Repeats
                            CommandPart: " \"{LinkFilePath}\""
                ExecutableShared:
                    Unix:
                        Flags: "-shared -Wl,-rpath,\\$ORIGIN"
                        Executable: "g++"
                        RunParts:
                        -   Type: Once
                            CommandPart: "{Executable} {LinkFlags} -o \"{OutputFilePath}\""
                        -   Type: Repeats
                            CommandPart: " \"{LinkFilePath}\""
                Static:
                    Unix:
                        Flags: "-Wl,-rpath,\\$ORIGIN"
                        Executable: "g++"
                        RunParts:
                        -   Type: Once
                            CommandPart: "{Executable} {LinkFlags} -o \"{OutputFilePath}\""
                        -   Type: Repeats
                            CommandPart: " \"{LinkFilePath}\""
                Shared:
                    Unix:
                        Flags: "-Wl,-rpath,\\$ORIGIN"
                        Executable: "g++"
                        RunParts: 
                        -   Type: Once
                            CommandPart: "{Executable} {LinkFlags} -o \"{OutputFilePath}\""
                        -   Type: Repeats
                            CommandPart: " \"{LinkFilePath}\""
        )";
        
        ssTEST_OUTPUT_SETUP
        (
            ryml::Tree tree = ryml::parse_in_arena(c4::to_csubstr(yamlStr));
            ryml::ConstNodeRef root = tree.rootref();
            
            runcpp2::Data::StageInfo stageInfo;
        );
        
        ssTEST_OUTPUT_EXECUTION
        (
            ryml::ConstNodeRef nodeRef = root;
            bool parseResult = stageInfo.ParseYAML_Node(nodeRef, "LinkTypes");
        );
        
        ssTEST_OUTPUT_ASSERT("ParseYAML_Node should succeed", parseResult);
        
        //Verify PreRun and CheckExistence
        ssTEST_OUTPUT_ASSERT("GCC CheckExistence", stageInfo.CheckExistence.at("GCC") == "which gcc");
        
        //Verify Unix OutputTypeInfo
        ssTEST_OUTPUT_SETUP
        (
            const auto& unixInfo = stageInfo.OutputTypes.Executable.at("Unix");
        );
        ssTEST_OUTPUT_ASSERT("Unix Executable flags", unixInfo.Flags, "-Wl,-rpath,\\$ORIGIN");
        ssTEST_OUTPUT_ASSERT("Unix Executable executable", unixInfo.Executable, "g++");
        ssTEST_OUTPUT_ASSERT("Unix Executable setup count", unixInfo.Setup.size(), 1);
        ssTEST_OUTPUT_ASSERT("Unix Executable cleanup count", unixInfo.Cleanup.size(), 1);
        ssTEST_OUTPUT_ASSERT("Unix Executable run parts count", unixInfo.RunParts.size(), 2);
        ssTEST_OUTPUT_ASSERT(   "Unix Executable first run type", 
                                unixInfo.RunParts.at(0).Type == 
                                runcpp2::Data::StageInfo::RunPart::RunType::ONCE);
        ssTEST_OUTPUT_ASSERT(   "Unix Executable second run command", 
                                unixInfo.RunParts.at(1).CommandPart, " \"{LinkFilePath}\"");
        
        //Verify Unix OutputTypeInfo for ExecutableShared
        ssTEST_OUTPUT_SETUP
        (
            const auto& unixExecSharedInfo = stageInfo.OutputTypes.ExecutableShared.at("Unix");
        );
        ssTEST_OUTPUT_ASSERT(   "Unix ExecutableShared flags", 
                                unixExecSharedInfo.Flags, 
                                "-shared -Wl,-rpath,\\$ORIGIN");
        ssTEST_OUTPUT_ASSERT(   "Unix ExecutableShared executable", 
                                unixExecSharedInfo.Executable, 
                                "g++");
        ssTEST_OUTPUT_ASSERT(   "Unix ExecutableShared run parts count", 
                                unixExecSharedInfo.RunParts.size(), 
                                2);
        ssTEST_OUTPUT_ASSERT(   "Unix ExecutableShared first run type", 
                                unixExecSharedInfo.RunParts.at(0).Type ==
                                runcpp2::Data::StageInfo::RunPart::RunType::ONCE);
        ssTEST_OUTPUT_ASSERT(   "Unix ExecutableShared second run command", 
                                unixExecSharedInfo.RunParts.at(1).CommandPart, 
                                " \"{LinkFilePath}\"");
        
        //Test ToString() and Equals()
        ssTEST_OUTPUT_EXECUTION
        (
            std::string yamlOutput = stageInfo.ToString("", "LinkTypes");
            ryml::Tree outputTree = ryml::parse_in_arena(ryml::to_csubstr(yamlOutput));
            
            runcpp2::Data::StageInfo parsedOutput;
            nodeRef = outputTree.rootref();
            parsedOutput.ParseYAML_Node(nodeRef, "LinkTypes");
        );
        
        ssTEST_OUTPUT_ASSERT("Parsed output should equal original", stageInfo.Equals(parsedOutput));
    };
    
    ssTEST("StageInfo Should Handle Malformed YAML")
    {
        const char* malformedYamlStr = R"(
            LinkTypes:
                ExecutableShared:  # Missing required fields
                    Unix:
                        Flags: "-shared"
                        # Missing Executable
                        # Missing RunParts
        )";
        
        ssTEST_OUTPUT_SETUP
        (
            ryml::Tree tree = ryml::parse_in_arena(c4::to_csubstr(malformedYamlStr));
            ryml::ConstNodeRef root = tree.rootref();
            
            runcpp2::Data::StageInfo stageInfo;
        );
        
        ssTEST_OUTPUT_EXECUTION
        (
            ryml::ConstNodeRef nodeRef = root;
            bool parseResult = stageInfo.ParseYAML_Node(nodeRef, "LinkTypes");
        );
        
        ssTEST_OUTPUT_ASSERT("ParseYAML_Node should fail for malformed YAML", !parseResult);
    };

    ssTEST("StageInfo Should Handle Missing ExecutableShared")
    {
        const char* yamlStr = R"(
            LinkTypes:
                Executable:
                    Unix:
                        Flags: "-Wl,-rpath,\\$ORIGIN"
                        Executable: "g++"
                        RunParts:
                        -   Type: Once
                            CommandPart: "{Executable} {LinkFlags}"
                # Missing ExecutableShared section
        )";
        
        ssTEST_OUTPUT_SETUP
        (
            ryml::Tree tree = ryml::parse_in_arena(c4::to_csubstr(yamlStr));
            ryml::ConstNodeRef root = tree.rootref();
            
            runcpp2::Data::StageInfo stageInfo;
        );
        
        ssTEST_OUTPUT_EXECUTION
        (
            ryml::ConstNodeRef nodeRef = root;
            bool parseResult = stageInfo.ParseYAML_Node(nodeRef, "LinkTypes");
        );
        
        ssTEST_OUTPUT_ASSERT(   "ParseYAML_Node should fail for missing ExecutableShared", 
                                !parseResult);
    };
    
    ssTEST_END_TEST_GROUP();
    return 0;
}
