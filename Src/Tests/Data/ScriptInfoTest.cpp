#include "runcpp2/Data/ScriptInfo.hpp"
#include "runcpp2/YamlLib.hpp"
#include "runcpp2/runcpp2.hpp"
#include "runcpp2/DeferUtil.hpp"
#include "ssLogger/ssLog.hpp"

DS::Result<void> TestMain()
{
    //ScriptInfo Should Parse Valid YAML
    {
        //NOTE: This is just a test YAML for validating parsing, don't use it for actual config
        const char* yamlStr = R"(
            Language: C++
            PassScriptPath: true
            BuildType: Static
            RequiredProfiles:
                Windows: [MSVC]
                Unix: [GCC]
            OverrideCompileFlags:
                Windows:
                    MSVC:
                        Remove: /W4
                        Append: /W3
                Unix:
                    GCC:
                        Append: -g
            OverrideLinkFlags:
                Windows:
                    MSVC:
                        Remove: /SUBSYSTEM:CONSOLE
                        Append: /SUBSYSTEM:WINDOWS
            OtherFilesToBeCompiled:
                Windows:
                    MSVC:
                    -   src/extra.cpp
                    -   src/debug.cpp
                Linux:
                    GCC:
                    -   src/extra.cpp
                    -   src/optimized.cpp
            IncludePaths:
                Windows:
                    MSVC:
                    -   include
            Defines:
                Windows:
                    MSVC:
                    -   _DEBUG
                    -   VERSION=1.0.0
                    GCC:
                    -   NDEBUG
            Setup:
                Windows:
                    MSVC:
                    -   echo 1
                    -   echo 2
                Unix:
                    GCC:
                    -   echo 3
                    -   echo 4
            PreBuild:
                Windows:
                    MSVC:
                    -   echo 2
                    -   echo 3
                Unix:
                    GCC:
                    -   echo 4
                    -   echo 5
            PostBuild:
                Windows:
                    MSVC:
                    -   echo 3
                    -   echo 4
                Unix:
                    GCC:
                    -   echo 5
                    -   echo 6
            Cleanup:
                Windows:
                    MSVC:
                    -   echo 4
                    -   echo 5
                Unix:
                    GCC:
                    -   echo 6
                    -   echo 7
            Dependencies:
            -   Name: MyLib
                Platforms:
                -   MSVC
                -   GCC
                Source:
                    Git:
                        URL: https://github.com/user/mylib.git
                LibraryType: Shared
                IncludePaths:
                -   include
                LinkProperties:
                    Windows:
                        MSVC:
                            SearchLibraryNames:
                            -   mylib
                            SearchDirectories:
                            -   lib/Debug
        )";
        
        runcpp2::YAML::ResourceHandle resource;
        std::vector<runcpp2::YAML::NodePtr> roots = runcpp2::YAML::ParseYAML(   yamlStr, 
                                                                                resource).DS_TRY();
        DEFER { FreeYAMLResource(resource); };
        
        DS_ASSERT_EQ(roots.size(), 1);
        runcpp2::YAML::NodePtr root = roots.front();
        runcpp2::Data::ScriptInfo scriptInfo;
        
        DS_ASSERT_TRUE(scriptInfo.ParseYAML_Node(root));
        
        //Verify basic fields
        DS_ASSERT_EQ(scriptInfo.Language, "C++");
        DS_ASSERT_TRUE(scriptInfo.PassScriptPath);
        DS_ASSERT_EQ((int)scriptInfo.CurrentBuildType, (int)runcpp2::Data::BuildType::STATIC);
        
        //Verify RequiredProfiles
        DS_ASSERT_EQ(scriptInfo.RequiredProfiles.at("Windows").size(), 1);
        DS_ASSERT_EQ(scriptInfo.RequiredProfiles.at("Windows").at(0), "MSVC");
        
        DS_ASSERT_EQ(scriptInfo.RequiredProfiles.at("Unix").size(), 1);
        DS_ASSERT_EQ(scriptInfo.RequiredProfiles.at("Unix").at(0), "GCC");
        
        //Verify OverrideCompileFlags
        const runcpp2::Data::FlagsOverrideInfo& msvcCompileFlags = 
            scriptInfo.OverrideCompileFlags.at("Windows").FlagsOverrides.at("MSVC");
        DS_ASSERT_EQ(msvcCompileFlags.Remove, "/W4");
        DS_ASSERT_EQ(msvcCompileFlags.Append, "/W3");
        
        //Verify OverrideLinkFlags
        const runcpp2::Data::FlagsOverrideInfo& msvcLinkFlags = 
            scriptInfo.OverrideLinkFlags.at("Windows").FlagsOverrides.at("MSVC");
        DS_ASSERT_EQ(msvcLinkFlags.Remove, "/SUBSYSTEM:CONSOLE");
        DS_ASSERT_EQ(msvcLinkFlags.Append, "/SUBSYSTEM:WINDOWS");
        
        //Verify OtherFilesToBeCompiled
        const std::vector<ghc::filesystem::path>& msvcCompileFiles = 
            scriptInfo.OtherFilesToBeCompiled.at("Windows").Paths.at("MSVC");
        DS_ASSERT_EQ(msvcCompileFiles.size(), 2);
        DS_ASSERT_EQ(msvcCompileFiles.at(0), "src/extra.cpp");
        
        
        const std::vector<ghc::filesystem::path>& gccCompileFiles = 
            scriptInfo.OtherFilesToBeCompiled.at("Linux").Paths.at("GCC");
        
        DS_ASSERT_EQ(gccCompileFiles.size(), 2);
        DS_ASSERT_EQ(gccCompileFiles.at(1), "src/optimized.cpp");
        
        //Verify IncludePaths
        const std::vector<ghc::filesystem::path>& msvcIncludeFiles = 
            scriptInfo.IncludePaths.at("Windows").Paths.at("MSVC");
        DS_ASSERT_EQ(msvcIncludeFiles.size(), 1);
        DS_ASSERT_EQ(msvcIncludeFiles.at(0), "include");

        //Verify Defines
        const std::vector<runcpp2::Data::Define>& msvcDefines = 
            scriptInfo.Defines.at("Windows").Defines.at("MSVC");
        DS_ASSERT_EQ(msvcDefines.size(), 2);
        DS_ASSERT_EQ(msvcDefines.at(0).Name, "_DEBUG");
        DS_ASSERT_EQ(msvcDefines.at(1).Name, "VERSION");
        DS_ASSERT_EQ(msvcDefines.at(1).Value, "1.0.0");
        
        //Verify Setup
        DS_ASSERT_EQ(scriptInfo.Setup.at("Windows").CommandSteps.at("MSVC").size(), 2);
        DS_ASSERT_EQ(scriptInfo.Setup.at("Unix").CommandSteps.at("GCC").size(), 2);
        
        const std::vector<std::string>& msvcSetupCommands = 
            scriptInfo.Setup.at("Windows").CommandSteps.at("MSVC");
            
        const std::vector<std::string>& gccSetupCommands = 
            scriptInfo.Setup.at("Unix").CommandSteps.at("GCC");
        
        DS_ASSERT_EQ(msvcSetupCommands.size(), 2);
        DS_ASSERT_EQ(msvcSetupCommands.at(0), "echo 1");
        
        DS_ASSERT_EQ(gccSetupCommands.size(), 2);
        DS_ASSERT_EQ(gccSetupCommands.at(0), "echo 3");
        
        //Verify PreBuild
        DS_ASSERT_EQ(scriptInfo.PreBuild.at("Windows").CommandSteps.at("MSVC").size(), 2);
        DS_ASSERT_EQ(scriptInfo.PreBuild.at("Unix").CommandSteps.at("GCC").size(), 2);
        
        const std::vector<std::string>& msvcPreBuildCommands = 
            scriptInfo.PreBuild.at("Windows").CommandSteps.at("MSVC");

        const std::vector<std::string>& gccPreBuildCommands = 
            scriptInfo.PreBuild.at("Unix").CommandSteps.at("GCC");
        
        DS_ASSERT_EQ(msvcPreBuildCommands.size(), 2);
        DS_ASSERT_EQ(msvcPreBuildCommands.at(0), "echo 2");
        
        DS_ASSERT_EQ(gccPreBuildCommands.size(), 2);
        DS_ASSERT_EQ(gccPreBuildCommands.at(1), "echo 5");

        //Verify PostBuild
        DS_ASSERT_EQ(scriptInfo.PostBuild.at("Unix").CommandSteps.at("GCC").size(), 2);

        const std::vector<std::string>& gccPostBuildCommands = 
            scriptInfo.PostBuild.at("Unix").CommandSteps.at("GCC");
        
        DS_ASSERT_EQ(gccPostBuildCommands.size(), 2);
        DS_ASSERT_EQ(gccPostBuildCommands.at(1), "echo 6");

        //Verify Cleanup
        DS_ASSERT_EQ(scriptInfo.Cleanup.at("Windows").CommandSteps.at("MSVC").size(), 2);
        
        const std::vector<std::string>& msvcCleanupCommands = 
            scriptInfo.Cleanup.at("Windows").CommandSteps.at("MSVC");

        DS_ASSERT_EQ(msvcCleanupCommands.size(), 2);
        DS_ASSERT_EQ(msvcCleanupCommands.at(0), "echo 4");

        //Verify Dependencies
        DS_ASSERT_EQ(scriptInfo.Dependencies.size(), 1);
        DS_ASSERT_EQ(scriptInfo.Dependencies.at(0).Name, "MyLib");
        DS_ASSERT_EQ(   (int)scriptInfo.Dependencies.at(0).LibraryType,
                        (int)runcpp2::Data::DependencyLibraryType::SHARED);

        //Test ToString() and Equals()
        std::string yamlOutput = scriptInfo.ToString("");
        roots = runcpp2::YAML::ParseYAML(yamlOutput, resource).DS_TRY();
        DS_ASSERT_EQ(roots.size(), 1);
        
        runcpp2::Data::ScriptInfo parsedOutput;
        parsedOutput.ParseYAML_Node(roots.front());
        DS_ASSERT_TRUE(scriptInfo.Equals(parsedOutput));
    }
    
    //ScriptInfo Should Parse SourceFiles as OtherFilesToBeCompiled
    {
        //NOTE: This is just a test YAML for validating parsing, don't use it for actual config
        const char* yamlStr = R"(
            SourceFiles:
                Windows:
                    MSVC:
                    -   src/extra.cpp
                    -   src/debug.cpp
        )";
        
        runcpp2::YAML::ResourceHandle resource;
        std::vector<runcpp2::YAML::NodePtr> roots = runcpp2::YAML::ParseYAML(   yamlStr, 
                                                                                resource).DS_TRY();
        DEFER { FreeYAMLResource(resource); };
        
        DS_ASSERT_EQ(roots.size(), 1);
        runcpp2::YAML::NodePtr root = roots.front();
        runcpp2::Data::ScriptInfo scriptInfo;
        
        DS_ASSERT_TRUE(scriptInfo.ParseYAML_Node(root));
        
        //Verify SourceFiles
        const std::vector<ghc::filesystem::path>& msvcCompileFiles = 
            scriptInfo.OtherFilesToBeCompiled.at("Windows").Paths.at("MSVC");
        
        DS_ASSERT_EQ(msvcCompileFiles.size(), 2);
        DS_ASSERT_EQ(msvcCompileFiles.at(0), "src/extra.cpp");
    }
    
    //ScriptInfo Should Parse Fields Without Platform And Profile As Default
    {
        //NOTE: This is just a test YAML for validating parsing, don't use it for actual config
        const char* yamlStr = R"(
            Language: C++
            OverrideCompileFlags:
                Remove: /W4
                Append: /W3
            OverrideLinkFlags:
                Remove: /SUBSYSTEM:CONSOLE
                Append: /SUBSYSTEM:WINDOWS
            OtherFilesToBeCompiled:
            -   src/extra.cpp
            -   src/debug.cpp
            IncludePaths:
            -   include
            -   src/include
            Defines:
            -   _DEBUG
            -   VERSION=1.0.0
            Setup:
            -   echo 1
            -   echo 2
            PreBuild:
            -   echo 3
            -   echo 4
            PostBuild:
            -   echo 5
            -   echo 6
            Cleanup:
            -   echo 7
            -   echo 8
        )";
        
        runcpp2::YAML::ResourceHandle resource;
        std::vector<runcpp2::YAML::NodePtr> roots = runcpp2::YAML::ParseYAML(   yamlStr, 
                                                                                resource).DS_TRY();
        DEFER { FreeYAMLResource(resource); };
        
        DS_ASSERT_EQ(roots.size(), 1);
        runcpp2::YAML::NodePtr root = roots.front();
        runcpp2::Data::ScriptInfo scriptInfo;
        
        DS_ASSERT_TRUE(scriptInfo.ParseYAML_Node(root));
        
        //Verify basic fields
        DS_ASSERT_EQ(scriptInfo.Language, "C++");
        
        //Verify OverrideCompileFlags with default platform and profile
        const runcpp2::Data::FlagsOverrideInfo& defaultCompileFlags = scriptInfo.OverrideCompileFlags
                                                                                .at("DefaultPlatform")
                                                                                .FlagsOverrides
                                                                                .at("DefaultProfile");
        DS_ASSERT_EQ(defaultCompileFlags.Remove, "/W4");
        DS_ASSERT_EQ(defaultCompileFlags.Append, "/W3");
        
        //Verify OverrideLinkFlags with default platform and profile
        const runcpp2::Data::FlagsOverrideInfo& defaultLinkFlags = scriptInfo   .OverrideLinkFlags
                                                                                .at("DefaultPlatform")
                                                                                .FlagsOverrides
                                                                                .at("DefaultProfile");
        DS_ASSERT_EQ(defaultLinkFlags.Remove, "/SUBSYSTEM:CONSOLE");
        DS_ASSERT_EQ(defaultLinkFlags.Append, "/SUBSYSTEM:WINDOWS");
        
        //Verify OtherFilesToBeCompiled with default platform and profile
        const std::vector<ghc::filesystem::path>& defaultCompileFiles = 
            scriptInfo.OtherFilesToBeCompiled.at("DefaultPlatform").Paths.at("DefaultProfile");
        
        DS_ASSERT_EQ(defaultCompileFiles.size(), 2);
        DS_ASSERT_EQ(defaultCompileFiles.at(0), "src/extra.cpp");
        DS_ASSERT_EQ(defaultCompileFiles.at(1), "src/debug.cpp");
        
        //Verify IncludePaths with default platform and profile
        const std::vector<ghc::filesystem::path>& defaultIncludePaths = 
            scriptInfo.IncludePaths.at("DefaultPlatform").Paths.at("DefaultProfile");
        DS_ASSERT_EQ(defaultIncludePaths.size(), 2);
        DS_ASSERT_EQ(defaultIncludePaths.at(0), "include");
        DS_ASSERT_EQ(defaultIncludePaths.at(1), "src/include");
        
        //Verify Defines with default platform and profile
        const std::vector<runcpp2::Data::Define>& defaultDefines = 
            scriptInfo.Defines.at("DefaultPlatform").Defines.at("DefaultProfile");
        
        DS_ASSERT_EQ(defaultDefines.size(), 2);
        DS_ASSERT_EQ(defaultDefines.at(0).Name, "_DEBUG");
        DS_ASSERT_EQ(defaultDefines.at(1).Name, "VERSION");
        DS_ASSERT_EQ(defaultDefines.at(1).Value, "1.0.0");
        
        //Verify Setup with default platform and profile
        const std::vector<std::string>& defaultSetupCommands = 
            scriptInfo.Setup.at("DefaultPlatform").CommandSteps.at("DefaultProfile");
        DS_ASSERT_EQ(defaultSetupCommands.size(), 2);
        DS_ASSERT_EQ(defaultSetupCommands.at(0), "echo 1");
        DS_ASSERT_EQ(defaultSetupCommands.at(1), "echo 2");
        
        //Verify PreBuild with default platform and profile
        const std::vector<std::string>& defaultPreBuildCommands = 
            scriptInfo.PreBuild.at("DefaultPlatform").CommandSteps.at("DefaultProfile");
        DS_ASSERT_EQ(defaultPreBuildCommands.size(), 2);
        DS_ASSERT_EQ(defaultPreBuildCommands.at(0), "echo 3");
        DS_ASSERT_EQ(defaultPreBuildCommands.at(1), "echo 4");
        
        //Verify PostBuild with default platform and profile
        const std::vector<std::string>& defaultPostBuildCommands = 
            scriptInfo.PostBuild.at("DefaultPlatform").CommandSteps.at("DefaultProfile");
        DS_ASSERT_EQ(defaultPostBuildCommands.size(), 2);
        DS_ASSERT_EQ(defaultPostBuildCommands.at(0), "echo 5");
        DS_ASSERT_EQ(defaultPostBuildCommands.at(1), "echo 6");
        
        //Verify Cleanup with default platform and profile
        const std::vector<std::string>& defaultCleanupCommands = 
            scriptInfo.Cleanup.at("DefaultPlatform").CommandSteps.at("DefaultProfile");
        DS_ASSERT_EQ(defaultCleanupCommands.size(), 2);
        DS_ASSERT_EQ(defaultCleanupCommands.at(0), "echo 7");
        DS_ASSERT_EQ(defaultCleanupCommands.at(1), "echo 8");
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
