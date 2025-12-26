#include "runcpp2/Data/FileProperties.hpp"
#include "ssTest.hpp"
#include "runcpp2/runcpp2.hpp"
#include "runcpp2/YamlLib.hpp"
#include "runcpp2/DeferUtil.hpp"

int main(int argc, char** argv)
{
    runcpp2::SetLogLevel("Warning");
    
    ssTEST_INIT_TEST_GROUP();
    
    ssTEST("FileProperties Should Parse Valid YAML")
    {
        ssTEST_OUTPUT_SETUP
        (
            //NOTE: This is just a test YAML for validating parsing, don't use it for actual config
            const char* yamlStr = R"(
                Prefix:
                    Windows: lib
                    Linux: lib
                Extension:
                    Windows: .dll
                    Linux: .so
            )";
        );
        
        runcpp2::YAML::ResourceHandle resource;
        std::vector<runcpp2::YAML::NodePtr> roots = 
            runcpp2::YAML::ParseYAML(yamlStr, resource).DS_TRY_ACT(ssTEST_OUTPUT_ASSERT("", false));
        DEFER { FreeYAMLResource(resource); };
        
        ssTEST_OUTPUT_ASSERT("", roots.size() == 1);
        runcpp2::YAML::NodePtr root = roots.front();
        runcpp2::Data::FileProperties fileProperties;
        
        ssTEST_OUTPUT_EXECUTION
        (
            bool parseResult = fileProperties.ParseYAML_Node(root);
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
            roots = 
                runcpp2::YAML::ParseYAML(   yamlOutput, 
                                            resource).DS_TRY_ACT(ssTEST_OUTPUT_ASSERT("", false));
            ssTEST_OUTPUT_ASSERT("", roots.size() == 1);
            
            runcpp2::Data::FileProperties parsedOutput;
            parsedOutput.ParseYAML_Node(roots.front());
        );
        
        ssTEST_OUTPUT_ASSERT(   "Parsed output should equal original", 
                                fileProperties.Equals(parsedOutput));
    };
    
    ssTEST_END_TEST_GROUP();
    return 0;
}
