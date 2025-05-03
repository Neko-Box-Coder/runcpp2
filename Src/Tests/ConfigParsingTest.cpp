#include "runcpp2/ConfigParsing.hpp"

#include "ssTest.hpp"
#include "CppOverride.hpp"
#include "ssLogger/ssLog.hpp"

CO_DECLARE_INSTANCE(OverrideInstance);

#define INTERNAL_RUNCPP2_UNDEF_MOCKS 1
#include "Tests/ConfigParsing/MockComponents.hpp"

#include <memory>

#if !INTERNAL_RUNCPP2_UNIT_TESTS || !defined(INTERNAL_RUNCPP2_UNIT_TESTS)
    static_assert(false, "INTERNAL_RUNCPP2_UNIT_TESTS not defined");
#endif


//Mock DefaultUserConfig for testing
extern "C" const uint8_t DefaultUserConfig[] = {0};
extern "C" const size_t DefaultUserConfig_size = 0;
extern "C" const uint8_t CommonFileTypes[] = {0};
extern "C" const size_t CommonFileTypes_size = 0;
extern "C" const uint8_t G_PlusPlus[] = {0};
extern "C" const size_t G_PlusPlus_size = 0;
extern "C" const uint8_t Vs2022_v17Plus[] = {0};
extern "C" const size_t Vs2022_v17Plus_size = 0;


