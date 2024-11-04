#include "runcpp2/Data/FileProperties.hpp"
#include "ssTest.hpp"
#include "runcpp2/runcpp2.hpp"
#include "runcpp2/YamlLib.hpp"

int main(int argc, char** argv)
{
    runcpp2::SetLogLevel("Warning");
    
    ssTEST_INIT_TEST_GROUP();
    
    ssTEST("FileProperties Should Parse Valid YAML")
    {
        ssTEST_OUTPUT_SETUP
        (
            const char* yamlStr = R"(
                Prefix:
                    Windows: lib
                    Linux: lib
                Extension:
                    Windows: .dll
                    Linux: .so
            )";
            
            ryml::Tree tree = ryml::parse_in_arena(c4::to_csubstr(yamlStr));
            ryml::ConstNodeRef root = tree.rootref();
            runcpp2::Data::FileProperties fileProperties;
        );
        
        ssTEST_OUTPUT_EXECUTION
        (
            ryml::ConstNodeRef nodeRef = root;
            bool parseResult = fileProperties.ParseYAML_Node(nodeRef);
        );
        
        ssTEST_OUTPUT_ASSERT("ParseYAML_Node should succeed", parseResult);
        
        //Verify parsed values
        ssTEST_OUTPUT_ASSERT("Windows prefix", fileProperties.Prefix.at("Windows") == "lib");
        ssTEST_OUTPUT_ASSERT("Linux prefix", fileProperties.Prefix.at("Linux") == "lib");
        ssTEST_OUTPUT_ASSERT("Windows extension", fileProperties.Extension.at("Windows") == ".dll");
        ssTEST_OUTPUT_ASSERT("Linux extension", fileProperties.Extension.at("Linux") == ".so");
        
        //Test ToString() and Equals()
        ssTEST_OUTPUT_EXECUTION
        (
            std::string yamlOutput = fileProperties.ToString("");
            ryml::Tree outputTree = ryml::parse_in_arena(ryml::to_csubstr(yamlOutput));
            
            runcpp2::Data::FileProperties parsedOutput;
            nodeRef = outputTree.rootref();
            parsedOutput.ParseYAML_Node(nodeRef);
        );
        
        ssTEST_OUTPUT_ASSERT(   "Parsed output should equal original", 
                                fileProperties.Equals(parsedOutput));
    };
    
    ssTEST_END_TEST_GROUP();
    return 0;
}
