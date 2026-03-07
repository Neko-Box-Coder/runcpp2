#include "runcpp2/Data/DependencySource.hpp"
#include "runcpp2/LibYAML_Wrapper.hpp"
#include "runcpp2/runcpp2.hpp"
#include "runcpp2/DeferUtil.hpp"
#include "ssLogger/ssLog.hpp"

DS::Result<void> TestMain()
{
    //DependencySource Should Parse Git Source
    {
        //NOTE: This is just a test YAML for validating parsing, don't use it for actual config
        const char* yamlStr = R"(
            Git:
                URL: https://github.com/user/repo.git
                Branch: master
                FullHistory: true
                SubmoduleInitType: Full
        )";
        
        runcpp2::YAML::ResourceHandle resource;
        std::vector<runcpp2::YAML::NodePtr> roots = runcpp2::YAML::ParseYAML(   yamlStr, 
                                                                                resource).DS_TRY();
        DEFER { FreeYAMLResource(resource); };
        
        DS_ASSERT_EQ(roots.size(), 1);
        runcpp2::YAML::NodePtr root = roots.front();
        runcpp2::Data::DependencySource dependencySource;
        DS_ASSERT_TRUE(dependencySource.ParseYAML_Node(root));
        
        
        //Verify parsed values
        const runcpp2::Data::GitSource* git = 
            mpark::get_if<runcpp2::Data::GitSource>(&dependencySource.Source);
        DS_ASSERT_TRUE(git != nullptr);
        DS_ASSERT_EQ(git->URL, "https://github.com/user/repo.git");
        
        DS_ASSERT_EQ(git->Branch, "master");
        DS_ASSERT_TRUE(git->FullHistory);
        DS_ASSERT_EQ((int)git->CurrentSubmoduleInitType, (int)runcpp2::Data::SubmoduleInitType::FULL);
        
        //Test ToString() and Equals()
        std::string yamlOutput = dependencySource.ToString("");
        roots = runcpp2::YAML::ParseYAML(yamlOutput, resource).DS_TRY();
        DS_ASSERT_EQ(roots.size(), 1);
        
        runcpp2::Data::DependencySource parsedOutput;
        parsedOutput.ParseYAML_Node(roots.front());
        DS_ASSERT_TRUE(dependencySource.Equals(parsedOutput));
    }
    
    //DependencySource Should Parse Local Source
    {
        //NOTE: This is just a test YAML for validating parsing, don't use it for actual config
        const char* yamlStr = R"(
            Local:
                Path: ../external/mylib
        )";
        
        runcpp2::YAML::ResourceHandle resource;
        std::vector<runcpp2::YAML::NodePtr> roots = runcpp2::YAML::ParseYAML(   yamlStr, 
                                                                                resource).DS_TRY();
        DEFER { FreeYAMLResource(resource); };
        DS_ASSERT_EQ(roots.size(), 1);
        runcpp2::YAML::NodePtr root = roots.front();
        runcpp2::Data::DependencySource dependencySource;
        DS_ASSERT_TRUE(dependencySource.ParseYAML_Node(root));
        
        //Verify parsed values
        const runcpp2::Data::LocalSource* local = 
            mpark::get_if<runcpp2::Data::LocalSource>(&dependencySource.Source);
        DS_ASSERT_TRUE(local != nullptr);
        DS_ASSERT_EQ(local->Path, "../external/mylib");
        DS_ASSERT_EQ((int)local->CopyMode, (int)runcpp2::Data::LocalCopyMode::Auto);
        
        //Test ToString() and Equals()
        std::string yamlOutput = dependencySource.ToString("");
        roots = runcpp2::YAML::ParseYAML(yamlOutput, resource).DS_TRY();
        DS_ASSERT_EQ(roots.size(), 1);
        
        runcpp2::Data::DependencySource parsedOutput;
        parsedOutput.ParseYAML_Node(roots.front());
        DS_ASSERT_TRUE(dependencySource.Equals(parsedOutput));
    }
    
    return {};
}

int main(int argc, char** argv)
{
    try
    {
        TestMain().DS_TRY_ACT(ssLOG_LINE(DS_TMP_ERROR.ToString()); return 1);
        return 0;
    }
    catch(std::exception& ex)
    {
        ssLOG_LINE(ex.what());
        return 1;
    }
    return 1;
}
