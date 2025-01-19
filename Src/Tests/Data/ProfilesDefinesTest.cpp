#include "runcpp2/Data/ProfilesDefines.hpp"
#include "ssTest.hpp"
#include "runcpp2/YamlLib.hpp"
#include "runcpp2/runcpp2.hpp"

int main(int argc, char** argv)
{
    runcpp2::SetLogLevel("Warning");
    
    ssTEST_INIT_TEST_GROUP();
    
    ssTEST("ProfilesDefines Should Parse Valid YAML")
    {
        ssTEST_OUTPUT_SETUP
        (
            //NOTE: This is just a test YAML for validating parsing, don't use it for actual config
            const char* yamlStr = R"(
                MSVC:
                -   _WIN32
                -   UNICODE
                -   VERSION=1.0.0
                GCC:
                -   __linux__
                -   DEBUG_LEVEL=2
                -   ENABLE_LOGGING
            )";
            
            ryml::Tree tree = ryml::parse_in_arena(c4::to_csubstr(yamlStr));
            ryml::ConstNodeRef root = tree.rootref();
            
            runcpp2::Data::ProfilesDefines profilesDefines;
        );
        
        ssTEST_OUTPUT_EXECUTION
        (
            ryml::ConstNodeRef nodeRef = root;
            bool parseResult = profilesDefines.ParseYAML_Node(nodeRef);
        );
        
        ssTEST_OUTPUT_ASSERT("ParseYAML_Node should succeed", parseResult);
        
        //Verify parsed values
        ssTEST_OUTPUT_ASSERT("MSVC defines count", profilesDefines.Defines.at("MSVC").size() == 3);
        ssTEST_OUTPUT_ASSERT("GCC defines count", profilesDefines.Defines.at("GCC").size() == 3);
        
        //Test MSVC defines
        ssTEST_OUTPUT_SETUP
        (
            const auto& msvcDefines = profilesDefines.Defines.at("MSVC");
        );
        ssTEST_OUTPUT_ASSERT("MSVC first define name", msvcDefines.at(0).Name == "_WIN32");
        ssTEST_OUTPUT_ASSERT("MSVC first define has no value", !msvcDefines.at(0).HasValue);
        ssTEST_OUTPUT_ASSERT("MSVC third define name", msvcDefines.at(2).Name == "VERSION");
        ssTEST_OUTPUT_ASSERT("MSVC third define value", msvcDefines.at(2).Value == "1.0.0");
        ssTEST_OUTPUT_ASSERT("MSVC third define has value", msvcDefines.at(2).HasValue);
        
        //Test GCC defines
        ssTEST_OUTPUT_SETUP
        (
            const auto& gccDefines = profilesDefines.Defines.at("GCC");
        );
        ssTEST_OUTPUT_ASSERT("GCC second define name", gccDefines.at(1).Name == "DEBUG_LEVEL");
        ssTEST_OUTPUT_ASSERT("GCC second define value", gccDefines.at(1).Value == "2");
        ssTEST_OUTPUT_ASSERT("GCC second define has value", gccDefines.at(1).HasValue);
        ssTEST_OUTPUT_ASSERT("GCC third define name", gccDefines.at(2).Name == "ENABLE_LOGGING");
        ssTEST_OUTPUT_ASSERT("GCC third define has no value", !gccDefines.at(2).HasValue);
        
        //Test ToString() and Equals()
        ssTEST_OUTPUT_EXECUTION
        (
            std::string yamlOutput = profilesDefines.ToString("");
            ryml::Tree outputTree = ryml::parse_in_arena(ryml::to_csubstr(yamlOutput));
            
            runcpp2::Data::ProfilesDefines parsedOutput;
            nodeRef = outputTree.rootref();
            parsedOutput.ParseYAML_Node(nodeRef);
        );
        
        ssTEST_OUTPUT_ASSERT(   "Parsed output should equal original", 
                                profilesDefines.Equals(parsedOutput));
    };
    
    ssTEST_END_TEST_GROUP();
    return 0;
}
