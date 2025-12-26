#include "runcpp2/Data/FilesTypesInfo.hpp"
#include "ssTest.hpp"
#include "runcpp2/YamlLib.hpp"
#include "runcpp2/runcpp2.hpp"
#include "runcpp2/DeferUtil.hpp"

int main(int argc, char** argv)
{
    runcpp2::SetLogLevel("Warning");
    
    ssTEST_INIT_TEST_GROUP();
    
    ssTEST("FilesTypesInfo Should Parse Valid YAML")
    {
        ssTEST_OUTPUT_SETUP
        (
            //NOTE: This is just a test YAML for validating parsing, don't use it for actual config
            const char* yamlStr = R"(
                ObjectLinkFile:
                    Prefix:
                        MSVC: ""
                        GCC: ""
                    Extension:
                        MSVC: .obj
                        GCC: .o
                SharedLinkFile:
                    Prefix:
                        MSVC: ""
                        GCC: lib
                    Extension:
                        MSVC: .lib
                        GCC: .so
                SharedLibraryFile:
                    Prefix:
                        MSVC: ""
                        GCC: lib
                    Extension:
                        MSVC: .dll
                        GCC: .so
                StaticLinkFile:
                    Prefix:
                        MSVC: ""
                        GCC: lib
                    Extension:
                        MSVC: .lib
                        GCC: .a
                DebugSymbolFile:
                    Prefix:
                        MSVC: ""
                        GCC: ""
                    Extension:
                        MSVC: .pdb
                        GCC: .debug
            )";
        );
        
        runcpp2::YAML::ResourceHandle resource;
        std::vector<runcpp2::YAML::NodePtr> roots = 
            runcpp2::YAML::ParseYAML(yamlStr, resource).DS_TRY_ACT(ssTEST_OUTPUT_ASSERT("", false));
        DEFER { FreeYAMLResource(resource); };
        
        ssTEST_OUTPUT_ASSERT("", roots.size() == 1);
        runcpp2::YAML::NodePtr root = roots.front();
        runcpp2::Data::FilesTypesInfo filesTypesInfo;
        
        ssTEST_OUTPUT_EXECUTION
        (
            bool parseResult = filesTypesInfo.ParseYAML_Node(root);
        );
        
        ssTEST_OUTPUT_ASSERT("ParseYAML_Node should succeed", parseResult);
        
        //Verify parsed values for ObjectLinkFile
        ssTEST_OUTPUT_ASSERT(   "MSVC obj extension", 
                                filesTypesInfo.ObjectLinkFile.Extension.at("MSVC") == ".obj");
        ssTEST_OUTPUT_ASSERT(   "GCC obj extension", 
                                filesTypesInfo.ObjectLinkFile.Extension.at("GCC") == ".o");
        
        //Verify parsed values for SharedLinkFile
        ssTEST_OUTPUT_ASSERT(   "GCC shared prefix", 
                                filesTypesInfo.SharedLinkFile.Prefix.at("GCC") == "lib");
        ssTEST_OUTPUT_ASSERT(   "MSVC shared extension", 
                                filesTypesInfo.SharedLinkFile.Extension.at("MSVC") == ".lib");
        
        //Verify parsed values for StaticLinkFile
        ssTEST_OUTPUT_ASSERT(   "GCC static prefix", 
                                filesTypesInfo.StaticLinkFile.Prefix.at("GCC") == "lib");
        ssTEST_OUTPUT_ASSERT(   "GCC static extension", 
                                filesTypesInfo.StaticLinkFile.Extension.at("GCC") == ".a");
        
        //Test ToString() and Equals()
        ssTEST_OUTPUT_EXECUTION
        (
            std::string yamlOutput = filesTypesInfo.ToString("");
            roots = 
                runcpp2::YAML::ParseYAML(   yamlOutput, 
                                            resource).DS_TRY_ACT(ssTEST_OUTPUT_ASSERT("", false));
            ssTEST_OUTPUT_ASSERT("", roots.size() == 1);
            
            runcpp2::Data::FilesTypesInfo parsedOutput;
            parsedOutput.ParseYAML_Node(roots.front());
        );
        
        ssTEST_OUTPUT_ASSERT(   "Parsed output should equal original", 
                                filesTypesInfo.Equals(parsedOutput));
    };
    
    ssTEST_END_TEST_GROUP();
    return 0;
}
