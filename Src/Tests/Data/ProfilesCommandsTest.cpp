#include "runcpp2/Data/ProfilesCommands.hpp"
#include "ssTest.hpp"
#include "runcpp2/YamlLib.hpp"
#include "runcpp2/runcpp2.hpp"
#include "runcpp2/DeferUtil.hpp"

int main(int argc, char** argv)
{
    runcpp2::SetLogLevel("Warning");
    
    ssTEST_INIT_TEST_GROUP();
    
    ssTEST("ProfilesCommands Should Parse Valid YAML")
    {
        ssTEST_OUTPUT_SETUP
        (
            //NOTE: This is just a test YAML for validating parsing, don't use it for actual config
            const char* yamlStr = R"(
                MSVC:
                -   mkdir build
                -   cd build
                -   cmake ..
                GCC:
                -   ./configure
                -   make
                -   make install
            )";
        );
        
        runcpp2::YAML::ResourceHandle resource;
        std::vector<runcpp2::YAML::NodePtr> roots = 
            runcpp2::YAML::ParseYAML(yamlStr, resource).DS_TRY_ACT(ssTEST_OUTPUT_ASSERT("", false));
        DEFER { FreeYAMLResource(resource); };
        
        ssTEST_OUTPUT_ASSERT("", roots.size() == 1);
        runcpp2::YAML::NodePtr root = roots.front();
        runcpp2::Data::ProfilesCommands profilesCommands;
        
        ssTEST_OUTPUT_EXECUTION
        (
            bool parseResult = profilesCommands.ParseYAML_Node(root);
        );
        
        ssTEST_OUTPUT_ASSERT("ParseYAML_Node should succeed", parseResult);
        
        //Verify parsed values
        ssTEST_OUTPUT_ASSERT(   "MSVC commands count", 
                                profilesCommands.CommandSteps.at("MSVC").size() == 3);
        ssTEST_OUTPUT_ASSERT(   "GCC commands count", 
                                profilesCommands.CommandSteps.at("GCC").size() == 3);
        ssTEST_OUTPUT_ASSERT(   "MSVC first command", 
                                profilesCommands.CommandSteps.at("MSVC").at(0) == "mkdir build");
        ssTEST_OUTPUT_ASSERT(   "GCC last command", 
                                profilesCommands.CommandSteps.at("GCC").at(2) == "make install");
        
        //Test ToString() and Equals()
        ssTEST_OUTPUT_EXECUTION
        (
            std::string yamlOutput = profilesCommands.ToString("");
            roots = 
                runcpp2::YAML::ParseYAML(   yamlOutput, 
                                            resource).DS_TRY_ACT(ssTEST_OUTPUT_ASSERT("", false));
            ssTEST_OUTPUT_ASSERT("", roots.size() == 1);
            
            runcpp2::Data::ProfilesCommands parsedOutput;
            parsedOutput.ParseYAML_Node(roots.front());
        );
        
        ssTEST_OUTPUT_ASSERT(   "Parsed output should equal original", 
                                profilesCommands.Equals(parsedOutput));
    };
    
    ssTEST_END_TEST_GROUP();
    return 0;
}
