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
                    Branch: master
                    FullHistory: true
                    SubmoduleInitType: Full
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
        ssTEST_OUTPUT_ASSERT("URL", git->URL, "https://github.com/user/repo.git");
        ssTEST_OUTPUT_ASSERT("Branch", git->Branch, "master");
        ssTEST_OUTPUT_ASSERT("FullHistory", git->FullHistory, true);
        ssTEST_OUTPUT_ASSERT(   "SubmoduleInitType", 
                                git->CurrentSubmoduleInitType == 
                                runcpp2::Data::SubmoduleInitType::FULL);
        
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
        ssTEST_OUTPUT_ASSERT(   "Default CopyMode", 
                                local->CopyMode == runcpp2::Data::LocalCopyMode::Auto);
        
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
    
    ssTEST("DependencySource Should Parse Local Source With CopyMode")
    {
        const std::vector<std::pair<std::string, 
                                    runcpp2::Data::LocalCopyMode>> testCases = 
        {
            {"Auto", runcpp2::Data::LocalCopyMode::Auto},
            {"Symlink", runcpp2::Data::LocalCopyMode::Symlink},
            {"Hardlink", runcpp2::Data::LocalCopyMode::Hardlink},
            {"Copy", runcpp2::Data::LocalCopyMode::Copy}
        };

        for(const std::pair<std::string, 
                            runcpp2::Data::LocalCopyMode>& testCase : testCases)
        {
            ssTEST_OUTPUT("Test Copy Mode: " << testCase.first);
            ssTEST_OUTPUT_SETUP
            (
                std::string yamlStr = R"(
                    Local:
                        Path: ../external/mylib
                        CopyMode: )" + testCase.first;
                
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
            
            const runcpp2::Data::LocalSource* local = 
                mpark::get_if<runcpp2::Data::LocalSource>(&dependencySource.Source);
            ssTEST_OUTPUT_ASSERT("Should be Local source", local != nullptr);

            if(!local)
                return;

            ssTEST_OUTPUT_ASSERT("Path", local->Path, "../external/mylib");
            ssTEST_OUTPUT_ASSERT("CopyMode", local->CopyMode == testCase.second);

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
        }
    };

    ssTEST("DependencySource Should Handle Invalid CopyMode")
    {
        ssTEST_OUTPUT_SETUP
        (
            const char* yamlStr = R"(
                Local:
                    Path: ../external/mylib
                    CopyMode: InvalidMode
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
        
        ssTEST_OUTPUT_ASSERT("ParseYAML_Node should fail", parseResult, false);
    };

    ssTEST("DependencySource Should Handle Invalid FullHistory Option")
    {
        ssTEST_OUTPUT_SETUP
        (
            const char* yamlStr = R"(
                Git:
                    URL: https://github.com/user/repo.git
                    FullHistory: What
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
        
        ssTEST_OUTPUT_ASSERT("ParseYAML_Node should fail", parseResult, false);
    };

    ssTEST("DependencySource Should Handle Invalid SubmoduleInitType Option")
    {
        ssTEST_OUTPUT_SETUP
        (
            const char* yamlStr = R"(
                Git:
                    URL: https://github.com/user/repo.git
                    SubmoduleInitType: What
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
        
        ssTEST_OUTPUT_ASSERT("ParseYAML_Node should fail", parseResult, false);
    };
    
    ssTEST_END_TEST_GROUP();
    return 0;
}
