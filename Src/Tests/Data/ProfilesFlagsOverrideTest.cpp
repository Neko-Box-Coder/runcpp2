#include "runcpp2/Data/ProfilesFlagsOverride.hpp"
#include "ssTest.hpp"
#include "runcpp2/YamlLib.hpp"
#include "runcpp2/runcpp2.hpp"
#include "runcpp2/DeferUtil.hpp"

int main(int argc, char** argv)
{
    runcpp2::SetLogLevel("Warning");
    
    ssTEST_INIT_TEST_GROUP();
    
    ssTEST("ProfilesFlagsOverride Should Parse Valid YAML")
    {
        ssTEST_OUTPUT_SETUP
        (
            //NOTE: This is just a test YAML for validating parsing, don't use it for actual config
            const char* yamlStr = R"(
                MSVC:
                    Remove: /W4
                    Append: /W3 /WX
                GCC:
                    Remove: -Wall
                    Append: -Wextra -Werror
            )";
        );
        
        runcpp2::YAML::ResourceHandle resource;
        std::vector<runcpp2::YAML::NodePtr> roots = 
            runcpp2::YAML::ParseYAML(yamlStr, resource).DS_TRY_ACT(ssTEST_OUTPUT_ASSERT("", false));
        DEFER { FreeYAMLResource(resource); };
        
        ssTEST_OUTPUT_ASSERT("", roots.size() == 1);
        runcpp2::YAML::NodePtr root = roots.front();
        runcpp2::Data::ProfilesFlagsOverride profilesFlagsOverride;
        
        ssTEST_OUTPUT_EXECUTION
        (
            bool parseResult = profilesFlagsOverride.ParseYAML_Node(root);
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
            roots = 
                runcpp2::YAML::ParseYAML(   yamlOutput, 
                                            resource).DS_TRY_ACT(ssTEST_OUTPUT_ASSERT("", false));
            ssTEST_OUTPUT_ASSERT("", roots.size() == 1);
            
            runcpp2::Data::ProfilesFlagsOverride parsedOutput;
            parsedOutput.ParseYAML_Node(roots.front());
        );
        
        ssTEST_OUTPUT_ASSERT(   "Parsed output should equal original", 
                                profilesFlagsOverride.Equals(parsedOutput));
    };
    
    ssTEST_END_TEST_GROUP();
    return 0;
}
