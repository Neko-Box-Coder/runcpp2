/* runcpp2
Import: ["../TestCommon.yaml", "../../runcpp2/runcpp2Dep.yaml"]
*/

#include "runcpp2/Data/DependencyInfo.hpp"
#include "runcpp2/LibYAML_Wrapper.hpp"
#include "runcpp2/runcpp2.hpp"
#include "runcpp2/DeferUtil.hpp"
#include "ssLogger/ssLogInit.hpp"
#include "ssLogger/ssLog.hpp"

DS::Result<void> TestMain()
{
    std::unordered_map<std::string, std::string> tempParameters;
    
    //DependencyInfo Should Parse Valid YAML
    {
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
            CompileProperties:
                Defines: ["Define1", "Define2=2"]
                AdditionalCompileOptions: ["-Wno-ignored-attributes"]
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
                -   cmake ..
                -   cmake --build .
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
    
        runcpp2::YAML::ResourceHandle resource;
        std::vector<runcpp2::YAML::NodePtr> roots = runcpp2::YAML::ParseYAML(   yamlStr, 
                                                                                resource).DS_TRY();
        DEFER { FreeYAMLResource(resource); };
        
        DS_ASSERT_EQ(roots.size(), 1);
        runcpp2::YAML::NodePtr root = roots.front();
        
        runcpp2::Data::DependencyInfo dependencyInfo;
        std::unordered_map<std::string, std::vector<std::string>> substitutionMap;
        dependencyInfo.ParseYAML_Node(root, substitutionMap, tempParameters).DS_TRY();
        
        //Verify basic fields
        DS_ASSERT_EQ(dependencyInfo.Name, "MyLibrary");
        DS_ASSERT_EQ(dependencyInfo.Platforms.size(), 2);
        DS_ASSERT_EQ(   (int)dependencyInfo.LibraryType, 
                        (int)runcpp2::Data::DependencyLibraryType::SHARED);
        DS_ASSERT_EQ(dependencyInfo.IncludePaths.size(), 2);
        
        //Verify Source
        const runcpp2::Data::GitSource* git = 
            mpark::get_if<runcpp2::Data::GitSource>(&dependencyInfo.Source.Source);
        DS_ASSERT_TRUE(git != nullptr);
        DS_ASSERT_EQ(git->URL, "https://github.com/user/mylibrary.git");
        
        //Verify Link Properties
        const runcpp2::Data::ProfileLinkProperty& msvcLink = 
            dependencyInfo.LinkProperties.at("Windows").ProfileProperties.at("MSVC");
        
        DS_ASSERT_EQ(msvcLink.SearchLibraryNames.size(), 1);
        DS_ASSERT_EQ(msvcLink.ExcludeLibraryNames.at(0), "mylib_test");
        
        //Verify Commands
        DS_ASSERT_EQ(dependencyInfo.Setup.at("Windows").CommandSteps.at("MSVC").size(), 2);
        DS_ASSERT_EQ(dependencyInfo .Build
                                    .at("DefaultPlatform")
                                    .CommandSteps
                                    .at("DefaultProfile")
                                    .size(), 2);
        
        //Verify Compile Properties
        const runcpp2::Data::ProfileCompileProperty& compileProp = 
            dependencyInfo  .CompileProperties
                            .at("DefaultPlatform")
                            .ProfileProperties
                            .at("DefaultProfile");
        DS_ASSERT_EQ(compileProp.Defines.size(), 2);
        DS_ASSERT_EQ(compileProp.Defines[1].Name, "Define2");
        DS_ASSERT_EQ(compileProp.Defines[1].Value, "2");
        DS_ASSERT_TRUE(compileProp.Defines[1].HasValue);
        DS_ASSERT_EQ(compileProp.AdditionalCompileOptions.size(), 1);
        DS_ASSERT_EQ(compileProp.AdditionalCompileOptions[0], "-Wno-ignored-attributes");
        
        //Verify Files to Copy
        const std::vector<std::string>& msvcDebugFiles = 
            dependencyInfo.FilesToCopy.at("Windows").ProfileFiles.at("MSVC");
        
        DS_ASSERT_EQ(msvcDebugFiles.size(), 2);
        DS_ASSERT_EQ(msvcDebugFiles.at(0), "bin/Debug/mylib.dll");
        
        //Test ToString() and Equals()
        std::string yamlOutput = dependencyInfo.ToString("");
        roots = runcpp2::YAML::ParseYAML(yamlOutput, resource).DS_TRY();
        DS_ASSERT_EQ(roots.size(), 1);
        
        runcpp2::Data::DependencyInfo parsedOutput;
        parsedOutput.ParseYAML_Node(roots.front(), substitutionMap, tempParameters).DS_TRY();
        DS_ASSERT_TRUE(dependencyInfo.Equals(parsedOutput));
    }
    
    //DependencyInfo Should Perform Substitutions Correctly
    {
        std::unordered_map<std::string, std::string> parameters =   
        {
            {"Platforms", "Windows,Unix"}, 
            {"SourceURL", "https://github.com/user/mylibrary.git"},
            {"LibraryType", "Head"}
        };
    
        const char* yamlStr = R"(
            Name: MyLibrary
            Parameters:
                Platforms:
                    Optional: false
                    Array: true
                SourceURL:
                    Optional: false
                LibraryType:
                    Optional: false
            Variables:
                LibraryTypeVar: "{LibraryType}er"
            Platforms: [ "{Platforms}" ]
            Source:
                Git:
                    URL: "{SourceURL}"
            LibraryType: "{LibraryTypeVar}"
            IncludePaths:
            -   include
            -   src
        )";
    
        runcpp2::YAML::ResourceHandle resource;
        std::vector<runcpp2::YAML::NodePtr> roots = runcpp2::YAML::ParseYAML(   yamlStr, 
                                                                                resource).DS_TRY();
        DEFER { FreeYAMLResource(resource); };
        
        DS_ASSERT_EQ(roots.size(), 1);
        runcpp2::YAML::NodePtr root = roots.front();
        
        runcpp2::Data::DependencyInfo dependencyInfo;
        std::unordered_map<std::string, std::vector<std::string>> substitutionMap;
        dependencyInfo.ParseYAML_Node(root, substitutionMap, parameters).DS_TRY();
        
        DS_ASSERT_EQ(dependencyInfo.Platforms.size(), 2);
        DS_ASSERT_TRUE(dependencyInfo.Platforms.count("Windows") > 0);
        DS_ASSERT_TRUE(dependencyInfo.Platforms.count("Unix") > 0);

        const runcpp2::Data::GitSource* git = 
            mpark::get_if<runcpp2::Data::GitSource>(&dependencyInfo.Source.Source);
        DS_ASSERT_TRUE(git != nullptr);
        DS_ASSERT_EQ(git->URL, "https://github.com/user/mylibrary.git");
        
        DS_ASSERT_EQ(   (int)dependencyInfo.LibraryType, 
                        (int)runcpp2::Data::DependencyLibraryType::HEADER);
    
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
