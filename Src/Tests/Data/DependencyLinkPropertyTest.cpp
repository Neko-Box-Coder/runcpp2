#include "runcpp2/Data/DependencyLinkProperty.hpp"
#include "ssTest.hpp"
#include "runcpp2/YamlLib.hpp"
#include "runcpp2/runcpp2.hpp"

int main(int argc, char** argv)
{
    runcpp2::SetLogLevel("Warning");
    
    ssTEST_INIT_TEST_GROUP();
    
    ssTEST("DependencyLinkProperty Should Parse Valid YAML")
    {
        ssTEST_OUTPUT_SETUP
        (
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
            
            ryml::Tree tree = ryml::parse_in_arena(c4::to_csubstr(yamlStr));
            ryml::ConstNodeRef root = tree.rootref();
            
            runcpp2::Data::DependencyLinkProperty dependencyLinkProperty;
        );
        
        ssTEST_OUTPUT_EXECUTION
        (
            ryml::ConstNodeRef nodeRef = root;
            bool parseResult = dependencyLinkProperty.ParseYAML_Node(nodeRef);
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
            ryml::Tree outputTree = ryml::parse_in_arena(ryml::to_csubstr(yamlOutput));
            
            runcpp2::Data::DependencyLinkProperty parsedOutput;
            nodeRef = outputTree.rootref();
            parsedOutput.ParseYAML_Node(nodeRef);
        );
        
        ssTEST_OUTPUT_ASSERT(   "Parsed output should equal original", 
                                dependencyLinkProperty.Equals(parsedOutput));
    };
    
    ssTEST_END_TEST_GROUP();
    return 0;
}