int main(int argc, char** argv)
{
    ssTEST_INIT_TEST_GROUP();
    
    ssTEST_PARSE_ARGS(argc, argv);
    
    using namespace CppOverride;
    
    std::string configPath;
    std::shared_ptr<OverrideResult> existsResult;
    std::shared_ptr<OverrideResult> isDirResult;
    std::shared_ptr<OverrideResult> ifstreamResult;
    std::shared_ptr<OverrideResult> ifstreamSucceedResult;
    std::shared_ptr<OverrideResult> writeDefaultConfigResult;
    std::shared_ptr<OverrideResult> rdbufResult;
    void* userConfigIfstreamInstance = nullptr;
    
    ssTEST_COMMON_SETUP
    {
        CO_CLEAR_ALL_OVERRIDE_SETUP(OverrideInstance);
        ssLOG_SET_CURRENT_THREAD_TARGET_LEVEL(ssLOG_LEVEL_WARNING);
        
        configPath = "some/config/dir/UserConfig.yaml";
        existsResult = 
            CO_SETUP_OVERRIDE   (OverrideInstance, Mock_exists)
                                .WhenCalledWith<const std::string&, CO_ANY_TYPE>(configPath, CO_ANY)
                                .Returns<bool>(true)
                                .ReturnsResult();
        isDirResult = 
            CO_SETUP_OVERRIDE   (OverrideInstance, Mock_is_directory)
                                .WhenCalledWith<const std::string&, CO_ANY_TYPE>(configPath, CO_ANY)
                                .Returns<bool>(false)
                                .ReturnsResult();
        ifstreamResult = 
            CO_SETUP_OVERRIDE   (OverrideInstance, Mock_ifstream)
                                .WhenCalledWith<const ghc::filesystem::path&>(configPath)
                                .WhenCalledExpectedly_Do
                                (
                                    [&userConfigIfstreamInstance]
                                    (void* instance, const std::vector<void*>&)
                                    {
                                        userConfigIfstreamInstance = instance;
                                    }
                                )
                                .ReturnsResult();
        ifstreamSucceedResult = 
            CO_SETUP_OVERRIDE   (OverrideInstance, operator!)
                                .If
                                (
                                    [&userConfigIfstreamInstance]
                                    (void* instance, const std::vector<void*>&) -> bool
                                    {
                                        return instance == userConfigIfstreamInstance;
                                    }
                                )
                                .Returns<bool>(false)
                                .ReturnsResult();
        writeDefaultConfigResult = 
            CO_SETUP_OVERRIDE   (OverrideInstance, WriteDefaultConfig)
                                .Returns<bool>(false)
                                .ReturnsResult();
    };
    
    auto commonOverrideStatusCheck = [&]()
    {
        ssTEST_OUTPUT_ASSERT("", existsResult->GetSucceedCount(), 1);
        ssTEST_OUTPUT_ASSERT("", isDirResult->GetSucceedCount(), 1);
        ssTEST_OUTPUT_ASSERT("", ifstreamResult->GetSucceedCount(), 1);
        ssTEST_OUTPUT_ASSERT("", ifstreamSucceedResult->GetSucceedCount(), 1);
        ssTEST_OUTPUT_ASSERT("", rdbufResult->GetSucceedCount(), 1);
        ssTEST_OUTPUT_ASSERT(   "WriteDefaultConfig() should not be called", 
                                writeDefaultConfigResult->GetStatusCount(), 0);
    };
    
    auto commonDefaultOverrideSetup = [&]()
    {
        CO_SETUP_OVERRIDE(OverrideInstance, Mock_exists).Returns<bool>(false);
        CO_SETUP_OVERRIDE(OverrideInstance, Mock_is_directory).Returns<bool>(false);
        CO_SETUP_OVERRIDE(OverrideInstance, operator!).Returns<bool>(false);
        CO_SETUP_OVERRIDE(OverrideInstance, WriteDefaultConfig).Returns<bool>(false);
        CO_SETUP_OVERRIDE(OverrideInstance, rdbuf).Returns<std::string>("");
    };
    
    ssTEST_COMMON_CLEANUP
    {
        ssLOG_SET_CURRENT_THREAD_TARGET_LEVEL(ssLOG_LEVEL_WARNING);
        configPath.clear();
        existsResult = CreateOverrideResult();
        isDirResult = CreateOverrideResult();
        ifstreamResult = CreateOverrideResult();
        ifstreamSucceedResult = CreateOverrideResult();
        writeDefaultConfigResult = CreateOverrideResult();
        rdbufResult = CreateOverrideResult();
        userConfigIfstreamInstance = nullptr;
    };

    ssTEST("ReadUserConfig Should Parse PreferredProfile As String Correctly")
    {
        ssTEST_OUTPUT_SETUP
        (
            const char* yamlStr = R"(
                PreferredProfile: "g++"
                Profiles:
                -   &ProfileTemplate
                    Name: "g++"
                    FileExtensions: [.cpp, .cc, .cxx]
                    Languages: ["c++"]
                    FilesTypes: 
                        ObjectLinkFile: &PrefixExtensionTemplate
                            Prefix:
                                DefaultPlatform: "prefix"
                            Extension:
                                DefaultPlatform: ".extension"
                        SharedLinkFile: *PrefixExtensionTemplate
                        SharedLibraryFile: *PrefixExtensionTemplate
                        StaticLinkFile: *PrefixExtensionTemplate
                        DebugSymbolFile: *PrefixExtensionTemplate
                    Compiler:
                        CheckExistence: 
                            DefaultPlatform: "g++ -v"
                        CompileTypes: &TypeInfoEntries
                            Executable: &DefaultTypeInfo
                                DefaultPlatform:
                                    Flags: "Flags"
                                    Executable: "g++"
                                    RunParts: []
                                    ExpectedOutputFiles: []
                            ExecutableShared: *DefaultTypeInfo
                            Static: *DefaultTypeInfo
                            Shared: *DefaultTypeInfo
                    Linker:
                        CheckExistence: 
                            DefaultPlatform: "g++ -v"
                        LinkTypes: *TypeInfoEntries
                -   <<: *ProfileTemplate
                    Name: "g++2"
                -   <<: *ProfileTemplate
                    Name: "g++3"
            )";
            
            rdbufResult = CO_SETUP_OVERRIDE (OverrideInstance, rdbuf)
                                            .Returns<std::string>(yamlStr)
                                            .Times(1)
                                            .ReturnsResult();
            commonDefaultOverrideSetup();
            
            std::vector<runcpp2::Data::Profile> profiles;
            std::string preferredProfile;
        );
        
        ssTEST_OUTPUT_EXECUTION
        (
            bool parseResult = runcpp2::ReadUserConfig(profiles, preferredProfile, configPath);
        );
        
        commonOverrideStatusCheck();
        ssTEST_OUTPUT_ASSERT("ReadUserConfig should succeed", parseResult);
        ssTEST_OUTPUT_ASSERT("Should parse 3 profiles", profiles.size() == 3);
        ssTEST_OUTPUT_ASSERT(preferredProfile == "g++");
    };

    ssTEST("ReadUserConfig Should Parse PreferredProfile As Platform Map Correctly")
    {
        ssTEST_OUTPUT_SETUP
        (
            const char* yamlStr = R"(
                PreferredProfile: 
                    MacOS: "g++3"
                    DefaultPlatform: "g++"
                Profiles:
                -   &ProfileTemplate
                    Name: "g++"
                    FileExtensions: [.cpp, .cc, .cxx]
                    Languages: ["c++"]
                    FilesTypes: 
                        ObjectLinkFile: &PrefixExtensionTemplate
                            Prefix:
                                DefaultPlatform: "prefix"
                            Extension:
                                DefaultPlatform: ".extension"
                        SharedLinkFile: *PrefixExtensionTemplate
                        SharedLibraryFile: *PrefixExtensionTemplate
                        StaticLinkFile: *PrefixExtensionTemplate
                        DebugSymbolFile: *PrefixExtensionTemplate
                    Compiler:
                        CheckExistence: 
                            DefaultPlatform: "g++ -v"
                        CompileTypes: &TypeInfoEntries
                            Executable: &DefaultTypeInfo
                                DefaultPlatform:
                                    Flags: "Flags"
                                    Executable: "g++"
                                    RunParts: []
                                    ExpectedOutputFiles: []
                            ExecutableShared: *DefaultTypeInfo
                            Static: *DefaultTypeInfo
                            Shared: *DefaultTypeInfo
                    Linker:
                        CheckExistence: 
                            DefaultPlatform: "g++ -v"
                        LinkTypes: *TypeInfoEntries
                -   <<: *ProfileTemplate
                    Name: "g++2"
                -   <<: *ProfileTemplate
                    Name: "g++3"
            )";
            
            rdbufResult = CO_SETUP_OVERRIDE (OverrideInstance, rdbuf)
                                            .Returns<std::string>(yamlStr)
                                            .Times(1)
                                            .ReturnsResult();
            std::shared_ptr<OverrideResult> getPlatformNamesResult = 
                CO_SETUP_OVERRIDE   (OverrideInstance, GetPlatformNames)
                                    .Returns<std::vector<std::string>>({"MacOS", 
                                                                        "Unix", 
                                                                        "DefaultPlatform"})
                                    .Times(1)
                                    .ReturnsResult();
            commonDefaultOverrideSetup();
            
            std::vector<runcpp2::Data::Profile> profiles;
            std::string preferredProfile;
        );
        
        ssTEST_OUTPUT_EXECUTION
        (
            bool parseResult = runcpp2::ReadUserConfig(profiles, preferredProfile, configPath);
        );
        
        commonOverrideStatusCheck();
        ssTEST_OUTPUT_ASSERT(getPlatformNamesResult->LastStatusSucceed());
        ssTEST_OUTPUT_ASSERT("ReadUserConfig should succeed", parseResult);
        ssTEST_OUTPUT_ASSERT("Should parse 3 profiles", profiles.size() == 3);
        ssTEST_OUTPUT_ASSERT("MacOS PreferredProfile should be g++3", preferredProfile == "g++3");
        
        ssTEST_OUTPUT_SETUP
        (
            ssTEST_CALL_COMMON_CLEANUP();
            ssTEST_CALL_COMMON_SETUP();
            profiles.clear();
            preferredProfile.clear();
            
            CO_SETUP_OVERRIDE   (OverrideInstance, rdbuf)
                                .Returns<std::string>(yamlStr)
                                .Times(1)
                                .AssignsResult(rdbufResult);
            CO_SETUP_OVERRIDE   (OverrideInstance, GetPlatformNames)
                                .Returns<std::vector<std::string>>({"Linux", 
                                                                    "Unix", 
                                                                    "DefaultPlatform"})
                                .Times(1)
                                .AssignsResult(getPlatformNamesResult);
            commonDefaultOverrideSetup();
        );
        
        ssTEST_OUTPUT_EXECUTION
        (
            parseResult = runcpp2::ReadUserConfig(profiles, preferredProfile, configPath);
        );
        
        ssTEST_OUTPUT_ASSERT(getPlatformNamesResult->LastStatusSucceed());
        ssTEST_OUTPUT_ASSERT("ReadUserConfig should succeed", parseResult);
        ssTEST_OUTPUT_ASSERT("Should parse 3 profiles", profiles.size() == 3);
        ssTEST_OUTPUT_ASSERT(   "DefaultProfile PreferredProfile should be g++", 
                                preferredProfile == "g++");
    };

    ssTEST("ReadUserConfig Should Import Profile From External YAML File")
    {
        ssTEST_OUTPUT_SETUP
        (
            const char* mainConfigYamlStr = R"(
                PreferredProfile: "imported-profile"
                Profiles:
                -   Import: "Default/g++.yaml"
                -   Name: "second-profile"
                    FileExtensions: [.cpp, .cc, .cxx]
                    Languages: ["c++"]
                    FilesTypes: 
                        ObjectLinkFile: &PrefixExtensionTemplate
                            Prefix:
                                DefaultPlatform: "prefix"
                            Extension:
                                DefaultPlatform: ".extension"
                        SharedLinkFile: *PrefixExtensionTemplate
                        SharedLibraryFile: *PrefixExtensionTemplate
                        StaticLinkFile: *PrefixExtensionTemplate
                        DebugSymbolFile: *PrefixExtensionTemplate
                    Compiler:
                        CheckExistence: 
                            DefaultPlatform: "g++ -v"
                        CompileTypes: &TypeInfoEntries
                            Executable: &DefaultTypeInfo
                                DefaultPlatform:
                                    Flags: "Flags"
                                    Executable: "g++"
                                    RunParts: []
                                    ExpectedOutputFiles: []
                            ExecutableShared: *DefaultTypeInfo
                            Static: *DefaultTypeInfo
                            Shared: *DefaultTypeInfo
                    Linker:
                        CheckExistence: 
                            DefaultPlatform: "g++ -v"
                        LinkTypes: *TypeInfoEntries
            )";

            const char* importedProfileYamlStr = R"(
                Name: "imported-profile"
                NameAliases: ["gcc"]
                FileExtensions: [.cpp, .cc]
                Languages: ["c++"]
                Import: "filetypes.yaml"
                Compiler:
                    CheckExistence: 
                        DefaultPlatform: "imported-gcc -v"
                    CompileTypes: &TypeInfoEntries
                        Executable: &DefaultTypeInfo
                            DefaultPlatform:
                                Flags: "Imported-Flags"
                                Executable: "imported-gcc"
                                RunParts: []
                                ExpectedOutputFiles: []
                        ExecutableShared: *DefaultTypeInfo
                        Static: *DefaultTypeInfo
                        Shared: *DefaultTypeInfo
                Linker:
                    CheckExistence: 
                        DefaultPlatform: "imported-gcc -v"
                    LinkTypes: *TypeInfoEntries
            )";
            
            const char* importedFiletypesYamlStr = R"(
                FilesTypes: 
                    ObjectLinkFile: &PrefixExtensionTemplate
                        Prefix:
                            DefaultPlatform: "file-prefix"
                        Extension:
                            DefaultPlatform: ".file-ext"
                    SharedLinkFile: *PrefixExtensionTemplate
                    SharedLibraryFile: *PrefixExtensionTemplate
                    StaticLinkFile: *PrefixExtensionTemplate
                    DebugSymbolFile: *PrefixExtensionTemplate
            )";
            
            //Mock for main config file 
            rdbufResult = CO_SETUP_OVERRIDE (OverrideInstance, rdbuf)
                                            .Returns<std::string>(mainConfigYamlStr)
                                            .Times(1)
                                            .If
                                            (
                                                [&userConfigIfstreamInstance]
                                                (void* instance, const std::vector<void*>&) -> bool
                                                {
                                                    return instance == userConfigIfstreamInstance;
                                                }
                                            )
                                            .ReturnsResult();
            
            //Setup filesystem exist for the import yaml files
            std::string firstExpectedImportPath = "some/config/dir/Default/g++.yaml";
            std::string secondExpectedImportPath = "some/config/dir/Default/filetypes.yaml";
            
            std::shared_ptr<OverrideResult> firstImportFileExistResult = 
                CO_SETUP_OVERRIDE   (OverrideInstance, Mock_exists)
                                    .WhenCalledWith<const std::string&, 
                                                    CO_ANY_TYPE>(   firstExpectedImportPath, 
                                                                    CO_ANY)
                                    .Returns<bool>(true)
                                    .ReturnsResult();
            std::shared_ptr<OverrideResult> secondImportFileExistResult = 
                CO_SETUP_OVERRIDE   (OverrideInstance, Mock_exists)
                                    .WhenCalledWith<const std::string&, 
                                                    CO_ANY_TYPE>(   secondExpectedImportPath, 
                                                                    CO_ANY)
                                    .Returns<bool>(true)
                                    .ReturnsResult();
            
            //Record if there's an ifstream created with import paths
            void* firstImportIfstreamInstance = nullptr;
            void* secondImportIfstreamInstance = nullptr;
            std::shared_ptr<OverrideResult> firstImportedFileStreamResult = 
                CO_SETUP_OVERRIDE   (OverrideInstance, Mock_ifstream)
                                    .WhenCalledWith<const ghc::filesystem::path>
                                        (firstExpectedImportPath)
                                    .Times(1)
                                    .WhenCalledExpectedly_Do
                                    (
                                        [&firstImportIfstreamInstance]
                                        (void* instance, const std::vector<void*>&) 
                                        {
                                            firstImportIfstreamInstance = instance;
                                        }
                                    )
                                    .ReturnsResult();
            std::shared_ptr<OverrideResult> secondImportedFileStreamResult = 
                CO_SETUP_OVERRIDE   (OverrideInstance, Mock_ifstream)
                                    .WhenCalledWith<const ghc::filesystem::path>
                                        (secondExpectedImportPath)
                                    .Times(1)
                                    .WhenCalledExpectedly_Do
                                    (
                                        [&secondImportIfstreamInstance]
                                        (void* instance, const std::vector<void*>&) 
                                        {
                                            secondImportIfstreamInstance = instance;
                                        }
                                    )
                                    .ReturnsResult();
            
            //Mock for rdbuf call with specific path check for import files
            std::shared_ptr<OverrideResult> firstImportedFileRdbufResult = 
                CO_SETUP_OVERRIDE   (OverrideInstance, rdbuf)
                                    .Returns<std::string>(importedProfileYamlStr)
                                    .Times(1)
                                    .If
                                    (
                                        [&firstImportIfstreamInstance]
                                        (void* instance, const std::vector<void*>&) -> bool
                                        {
                                            return instance == firstImportIfstreamInstance;
                                        }
                                    )
                                    .ReturnsResult();
            std::shared_ptr<OverrideResult> secondImportedFileRdbufResult = 
                CO_SETUP_OVERRIDE   (OverrideInstance, rdbuf)
                                    .Returns<std::string>(importedFiletypesYamlStr)
                                    .Times(1)
                                    .If
                                    (
                                        [&secondImportIfstreamInstance]
                                        (void* instance, const std::vector<void*>&) -> bool
                                        {
                                            return instance == secondImportIfstreamInstance;
                                        }
                                    )
                                    .ReturnsResult();
            commonDefaultOverrideSetup();
            
            std::vector<runcpp2::Data::Profile> profiles;
            std::string preferredProfile;
        );
        
        ssTEST_OUTPUT_EXECUTION
        (
            bool parseResult = runcpp2::ReadUserConfig(profiles, preferredProfile, configPath);
        );
        
        commonOverrideStatusCheck();
        ssTEST_OUTPUT_ASSERT("", firstImportFileExistResult->GetSucceedCount(), 1);
        ssTEST_OUTPUT_ASSERT("", secondImportFileExistResult->GetSucceedCount(), 1);
        
        ssTEST_OUTPUT_ASSERT("", firstImportedFileStreamResult->GetSucceedCount(), 1);
        ssTEST_OUTPUT_ASSERT("", secondImportedFileStreamResult->GetSucceedCount(), 1);
        
        ssTEST_OUTPUT_ASSERT("", firstImportedFileRdbufResult->GetSucceedCount(), 1);
        ssTEST_OUTPUT_ASSERT("", secondImportedFileRdbufResult->GetSucceedCount(), 1);
        
        ssTEST_OUTPUT_ASSERT("ReadUserConfig should succeed", parseResult);
        ssTEST_OUTPUT_ASSERT("Should parse 2 profiles (1 imported + 1 inline)", profiles.size(), 2);
        if(!profiles.empty())
        {
            ssTEST_OUTPUT_ASSERT(   "First profile should be the imported one", 
                                    profiles[0].Name, 
                                    "imported-profile");
            ssTEST_OUTPUT_ASSERT(   "First profile should have the imported executable", 
                                    profiles[0] .Compiler
                                                .OutputTypes
                                                .Executable.at("DefaultPlatform")
                                                .Executable, 
                                    "imported-gcc");
            ssTEST_OUTPUT_ASSERT(   "First profile should have nested imported filetype", 
                                    profiles[0].FilesTypes.SharedLinkFile.Prefix.at("DefaultPlatform"),
                                    "file-prefix");
            ssTEST_OUTPUT_ASSERT(   "First profile should have correct name aliases", 
                                    profiles[0].NameAliases.size() == 1 && 
                                    profiles[0].NameAliases.find("gcc") != 
                                        profiles[0].NameAliases.end());
            ssTEST_OUTPUT_ASSERT(   "PreferredProfile should match the imported profile", 
                                    preferredProfile, 
                                    "imported-profile");
        }
    };

    ssTEST_END_TEST_GROUP();

    return 0;
} 
