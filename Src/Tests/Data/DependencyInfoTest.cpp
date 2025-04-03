#include "runcpp2/Data/DependencyInfo.hpp"
#include "ssTest.hpp"
#include "runcpp2/YamlLib.hpp"
#include "runcpp2/runcpp2.hpp"

int main(int argc, char** argv)
{
    runcpp2::SetLogLevel("Warning");
    
    ssTEST_INIT_TEST_GROUP();
    
    ssTEST("DependencyInfo Should Parse Valid YAML")
    {
        ssTEST_OUTPUT_SETUP
        (
            //NOTE: This is just a test YAML for validating parsing, don't use it for actual config
            const char* yamlStr = R"(
                Name: MyLibrary
                Platforms:
                -   Windows
                -   Unix
                Source:
                    Git:
                        URL: https://github.com/user/mylibrary.git
                LibraryType: Shared
                IncludePaths:
                -   include
                -   src
                LinkProperties:
                    Windows:
                        MSVC:
                            SearchLibraryNames:
                            -   mylib
                            SearchDirectories:
                            -   lib/Debug
                            -   lib/Release
                            ExcludeLibraryNames:
                            -   mylib_test
                            AdditionalLinkOptions:
                            -   /NODEFAULTLIB
                    Unix:
                        GCC:
                            SearchLibraryNames:
                            -   mylib
                            SearchDirectories:
                            -   /usr/local/lib
                Setup:
                    Windows:
                        MSVC:
                        -   mkdir build
                        -   cd build
                    Unix:
                        GCC:
                        -   mkdir -p build
                        -   cd build
                Build:
                    Windows:
                        MSVC:
                        -   cmake ..
                        -   cmake --build .
                    Unix:
                        GCC:
                        -   cmake ..
                        -   make
                Cleanup:
                    Windows:
                        MSVC:
                        -   rmdir /s /q build
                    Unix:
                        GCC:
                        -   rm -rf build
                FilesToCopy:
                    Windows:
                        MSVC:
                        -   bin/Debug/mylib.dll
                        -   bin/Debug/mylib.pdb
                    Unix:
                        GCC:
                        -   lib/libmylib.so
            )";
            
            ryml::Tree tree = ryml::parse_in_arena(c4::to_csubstr(yamlStr));
            ryml::ConstNodeRef root = tree.rootref();
            
            runcpp2::Data::DependencyInfo dependencyInfo;
        );
        
        ssTEST_OUTPUT_EXECUTION
        (
            bool parseResult = dependencyInfo.ParseYAML_Node(root);
        );
        
        ssTEST_OUTPUT_ASSERT("ParseYAML_Node should succeed", parseResult);
        
        //Verify basic fields
        ssTEST_OUTPUT_ASSERT("Name", dependencyInfo.Name == "MyLibrary");
        ssTEST_OUTPUT_ASSERT("Platforms count", dependencyInfo.Platforms.size() == 2);
        ssTEST_OUTPUT_ASSERT(   "Library type", 
                                dependencyInfo.LibraryType == 
                                runcpp2::Data::DependencyLibraryType::SHARED);
        ssTEST_OUTPUT_ASSERT("Include paths count", dependencyInfo.IncludePaths.size() == 2);
        
        //Verify Source
        const runcpp2::Data::GitSource* git = 
            mpark::get_if<runcpp2::Data::GitSource>(&dependencyInfo.Source.Source);
        ssTEST_OUTPUT_ASSERT("Should be Git source", git != nullptr);
        ssTEST_OUTPUT_ASSERT("URL", git->URL == "https://github.com/user/mylibrary.git");
        
        //Verify Link Properties
        ssTEST_OUTPUT_SETUP
        (
            const runcpp2::Data::ProfileLinkProperty& msvcLink = 
                dependencyInfo.LinkProperties.at("Windows").ProfileProperties.at("MSVC");
        );
        
        ssTEST_OUTPUT_ASSERT("MSVC search lib count", msvcLink.SearchLibraryNames.size() == 1);
        ssTEST_OUTPUT_ASSERT("MSVC exclude lib", msvcLink.ExcludeLibraryNames.at(0) == "mylib_test");
        
        //Verify Commands
        ssTEST_OUTPUT_ASSERT(   "Setup commands count", 
                                dependencyInfo  .Setup
                                                .at("Windows")
                                                .CommandSteps
                                                .at("MSVC")
                                                .size() == 2);
        
        ssTEST_OUTPUT_ASSERT(   "Build commands count", 
                                dependencyInfo  .Build
                                                .at("Unix")
                                                .CommandSteps
                                                .at("GCC")
                                                .size() == 2);
        
        //Verify Files to Copy
        ssTEST_OUTPUT_SETUP
        (
            const std::vector<std::string>& msvcDebugFiles = 
                dependencyInfo.FilesToCopy.at("Windows").ProfileFiles.at("MSVC");
        );
        
        ssTEST_OUTPUT_ASSERT("MSVC files count", msvcDebugFiles.size() == 2);
        ssTEST_OUTPUT_ASSERT("MSVC first file", msvcDebugFiles.at(0) == "bin/Debug/mylib.dll");
        
        //Test ToString() and Equals()
        ssTEST_OUTPUT_EXECUTION
        (
            std::string yamlOutput = dependencyInfo.ToString("");
            ryml::Tree outputTree = ryml::parse_in_arena(ryml::to_csubstr(yamlOutput));
            
            runcpp2::Data::DependencyInfo parsedOutput;
            parsedOutput.ParseYAML_Node(outputTree.rootref());
        );
        
        ssTEST_OUTPUT_ASSERT(   "Parsed output should equal original", 
                                dependencyInfo.Equals(parsedOutput));
    };
    
    ssTEST_END_TEST_GROUP();
    return 0;
}
