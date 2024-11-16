#include "runcpp2/Data/ProfilesProcessPaths.hpp"
#include "ssTest.hpp"
#include "runcpp2/YamlLib.hpp"
#include "runcpp2/runcpp2.hpp"

int main(int argc, char** argv)
{
    runcpp2::SetLogLevel("Warning");
    
    ssTEST_INIT_TEST_GROUP();
    
    ssTEST("ProfilesProcessPaths Should Parse Valid YAML")
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
            
            runcpp2::Data::ProfilesProcessPaths profilesProcessPaths;
        );
        
        ssTEST_OUTPUT_EXECUTION
        (
            ryml::ConstNodeRef nodeRef = root;
            bool parseResult = profilesProcessPaths.ParseYAML_Node(nodeRef);
        );
        
        ssTEST_OUTPUT_ASSERT("ParseYAML_Node should succeed", parseResult);
        
        //Verify parsed values
        ssTEST_OUTPUT_ASSERT(   "MSVC files count", 
                                profilesProcessPaths.Paths.at("MSVC").size() == 2);
        ssTEST_OUTPUT_ASSERT(   "GCC files count", 
                                profilesProcessPaths.Paths.at("GCC").size() == 2);
        ssTEST_OUTPUT_ASSERT(   "MSVC first file", 
                                profilesProcessPaths.Paths.at("MSVC").at(0) == 
                                "src/main.cpp");
        ssTEST_OUTPUT_ASSERT(   "GCC last file", 
                                profilesProcessPaths.Paths.at("GCC").at(1) == 
                                "src/optimized.cpp");
        
        //Test ToString() and Equals()
        ssTEST_OUTPUT_EXECUTION
        (
            std::string yamlOutput = profilesProcessPaths.ToString("");
            ryml::Tree outputTree = ryml::parse_in_arena(ryml::to_csubstr(yamlOutput));
            
            runcpp2::Data::ProfilesProcessPaths parsedOutput;
            nodeRef = outputTree.rootref();
            parsedOutput.ParseYAML_Node(nodeRef);
        );
        
        ssTEST_OUTPUT_ASSERT(   "Parsed output should equal original", 
                                profilesProcessPaths.Equals(parsedOutput));
    };
    
    ssTEST_END_TEST_GROUP();
    return 0;
}
