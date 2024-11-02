#include "runcpp2/Data/ProfilesCompilesFiles.hpp"
#include "ssTest.hpp"
#include "ryml.hpp"
#include "c4/std/string.hpp"
#include "runcpp2/runcpp2.hpp"

int main(int argc, char** argv)
{
    runcpp2::SetLogLevel("Warning");
    
    ssTEST_INIT_TEST_GROUP();
    
    ssTEST("ProfilesCompilesFiles Should Parse Valid YAML")
    {
        ssTEST_OUTPUT_SETUP
        (
            const char* yamlStr = R"(
                MSVC:
                -   src/main.cpp
                -   src/utils.cpp
                GCC:
                -   src/main.cpp
                -   src/optimized.cpp
            )";
            
            ryml::Tree tree = ryml::parse_in_arena(c4::to_csubstr(yamlStr));
            ryml::ConstNodeRef root = tree.rootref();
            
            runcpp2::Data::ProfilesCompilesFiles profilesCompilesFiles;
        );
        
        ssTEST_OUTPUT_EXECUTION
        (
            ryml::ConstNodeRef nodeRef = root;
            bool parseResult = profilesCompilesFiles.ParseYAML_Node(nodeRef);
        );
        
        ssTEST_OUTPUT_ASSERT("ParseYAML_Node should succeed", parseResult);
        
        //Verify parsed values
        ssTEST_OUTPUT_ASSERT(   "MSVC files count", 
                                profilesCompilesFiles.CompilesFiles.at("MSVC").size() == 2);
        ssTEST_OUTPUT_ASSERT(   "GCC files count", 
                                profilesCompilesFiles.CompilesFiles.at("GCC").size() == 2);
        ssTEST_OUTPUT_ASSERT(   "MSVC first file", 
                                profilesCompilesFiles.CompilesFiles.at("MSVC").at(0) == 
                                "src/main.cpp");
        ssTEST_OUTPUT_ASSERT(   "GCC last file", 
                                profilesCompilesFiles.CompilesFiles.at("GCC").at(1) == 
                                "src/optimized.cpp");
        
        //Test ToString() and Equals()
        ssTEST_OUTPUT_EXECUTION
        (
            std::string yamlOutput = profilesCompilesFiles.ToString("");
            ryml::Tree outputTree = ryml::parse_in_arena(ryml::to_csubstr(yamlOutput));
            
            runcpp2::Data::ProfilesCompilesFiles parsedOutput;
            nodeRef = outputTree.rootref();
            parsedOutput.ParseYAML_Node(nodeRef);
        );
        
        ssTEST_OUTPUT_ASSERT(   "Parsed output should equal original", 
                                profilesCompilesFiles.Equals(parsedOutput));
    };
    
    ssTEST_END_TEST_GROUP();
    return 0;
}
