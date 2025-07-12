#include "runcpp2/ConfigParsing.hpp"

#include "ssTest.hpp"
#include "CppOverride.hpp"
#include "ssLogger/ssLog.hpp"
#include "MacroPowerToys.h"

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
    
    const std::string configPath = "some/config/dir/UserConfig.yaml";
    const std::string versionPath = "some/config/dir/.version";
    #define STR(x) #x
        const std::string versionString = MPT_COMPOSE(STR, (RUNCPP2_CONFIG_VERSION));
    #undef STR
    std::shared_ptr<OverrideResult> configExistsResult;
    std::shared_ptr<OverrideResult> versionExistsResult;
    std::shared_ptr<OverrideResult> isDirResult;
    std::shared_ptr<OverrideResult> configIfstreamResult;
    std::shared_ptr<OverrideResult> versionIfstreamResult;
    
    std::shared_ptr<OverrideResult> ifstreamSucceedResult;
    std::shared_ptr<OverrideResult> configRdbufResult;
    std::shared_ptr<OverrideResult> versionRdbufResult;
    std::shared_ptr<OverrideResult> writeDefaultConfigsResult;
    void* userConfigIfstreamInstance = nullptr;
    void* versionIfstreamInstance = nullptr;
    
    ssTEST_COMMON_SETUP
    {
        CO_CLEAR_ALL_OVERRIDE_SETUP(OverrideInstance);
        ssLOG_SET_CURRENT_THREAD_TARGET_LEVEL(ssLOG_LEVEL_WARNING);
        
        configExistsResult = nullptr;
        CO_INSERT_REF   (OverrideInstance, ghc::filesystem, Mock_exists)
                        .WhenCalledWith<const std::string&, CO_ANY_TYPE>(configPath, CO_ANY)
                        .Returns<bool>(true)
                        .AssignsResult(configExistsResult);
        versionExistsResult = nullptr;
        CO_INSERT_REF   (OverrideInstance, ghc::filesystem, Mock_exists)
                            .WhenCalledWith<const std::string&, CO_ANY_TYPE>(versionPath, CO_ANY)
                            .Returns<bool>(true)
                            .AssignsResult(versionExistsResult);
        isDirResult = nullptr
        CO_INSERT_REF   (OverrideInstance, ghc::filesystem, Mock_is_directory)
                        .WhenCalledWith<const std::string&, CO_ANY_TYPE>(configPath, CO_ANY)
                        .Returns<bool>(false)
                        .AssignsResult(isDirResult);
        configIfstreamResult = 
            CO_INSERT_NO_REF(OverrideInstance, Mock_ifstream)
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
        versionIfstreamResult = 
            CO_INSERT_NO_REF   (OverrideInstance, Mock_ifstream)
                                .WhenCalledWith<const ghc::filesystem::path&>(versionPath)
                                .WhenCalledExpectedly_Do
                                (
                                    [&versionIfstreamInstance]
                                    (void* instance, const std::vector<void*>&)
                                    {
                                        versionIfstreamInstance = instance;
                                    }
                                )
                                .ReturnsResult();
        ifstreamSucceedResult = 
            CO_INSERT_REF   (OverrideInstance, std::Mock_ifstream, operator!)
                                .If
                                (
                                    [&userConfigIfstreamInstance, &versionIfstreamInstance]
                                    (void* instance, const std::vector<void*>&) -> bool
                                    {
                                        return  instance == userConfigIfstreamInstance ||
                                                instance == versionIfstreamInstance;
                                    }
                                )
                                .Returns<bool>(false)
                                .ReturnsResult();
        versionRdbufResult = 
            CO_INSERT_REF   (OverrideInstance, std::Mock_ifstream, rdbuf)
                                .Returns<std::string>(versionString)
                                .If
                                (
                                    [&versionIfstreamInstance]
                                    (void* instance, const std::vector<void*>&) -> bool
                                    {
                                        return instance == versionIfstreamInstance;
                                    }
                                )
                                .Times(1)
                                .ReturnsResult();
        writeDefaultConfigsResult = 
            CO_INSERT_REF   (OverrideInstance, WriteDefaultConfigs)
                                .Returns<bool>(false)
                                .ReturnsResult();
    };
    
    auto commonOverrideStatusCheck = [&]()
    {
        ssTEST_OUTPUT_ASSERT("", configExistsResult->GetSucceedCount(), 1);
        ssTEST_OUTPUT_ASSERT("", versionExistsResult->GetSucceedCount(), 1);
        ssTEST_OUTPUT_ASSERT("", isDirResult->GetSucceedCount(), 1);
        ssTEST_OUTPUT_ASSERT("", configIfstreamResult->GetSucceedCount(), 1);
        ssTEST_OUTPUT_ASSERT("", versionIfstreamResult->GetSucceedCount(), 1);
        ssTEST_OUTPUT_ASSERT("", ifstreamSucceedResult->GetSucceedCount(), 2);
        ssTEST_OUTPUT_ASSERT("", configRdbufResult->GetSucceedCount(), 1);
        ssTEST_OUTPUT_ASSERT("", versionRdbufResult->GetSucceedCount(), 1);
        ssTEST_OUTPUT_ASSERT(   "WriteDefaultConfigs() should not be called", 
                                writeDefaultConfigsResult->GetStatusCount(), 0);
    };
    
    auto commonDefaultOverrideSetup = [&]()
    {
        CO_INSERT_REF(OverrideInstance, ghc::filesystem, Mock_exists).Returns<bool>(false);
        CO_INSERT_REF(OverrideInstance, ghc::filesystem, Mock_is_directory).Returns<bool>(false);
        CO_INSERT_REF(OverrideInstance, operator!).Returns<bool>(false);
        CO_INSERT_REF(OverrideInstance, runcpp2, WriteDefaultConfigs).Returns<bool>(false);
        CO_INSERT_REF(OverrideInstance, std::Mock_ifstream, rdbuf).Returns<std::string>("");
    };
    
    ssTEST_COMMON_CLEANUP
    {
        ssLOG_SET_CURRENT_THREAD_TARGET_LEVEL(ssLOG_LEVEL_WARNING);
        configExistsResult = CreateOverrideResult();
        versionExistsResult = CreateOverrideResult();
        isDirResult = CreateOverrideResult();
        configIfstreamResult = CreateOverrideResult();
        ifstreamSucceedResult = CreateOverrideResult();
        configIfstreamResult = CreateOverrideResult();
        versionIfstreamResult = CreateOverrideResult();
        configRdbufResult = CreateOverrideResult();
        versionRdbufResult = CreateOverrideResult();
        writeDefaultConfigsResult = CreateOverrideResult();
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
            
            configRdbufResult = 
                CO_INSERT_REF   (OverrideInstance, std::Mock_ifstream, rdbuf)
                                    .Returns<std::string>(yamlStr)
                                    .If
                                    (
                                        [&userConfigIfstreamInstance]
                                        (void* instance, const std::vector<void*>&) -> bool
                                        {
                                            return instance == userConfigIfstreamInstance;
                                        }
                                    )
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
            
            configRdbufResult = 
                CO_INSERT_REF   (OverrideInstance, std::Mock_ifstream, rdbuf)
                                    .Returns<std::string>(yamlStr)
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
            std::shared_ptr<OverrideResult> getPlatformNamesResult = 
                CO_INSERT_REF   (OverrideInstance, runcpp2, GetPlatformNames)
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
            
            CO_INSERT_REF   (OverrideInstance, std::Mock_ifstream, rdbuf)
                                .Returns<std::string>(yamlStr)
                                .Times(1)
                                .If
                                (
                                    [&userConfigIfstreamInstance]
                                    (void* instance, const std::vector<void*>&) -> bool
                                    {
                                        return instance == userConfigIfstreamInstance;
                                    }
                                )
                                .AssignsResult(configRdbufResult);
            CO_INSERT_REF   (OverrideInstance, runcpp2, GetPlatformNames)
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
                -   Import: ["Default/gcc.yaml", "Default/gccCompilerLinker.yaml"]
                
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

            const char* importedGccProfileYamlStr = R"(
                Name: "imported-profile"
                NameAliases: ["g++"]
                FileExtensions: [.cpp, .cc]
                Languages: ["c++"]
                Import: "filetypes.yaml"
            )";
            
            const char* importedCompilerLinkerYamlStr = R"(
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
            configRdbufResult = 
                CO_INSERT_REF   (OverrideInstance, std::Mock_ifstream, rdbuf)
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
            std::string gccExpectedImportPath = "some/config/dir/Default/gcc.yaml";
            std::string filetypesExpectedImportPath = "some/config/dir/Default/filetypes.yaml";
            std::string gccCompilerLinkerExpectedImportPath = 
                "some/config/dir/Default/gccCompilerLinker.yaml";
            
            std::shared_ptr<OverrideResult> gccImportFileExistResult = 
                CO_INSERT_REF   (OverrideInstance, ghc::filesystem, Mock_exists)
                                    .WhenCalledWith<const std::string&, 
                                                    CO_ANY_TYPE>(   gccExpectedImportPath, 
                                                                    CO_ANY)
                                    .Returns<bool>(true)
                                    .ReturnsResult();
            std::shared_ptr<OverrideResult> filetypesImportFileExistResult = 
                CO_INSERT_REF   (OverrideInstance, ghc::filesystem, Mock_exists)
                                    .WhenCalledWith<const std::string&, 
                                                    CO_ANY_TYPE>(   filetypesExpectedImportPath, 
                                                                    CO_ANY)
                                    .Returns<bool>(true)
                                    .ReturnsResult();
            std::shared_ptr<OverrideResult> gccCompilerLinkerImportFileExistResult = 
                CO_INSERT_REF   (OverrideInstance, ghc::filesystem, Mock_exists)
                                    .WhenCalledWith
                                    <
                                        const std::string&, 
                                        CO_ANY_TYPE
                                    >
                                    (
                                        gccCompilerLinkerExpectedImportPath, 
                                        CO_ANY
                                    )
                                    .Returns<bool>(true)
                                    .ReturnsResult();
            
            //Record if there's an ifstream created with import paths
            void* gccImportIfstreamInstance = nullptr;
            void* filetypesImportIfstreamInstance = nullptr;
            void* gccCompilerLinkerImportIfstreamInstance = nullptr;
            std::shared_ptr<OverrideResult> gccImportedFileStreamResult = 
                CO_INSERT_NO_REF(OverrideInstance, Mock_ifstream)
                                    .WhenCalledWith<const ghc::filesystem::path>
                                        (gccExpectedImportPath)
                                    .Times(1)
                                    .WhenCalledExpectedly_Do
                                    (
                                        [&gccImportIfstreamInstance]
                                        (void* instance, const std::vector<void*>&) 
                                        {
                                            gccImportIfstreamInstance = instance;
                                        }
                                    )
                                    .ReturnsResult();
            std::shared_ptr<OverrideResult> filetypesImportedFileStreamResult = 
                CO_INSERT_NO_REF(OverrideInstance, Mock_ifstream)
                                    .WhenCalledWith<const ghc::filesystem::path>
                                        (filetypesExpectedImportPath)
                                    .Times(1)
                                    .WhenCalledExpectedly_Do
                                    (
                                        [&filetypesImportIfstreamInstance]
                                        (void* instance, const std::vector<void*>&) 
                                        {
                                            filetypesImportIfstreamInstance = instance;
                                        }
                                    )
                                    .ReturnsResult();
            std::shared_ptr<OverrideResult> gccCompilerLinkerImportedFileStreamResult = 
                CO_INSERT_NO_REF(OverrideInstance, Mock_ifstream)
                                    .WhenCalledWith<const ghc::filesystem::path>
                                        (gccCompilerLinkerExpectedImportPath)
                                    .Times(1)
                                    .WhenCalledExpectedly_Do
                                    (
                                        [&gccCompilerLinkerImportIfstreamInstance]
                                        (void* instance, const std::vector<void*>&) 
                                        {
                                            gccCompilerLinkerImportIfstreamInstance = instance;
                                        }
                                    )
                                    .ReturnsResult();
            
            //Mock for rdbuf call with specific path check for import files
            std::shared_ptr<OverrideResult> gccImportedFileRdbufResult = 
                CO_INSERT_REF   (OverrideInstance, std::Mock_ifstream, rdbuf)
                                    .Returns<std::string>(importedGccProfileYamlStr)
                                    .Times(1)
                                    .If
                                    (
                                        [&gccImportIfstreamInstance]
                                        (void* instance, const std::vector<void*>&) -> bool
                                        {
                                            return instance == gccImportIfstreamInstance;
                                        }
                                    )
                                    .ReturnsResult();
            std::shared_ptr<OverrideResult> filetypesImportedFileRdbufResult = 
                CO_INSERT_REF   (OverrideInstance, std::Mock_ifstream, rdbuf)
                                    .Returns<std::string>(importedFiletypesYamlStr)
                                    .Times(1)
                                    .If
                                    (
                                        [&filetypesImportIfstreamInstance]
                                        (void* instance, const std::vector<void*>&) -> bool
                                        {
                                            return instance == filetypesImportIfstreamInstance;
                                        }
                                    )
                                    .ReturnsResult();
            std::shared_ptr<OverrideResult> gccCompilerLinkerImportedFileRdbufResult = 
                CO_INSERT_REF   (OverrideInstance, std::Mock_ifstream, rdbuf)
                                    .Returns<std::string>(importedCompilerLinkerYamlStr)
                                    .Times(1)
                                    .If
                                    (
                                        [&gccCompilerLinkerImportIfstreamInstance]
                                        (void* instance, const std::vector<void*>&) -> bool
                                        {
                                            return  instance == 
                                                    gccCompilerLinkerImportIfstreamInstance;
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
        ssTEST_OUTPUT_ASSERT("", gccImportFileExistResult->GetSucceedCount(), 1);
        ssTEST_OUTPUT_ASSERT("", filetypesImportFileExistResult->GetSucceedCount(), 1);
        ssTEST_OUTPUT_ASSERT("", gccCompilerLinkerImportFileExistResult->GetSucceedCount(), 1);
        
        ssTEST_OUTPUT_ASSERT("", gccImportedFileStreamResult->GetSucceedCount(), 1);
        ssTEST_OUTPUT_ASSERT("", filetypesImportedFileStreamResult->GetSucceedCount(), 1);
        ssTEST_OUTPUT_ASSERT("", gccCompilerLinkerImportedFileStreamResult->GetSucceedCount(), 1);
        
        ssTEST_OUTPUT_ASSERT("", gccImportedFileRdbufResult->GetSucceedCount(), 1);
        ssTEST_OUTPUT_ASSERT("", filetypesImportedFileRdbufResult->GetSucceedCount(), 1);
        ssTEST_OUTPUT_ASSERT("", gccCompilerLinkerImportedFileRdbufResult->GetSucceedCount(), 1);
        
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
                                    profiles[0].NameAliases.find("g++") != 
                                        profiles[0].NameAliases.end());
            ssTEST_OUTPUT_ASSERT(   "PreferredProfile should match the imported profile", 
                                    preferredProfile, 
                                    "imported-profile");
        }
    };

    ssTEST_END_TEST_GROUP();

    return 0;
} 
