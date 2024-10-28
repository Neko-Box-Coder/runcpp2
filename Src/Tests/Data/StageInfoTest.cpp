#include "runcpp2/Data/StageInfo.hpp"
#include "ssTest.hpp"
#include "ryml.hpp"
#include "c4/std/string.hpp"
#include "runcpp2/runcpp2.hpp"

int main(int argc, char** argv)
{
    runcpp2::SetLogLevel("Warning");
    
    ssTEST_INIT_TEST_GROUP();
    
    ssTEST("StageInfo Should Parse Valid YAML")
    {
        ssTEST_OUTPUT_SETUP
        (
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
        ssTEST_OUTPUT_ASSERT("Unix flags", unixInfo.Flags == "-Wl,-rpath,\\$ORIGIN");
        ssTEST_OUTPUT_ASSERT("Unix executable", unixInfo.Executable == "g++");
        ssTEST_OUTPUT_ASSERT("Unix setup count", unixInfo.Setup.size() == 1);
        ssTEST_OUTPUT_ASSERT("Unix cleanup count", unixInfo.Cleanup.size() == 1);
        ssTEST_OUTPUT_ASSERT("Unix run parts count", unixInfo.RunParts.size() == 2);
        ssTEST_OUTPUT_ASSERT(   "Unix first run type", 
                                unixInfo.RunParts.at(0).Type == 
                                runcpp2::Data::StageInfo::RunPart::RunType::ONCE);
        ssTEST_OUTPUT_ASSERT(   "Unix second run command", 
                                unixInfo.RunParts.at(1).CommandPart, " \"{LinkFilePath}\"");
        
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
    
    ssTEST_END_TEST_GROUP();
    return 0;
}
