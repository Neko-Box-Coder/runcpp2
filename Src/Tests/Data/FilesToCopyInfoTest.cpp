#include "runcpp2/Data/FilesToCopyInfo.hpp"
#include "ssTest.hpp"
#include "runcpp2/YamlLib.hpp"
#include "runcpp2/runcpp2.hpp"
#include "runcpp2/DeferUtil.hpp"

int main(int argc, char** argv)
{
    runcpp2::SetLogLevel("Warning");
    
    ssTEST_INIT_TEST_GROUP();
    
    ssTEST("FilesToCopyInfo Should Parse Valid YAML")
    {
        ssTEST_OUTPUT_SETUP
        (
            //NOTE: This is just a test YAML for validating parsing, don't use it for actual config
            const char* yamlStr = R"(
                MSVC:
                -   bin/Debug/mylib.dll
                -   bin/Debug/mylib.pdb
                GCC:
                -   lib/libmylib.so
                -   lib/libmylib.so.1
            )";
        );
        
        runcpp2::YAML::ResourceHandle resource;
        std::vector<runcpp2::YAML::NodePtr> roots = 
            runcpp2::YAML::ParseYAML(yamlStr, resource).DS_TRY_ACT(ssTEST_OUTPUT_ASSERT("", false));
        DEFER { FreeYAMLResource(resource); };
        
        ssTEST_OUTPUT_ASSERT("", roots.size() == 1);
        runcpp2::YAML::NodePtr root = roots.front();
        runcpp2::Data::FilesToCopyInfo filesToCopy;
        
        ssTEST_OUTPUT_EXECUTION
        (
            bool parseResult = filesToCopy.ParseYAML_Node(root);
        );
        
        ssTEST_OUTPUT_ASSERT("ParseYAML_Node should succeed", parseResult);
        
        //Verify parsed values
        ssTEST_OUTPUT_ASSERT(   "MSVC files count", 
                                filesToCopy.ProfileFiles.at("MSVC").size() == 2);
        ssTEST_OUTPUT_ASSERT(   "GCC files count", 
                                filesToCopy.ProfileFiles.at("GCC").size() == 2);
        ssTEST_OUTPUT_ASSERT(   "MSVC first file", 
                                filesToCopy.ProfileFiles.at("MSVC").at(0) == "bin/Debug/mylib.dll");
        ssTEST_OUTPUT_ASSERT(   "GCC last file", 
                                filesToCopy.ProfileFiles.at("GCC").at(1) == "lib/libmylib.so.1");
        
        //Test ToString() and Equals()
        ssTEST_OUTPUT_EXECUTION
        (
            std::string yamlOutput = filesToCopy.ToString("");
            roots = 
                runcpp2::YAML::ParseYAML(   yamlOutput, 
                                            resource).DS_TRY_ACT(ssTEST_OUTPUT_ASSERT("", false));
            ssTEST_OUTPUT_ASSERT("", roots.size() == 1);
            
            runcpp2::Data::FilesToCopyInfo parsedOutput;
            parsedOutput.ParseYAML_Node(roots.front());
        );
        
        ssTEST_OUTPUT_ASSERT(   "Parsed output should equal original", 
                                filesToCopy.Equals(parsedOutput));
    };
    
    ssTEST_END_TEST_GROUP();
    return 0;
}
