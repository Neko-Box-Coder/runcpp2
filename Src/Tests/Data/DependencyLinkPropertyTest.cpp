#include "runcpp2/Data/DependencyLinkProperty.hpp"
#include "ssTest.hpp"
#include "runcpp2/YamlLib.hpp"
#include "runcpp2/runcpp2.hpp"
#include "runcpp2/DeferUtil.hpp"

int main(int argc, char** argv)
{
    runcpp2::SetLogLevel("Warning");
    
    ssTEST_INIT_TEST_GROUP();
    
    ssTEST("DependencyLinkProperty Should Parse Valid YAML")
    {
        ssTEST_OUTPUT_SETUP
        (
            //NOTE: This is just a test YAML for validating parsing, don't use it for actual config
            const char* yamlStr = R"(
                MSVC:
                    SearchLibraryNames: [mylib, mylib_static]
                    SearchDirectories: ["lib/Debug", "lib/Release"]
                    ExcludeLibraryNames: [mylib_test]
                    AdditionalLinkOptions: ["/NODEFAULTLIB"]
                GCC:
                    SearchLibraryNames: [mylib]
                    SearchDirectories: ["/usr/local/lib"]
                    AdditionalLinkOptions: ["-Wl,-rpath=."]
            )";
        );
        
        runcpp2::YAML::ResourceHandle resource;
        std::vector<runcpp2::YAML::NodePtr> roots = 
            runcpp2::YAML::ParseYAML(yamlStr, resource).DS_TRY_ACT(ssTEST_OUTPUT_ASSERT("", false));
        DEFER { FreeYAMLResource(resource); };
        
        ssTEST_OUTPUT_ASSERT("", roots.size() == 1);
        runcpp2::YAML::NodePtr root = roots.front();
        runcpp2::Data::DependencyLinkProperty dependencyLinkProperty;
        
        ssTEST_OUTPUT_EXECUTION
        (
            bool parseResult = dependencyLinkProperty.ParseYAML_Node(root);
        );
        
        ssTEST_OUTPUT_ASSERT("ParseYAML_Node should succeed", parseResult);
        
        //Verify parsed values for MSVC
        ssTEST_OUTPUT_SETUP
        (
            const auto& msvcProps = dependencyLinkProperty.ProfileProperties.at("MSVC");
        );
        ssTEST_OUTPUT_ASSERT("MSVC search lib count", msvcProps.SearchLibraryNames.size() == 2);
        ssTEST_OUTPUT_ASSERT("MSVC search dir count", msvcProps.SearchDirectories.size() == 2);
        ssTEST_OUTPUT_ASSERT("MSVC exclude lib count", msvcProps.ExcludeLibraryNames.size() == 1);
        ssTEST_OUTPUT_ASSERT("MSVC link options count", msvcProps.AdditionalLinkOptions.size() == 1);
        
        //Verify specific values
        ssTEST_OUTPUT_ASSERT("MSVC first lib", msvcProps.SearchLibraryNames.at(0) == "mylib");
        ssTEST_OUTPUT_ASSERT("MSVC exclude lib", msvcProps.ExcludeLibraryNames.at(0) == "mylib_test");
        ssTEST_OUTPUT_ASSERT(   "MSVC link option", 
                                msvcProps.AdditionalLinkOptions.at(0) == "/NODEFAULTLIB");
        
        //Verify GCC has no exclude libraries
        ssTEST_OUTPUT_SETUP
        (
            const auto& gccProps = dependencyLinkProperty.ProfileProperties.at("GCC");
        );
        ssTEST_OUTPUT_ASSERT("GCC exclude lib empty", gccProps.ExcludeLibraryNames.empty());
        ssTEST_OUTPUT_ASSERT("GCC link option", gccProps.AdditionalLinkOptions.at(0) == "-Wl,-rpath=.");
        
        //Test ToString() and Equals()
        ssTEST_OUTPUT_EXECUTION
        (
            std::string yamlOutput = dependencyLinkProperty.ToString("");
            roots = 
                runcpp2::YAML::ParseYAML(   yamlOutput, 
                                            resource).DS_TRY_ACT(ssTEST_OUTPUT_ASSERT("", false));
            ssTEST_OUTPUT_ASSERT("", roots.size() == 1);
            
            runcpp2::Data::DependencyLinkProperty parsedOutput;
            parsedOutput.ParseYAML_Node(roots.front());
        );
        
        ssTEST_OUTPUT_ASSERT(   "Parsed output should equal original", 
                                dependencyLinkProperty.Equals(parsedOutput));
    };
    
    ssTEST_END_TEST_GROUP();
    return 0;
}
