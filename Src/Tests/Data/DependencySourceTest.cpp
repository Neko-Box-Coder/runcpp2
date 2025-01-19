#include "runcpp2/Data/DependencySource.hpp"
#include "ssTest.hpp"
#include "runcpp2/YamlLib.hpp"
#include "runcpp2/runcpp2.hpp"

int main(int argc, char** argv)
{
    runcpp2::SetLogLevel("Warning");
    
    ssTEST_INIT_TEST_GROUP();
    
    ssTEST("DependencySource Should Parse Git Source")
    {
        ssTEST_OUTPUT_SETUP
        (
            //NOTE: This is just a test YAML for validating parsing, don't use it for actual config
            const char* yamlStr = R"(
                Git:
                    URL: https://github.com/user/repo.git
            )";
            
            ryml::Tree tree = ryml::parse_in_arena(c4::to_csubstr(yamlStr));
            ryml::ConstNodeRef root = tree.rootref();
            
            runcpp2::Data::DependencySource dependencySource;
        );
        
        ssTEST_OUTPUT_EXECUTION
        (
            ryml::ConstNodeRef nodeRef = root;
            bool parseResult = dependencySource.ParseYAML_Node(nodeRef);
        );
        
        ssTEST_OUTPUT_ASSERT("ParseYAML_Node should succeed", parseResult);
        
        //Verify parsed values
        const runcpp2::Data::GitSource* git = 
            mpark::get_if<runcpp2::Data::GitSource>(&dependencySource.Source);
        ssTEST_OUTPUT_ASSERT("Should be Git source", git != nullptr);
        ssTEST_OUTPUT_ASSERT("URL", git->URL == "https://github.com/user/repo.git");
        
        //Test ToString() and Equals()
        ssTEST_OUTPUT_EXECUTION
        (
            std::string yamlOutput = dependencySource.ToString("");
            ryml::Tree outputTree = ryml::parse_in_arena(ryml::to_csubstr(yamlOutput));
            
            runcpp2::Data::DependencySource parsedOutput;
            nodeRef = outputTree.rootref();
            parsedOutput.ParseYAML_Node(nodeRef);
        );
        
        ssTEST_OUTPUT_ASSERT(   "Parsed output should equal original", 
                                dependencySource.Equals(parsedOutput));
    };
    
    ssTEST("DependencySource Should Parse Local Source")
    {
        ssTEST_OUTPUT_SETUP
        (
            //NOTE: This is just a test YAML for validating parsing, don't use it for actual config
            const char* yamlStr = R"(
                Local:
                    Path: ../external/mylib
            )";
            
            ryml::Tree tree = ryml::parse_in_arena(c4::to_csubstr(yamlStr));
            ryml::ConstNodeRef root = tree.rootref();
            
            runcpp2::Data::DependencySource dependencySource;
        );
        
        ssTEST_OUTPUT_EXECUTION
        (
            ryml::ConstNodeRef nodeRef = root;
            bool parseResult = dependencySource.ParseYAML_Node(nodeRef);
        );
        
        ssTEST_OUTPUT_ASSERT("ParseYAML_Node should succeed", parseResult);
        
        //Verify parsed values
        const runcpp2::Data::LocalSource* local = 
            mpark::get_if<runcpp2::Data::LocalSource>(&dependencySource.Source);
        ssTEST_OUTPUT_ASSERT("Should be Local source", local != nullptr);
        ssTEST_OUTPUT_ASSERT("Path", local->Path == "../external/mylib");
        
        //Test ToString() and Equals()
        ssTEST_OUTPUT_EXECUTION
        (
            std::string yamlOutput = dependencySource.ToString("");
            ryml::Tree outputTree = ryml::parse_in_arena(ryml::to_csubstr(yamlOutput));
            
            runcpp2::Data::DependencySource parsedOutput;
            nodeRef = outputTree.rootref();
            parsedOutput.ParseYAML_Node(nodeRef);
        );
        
        ssTEST_OUTPUT_ASSERT(   "Parsed output should equal original", 
                                dependencySource.Equals(parsedOutput));
    };
    
    ssTEST_END_TEST_GROUP();
    return 0;
}
