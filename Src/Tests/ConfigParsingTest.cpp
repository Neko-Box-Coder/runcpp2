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
    void* userConfigIfstreamInstance = nullptr;
    void* versionIfstreamInstance = nullptr;
    
    ssTEST_COMMON_SETUP
    {
        ssLOG_SET_CURRENT_THREAD_TARGET_LEVEL(ssLOG_LEVEL_WARNING);
        
        /*
        Override so that the test can perform action for the config and version for:
        - Check if the file exists, which returns true
        - Check if config path is a directory, which returns false
        - Create Mock_ifstream with the correct filepath
        - Operator! for Mock_ifstream which returns false if Mock_ifstream instances match
        - rdbuf for version file Mock_ifstream instance
        
        And WriteDefaultConfigs() which always returns false
        */
        
        CO_INSTRUCT_REF (OverrideInstance, ghc::filesystem, Mock_exists)
                        .WhenCalledWith<const std::string&, CO_ANY_TYPE>(configPath, CO_ANY)
                        .Times(1)
                        .Returns<bool>(true)
                        .Expected();
        CO_INSTRUCT_REF (OverrideInstance, ghc::filesystem, Mock_exists)
                        .WhenCalledWith<const std::string&, CO_ANY_TYPE>(versionPath, CO_ANY)
                        .Times(1)
                        .Returns<bool>(true)
                        .Expected();
        CO_INSTRUCT_REF (OverrideInstance, ghc::filesystem, Mock_is_directory)
                        .WhenCalledWith<const std::string&, CO_ANY_TYPE>(configPath, CO_ANY)
                        .Times(1)
                        .Returns<bool>(false)
                        .Expected();
        CO_INSTRUCT_NO_REF  (OverrideInstance, Mock_ifstream)
                            .WhenCalledWith<const ghc::filesystem::path&>(configPath)
                            .WhenCalledExpectedly_Do
                            (
                                [&userConfigIfstreamInstance]
                                (void* instance, std::vector<CppOverride::TypedDataInfo>&)
                                {
                                    userConfigIfstreamInstance = instance;
                                }
                            )
                            .Times(1)
                            .Expected();
        CO_INSTRUCT_NO_REF  (OverrideInstance, Mock_ifstream)
                            .WhenCalledWith<const ghc::filesystem::path&>(versionPath)
                            .WhenCalledExpectedly_Do
                            (
                                [&versionIfstreamInstance]
                                (void* instance, std::vector<CppOverride::TypedDataInfo>&)
                                {
                                    versionIfstreamInstance = instance;
                                }
                            )
                            .Times(1)
                            .Expected();
        //Unassign ifstream pointers when done
        CO_INSTRUCT_NO_REF  (OverrideInstance, ~Mock_ifstream)
                            .WhenCalledExpectedly_Do
                            (
                                [&userConfigIfstreamInstance, &versionIfstreamInstance]
                                (void* instance, std::vector<CppOverride::TypedDataInfo>&)
                                {
                                    if(instance == userConfigIfstreamInstance)
                                        userConfigIfstreamInstance = nullptr;
                                    else if(instance == versionIfstreamInstance)
                                        versionIfstreamInstance = nullptr;
                                }
                            );
        CO_INSTRUCT_REF (OverrideInstance, std::Mock_ifstream, operator!)
                        .If
                        (
                            [&userConfigIfstreamInstance, &versionIfstreamInstance]
                            (void* instance, ...) -> bool
                            {
                                return  instance == userConfigIfstreamInstance ||
                                        instance == versionIfstreamInstance;
                            }
                        )
                        .Times(2)
                        .Returns<bool>(false)
                        .Expected();
        CO_INSTRUCT_REF (OverrideInstance, std::Mock_ifstream, rdbuf)
                        .Returns<std::string>(versionString)
                        .If
                        (
                            [&versionIfstreamInstance]
                            (void* instance, ...) -> bool
                            {
                                return instance == versionIfstreamInstance;
                            }
                        )
                        .Times(1)
                        .Expected();
        CO_INSTRUCT_REF (OverrideInstance, runcpp2, WriteDefaultConfigs)
                        .Returns<bool>(false)
                        .ExpectedNotSatisfied();
    };
    
    auto commonDefaultOverrideSetup = [&]()
    {
        CO_INSTRUCT_REF(OverrideInstance, ghc::filesystem, Mock_exists).Returns<bool>(false);
        CO_INSTRUCT_REF(OverrideInstance, ghc::filesystem, Mock_is_directory).Returns<bool>(false);
        CO_INSTRUCT_REF(OverrideInstance, std::Mock_ifstream, operator!).Returns<bool>(true);
        CO_INSTRUCT_REF(OverrideInstance, std::Mock_ifstream, rdbuf).Returns<std::string>("");
        CO_INSTRUCT_REF(OverrideInstance, runcpp2, WriteDefaultConfigs).Returns<bool>(false);
    };
    
    ssTEST_COMMON_CLEANUP
    {
        CO_CLEAR_ALL_INSTRUCTS(OverrideInstance);
        userConfigIfstreamInstance = nullptr;
        versionIfstreamInstance = nullptr;
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
            
            CO_INSTRUCT_REF (OverrideInstance, std::Mock_ifstream, rdbuf)
                            .Returns<std::string>(yamlStr)
                            .If
                            (
                                [&userConfigIfstreamInstance]
                                (void* instance, ...) -> bool
                                {
                                    return instance == userConfigIfstreamInstance;
                                }
                            )
                            .Times(1)
                            .Expected();
            commonDefaultOverrideSetup();
            
            std::vector<runcpp2::Data::Profile> profiles;
            std::string preferredProfile;
        );
        
        ssTEST_OUTPUT_EXECUTION
        (
            bool parseResult = runcpp2::ReadUserConfig(profiles, preferredProfile, configPath);
        );
        
        ssTEST_OUTPUT_ASSERT("ReadUserConfig should succeed", parseResult);
        ssTEST_OUTPUT_ASSERT("Should parse 3 profiles", profiles.size() == 3);
        ssTEST_OUTPUT_ASSERT(preferredProfile == "g++");
        ssTEST_OUTPUT_ASSERT("", CO_GET_FAILED_FUNCTIONS(OverrideInstance).size(), 0);
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
            
            CO_INSTRUCT_REF (OverrideInstance, std::Mock_ifstream, rdbuf)
                            .Returns<std::string>(yamlStr)
                            .Times(1)
                            .If
                            (
                                [&userConfigIfstreamInstance]
                                (void* instance, ...) -> bool
                                {
                                    return instance == userConfigIfstreamInstance;
                                }
                            )
                            .Expected();
            CO_INSTRUCT_NO_REF  (OverrideInstance, GetPlatformNames)
                                .Returns<std::vector<std::string>>({"MacOS", 
                                                                    "Unix", 
                                                                    "DefaultPlatform"})
                                .Times(1)
                                .Expected();
            commonDefaultOverrideSetup();
            
            std::vector<runcpp2::Data::Profile> profiles;
            std::string preferredProfile;
        );
        
        ssTEST_OUTPUT_EXECUTION
        (
            bool parseResult = runcpp2::ReadUserConfig(profiles, preferredProfile, configPath);
        );
        
        ssTEST_OUTPUT_ASSERT("ReadUserConfig should succeed", parseResult);
        ssTEST_OUTPUT_ASSERT("Should parse 3 profiles", profiles.size() == 3);
        ssTEST_OUTPUT_ASSERT("MacOS PreferredProfile should be g++3", preferredProfile == "g++3");
        ssTEST_OUTPUT_ASSERT("", CO_GET_FAILED_FUNCTIONS(OverrideInstance).size(), 0);
        
        ssTEST_OUTPUT_SETUP
        (
            ssTEST_CALL_COMMON_CLEANUP();
            ssTEST_CALL_COMMON_SETUP();
            profiles.clear();
            preferredProfile.clear();
            
            CO_INSTRUCT_REF (OverrideInstance, std::Mock_ifstream, rdbuf)
                            .Returns<std::string>(yamlStr)
                            .Times(1)
                            .If
                            (
                                [&userConfigIfstreamInstance]
                                (void* instance, ...) -> bool
                                {
                                    return instance == userConfigIfstreamInstance;
                                }
                            )
                            .Expected();
            CO_INSTRUCT_NO_REF  (OverrideInstance, GetPlatformNames)
                                .Returns<std::vector<std::string>>({"Linux", 
                                                                    "Unix", 
                                                                    "DefaultPlatform"})
                                .Times(1)
                                .Expected();
            commonDefaultOverrideSetup();
        );
        
        ssTEST_OUTPUT_EXECUTION
        (
            parseResult = runcpp2::ReadUserConfig(profiles, preferredProfile, configPath);
        );
        
        ssTEST_OUTPUT_ASSERT("ReadUserConfig should succeed", parseResult);
        ssTEST_OUTPUT_ASSERT("Should parse 3 profiles", profiles.size() == 3);
        ssTEST_OUTPUT_ASSERT("DefaultProfile PreferredProfile should be g++", preferredProfile, "g++");
        ssTEST_OUTPUT_ASSERT("", CO_GET_FAILED_FUNCTIONS(OverrideInstance).size(), 0);
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
            CO_INSTRUCT_REF (OverrideInstance, std::Mock_ifstream, rdbuf)
                            .Returns<std::string>(mainConfigYamlStr)
                            .Times(1)
                            .If
                            (
                                [&userConfigIfstreamInstance]
                                (void* instance, ...) -> bool
                                {
                                    return instance == userConfigIfstreamInstance;
                                }
                            )
                            .Expected();
            
            //Setup filesystem exist for the import yaml files
            std::string gccExpectedImportPath = "some/config/dir/Default/gcc.yaml";
            std::string filetypesExpectedImportPath = "some/config/dir/Default/filetypes.yaml";
            std::string gccCompilerLinkerExpectedImportPath = 
                "some/config/dir/Default/gccCompilerLinker.yaml";
            
            CO_INSTRUCT_REF (OverrideInstance, ghc::filesystem, Mock_exists)
                            .WhenCalledWith<const std::string&, 
                                            CO_ANY_TYPE>(   gccExpectedImportPath, 
                                                            CO_ANY)
                            .Returns<bool>(true)
                            .Times(1)
                            .Expected();
            CO_INSTRUCT_REF (OverrideInstance, ghc::filesystem, Mock_exists)
                            .WhenCalledWith<const std::string&, 
                                            CO_ANY_TYPE>(   filetypesExpectedImportPath, 
                                                            CO_ANY)
                            .Returns<bool>(true)
                            .Times(1)
                            .Expected();
            CO_INSTRUCT_REF (OverrideInstance, ghc::filesystem, Mock_exists)
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
                            .Times(1)
                            .Expected();
            
            //Record if there's an ifstream created with import paths
            void* gccImportIfstreamInstance = nullptr;
            void* filetypesImportIfstreamInstance = nullptr;
            void* gccCompilerLinkerImportIfstreamInstance = nullptr;
            CO_INSTRUCT_NO_REF  (OverrideInstance, Mock_ifstream)
                                .WhenCalledWith<const ghc::filesystem::path>
                                    (gccExpectedImportPath)
                                .Times(1)
                                .WhenCalledExpectedly_Do
                                (
                                    [&gccImportIfstreamInstance]
                                    (void* instance, ...) 
                                    {
                                        gccImportIfstreamInstance = instance;
                                    }
                                )
                                .Expected();
            CO_INSTRUCT_NO_REF  (OverrideInstance, Mock_ifstream)
                                .WhenCalledWith<const ghc::filesystem::path>
                                    (filetypesExpectedImportPath)
                                .Times(1)
                                .WhenCalledExpectedly_Do
                                (
                                    [&filetypesImportIfstreamInstance]
                                    (void* instance, ...) 
                                    {
                                        filetypesImportIfstreamInstance = instance;
                                    }
                                )
                                .Expected();
            CO_INSTRUCT_NO_REF  (OverrideInstance, Mock_ifstream)
                                .WhenCalledWith<const ghc::filesystem::path>
                                    (gccCompilerLinkerExpectedImportPath)
                                .Times(1)
                                .WhenCalledExpectedly_Do
                                (
                                    [&gccCompilerLinkerImportIfstreamInstance]
                                    (void* instance, ...) 
                                    {
                                        gccCompilerLinkerImportIfstreamInstance = instance;
                                    }
                                )
                                .Expected();
            //Unassign ifstream pointers when done
            CO_REMOVE_INSTRUCT_NO_REF(OverrideInstance, ~Mock_ifstream);
            CO_INSTRUCT_NO_REF  (OverrideInstance, ~Mock_ifstream)
                                .WhenCalledExpectedly_Do
                                (
                                    [
                                        &userConfigIfstreamInstance, 
                                        &versionIfstreamInstance,
                                        &gccImportIfstreamInstance,
                                        &filetypesImportIfstreamInstance,
                                        &gccCompilerLinkerImportIfstreamInstance
                                    ]
                                    (void* instance, ...)
                                    {
                                        if(instance == userConfigIfstreamInstance)
                                            userConfigIfstreamInstance = nullptr;
                                        else if(instance == versionIfstreamInstance)
                                            versionIfstreamInstance = nullptr;
                                        else if(instance == gccImportIfstreamInstance)
                                            gccImportIfstreamInstance = nullptr;
                                        else if(instance == filetypesImportIfstreamInstance)
                                            filetypesImportIfstreamInstance = nullptr;
                                        else if(instance == gccCompilerLinkerImportIfstreamInstance)
                                            gccCompilerLinkerImportIfstreamInstance = nullptr;
                                    }
                                );
            //Mock for rdbuf call with specific path check for import files
            CO_INSTRUCT_REF (OverrideInstance, std::Mock_ifstream, rdbuf)
                            .Returns<std::string>(importedGccProfileYamlStr)
                            .Times(1)
                            .If
                            (
                                [&gccImportIfstreamInstance]
                                (void* instance, ...) -> bool
                                {
                                    return instance == gccImportIfstreamInstance;
                                }
                            )
                            .Expected();
            CO_INSTRUCT_REF (OverrideInstance, std::Mock_ifstream, rdbuf)
                            .Returns<std::string>(importedFiletypesYamlStr)
                            .Times(1)
                            .If
                            (
                                [&filetypesImportIfstreamInstance]
                                (void* instance, ...) -> bool
                                {
                                    return instance == filetypesImportIfstreamInstance;
                                }
                            )
                            .Expected();
            CO_INSTRUCT_REF (OverrideInstance, std::Mock_ifstream, rdbuf)
                            .Returns<std::string>(importedCompilerLinkerYamlStr)
                            .Times(1)
                            .If
                            (
                                [&gccCompilerLinkerImportIfstreamInstance]
                                (void* instance, ...) -> bool
                                {
                                    return  instance == 
                                            gccCompilerLinkerImportIfstreamInstance;
                                }
                            )
                            .Expected();
            
            //Tracking each Mock_ifstream instance is too cumbersome, just return false for all 
            //operator! since rdbuf won't work anyway even if it passed the operator! check
            CO_INSTRUCT_REF(OverrideInstance, std::Mock_ifstream, operator!).Returns<bool>(false);
            commonDefaultOverrideSetup();
            
            std::vector<runcpp2::Data::Profile> profiles;
            std::string preferredProfile;
        );
        
        ssTEST_OUTPUT_EXECUTION
        (
            bool parseResult = runcpp2::ReadUserConfig(profiles, preferredProfile, configPath);
        );
        
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
        ssTEST_OUTPUT_ASSERT("", CO_GET_FAILED_FUNCTIONS(OverrideInstance).size(), 0);
    };

    ssTEST_END_TEST_GROUP();

    return 0;
}
