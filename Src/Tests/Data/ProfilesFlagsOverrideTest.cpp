#include "runcpp2/Data/ProfilesFlagsOverride.hpp"
#include "ssTest.hpp"
#include "runcpp2/YamlLib.hpp"
#include "runcpp2/runcpp2.hpp"

int main(int argc, char** argv)
{
    runcpp2::SetLogLevel("Warning");
    
    ssTEST_INIT_TEST_GROUP();
    
    ssTEST("ProfilesFlagsOverride Should Parse Valid YAML")
    {
        ssTEST_OUTPUT_SETUP
        (
            const char* yamlStr = R"(
                MSVC:
                    Remove: /W4
                    Append: /W3 /WX
                GCC:
                    Remove: -Wall
                    Append: -Wextra -Werror
            )";
            
            ryml::Tree tree = ryml::parse_in_arena(c4::to_csubstr(yamlStr));
            ryml::ConstNodeRef root = tree.rootref();
            
            runcpp2::Data::ProfilesFlagsOverride profilesFlagsOverride;
        );
        
        ssTEST_OUTPUT_EXECUTION
        (
            ryml::ConstNodeRef nodeRef = root;
            bool parseResult = profilesFlagsOverride.ParseYAML_Node(nodeRef);
        );
        
        ssTEST_OUTPUT_ASSERT("ParseYAML_Node should succeed", parseResult);
        
        //Verify parsed values for MSVC
        ssTEST_OUTPUT_SETUP
        (
            const auto& msvcFlags = profilesFlagsOverride.FlagsOverrides.at("MSVC");
        );
        ssTEST_OUTPUT_ASSERT("MSVC remove flag", msvcFlags.Remove == "/W4");
        ssTEST_OUTPUT_ASSERT("MSVC append flag", msvcFlags.Append == "/W3 /WX");
        
        //Verify parsed values for GCC
        ssTEST_OUTPUT_SETUP
        (
            const auto& gccFlags = profilesFlagsOverride.FlagsOverrides.at("GCC");
        );
        ssTEST_OUTPUT_ASSERT("GCC remove flag", gccFlags.Remove == "-Wall");
        ssTEST_OUTPUT_ASSERT("GCC append flag", gccFlags.Append == "-Wextra -Werror");
        
        // Test ToString() and Equals()
        ssTEST_OUTPUT_EXECUTION
        (
            std::string yamlOutput = profilesFlagsOverride.ToString("");
            ryml::Tree outputTree = ryml::parse_in_arena(ryml::to_csubstr(yamlOutput));
            
            runcpp2::Data::ProfilesFlagsOverride parsedOutput;
            nodeRef = outputTree.rootref();
            parsedOutput.ParseYAML_Node(nodeRef);
        );
        
        ssTEST_OUTPUT_ASSERT(   "Parsed output should equal original", 
                                profilesFlagsOverride.Equals(parsedOutput));
    };
    
    ssTEST_END_TEST_GROUP();
    return 0;
}
