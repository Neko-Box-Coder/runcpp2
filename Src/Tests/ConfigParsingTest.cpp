#include "runcpp2/ConfigParsing.hpp"

#include "DSResult/DSResult.hpp"
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


DS::Result<void> TestMain()
{
    using namespace CppOverride;
    
    const std::string configPath = "some/config/dir/UserConfig.yaml";
    const std::string versionPath = "some/config/dir/.version";
    #define STR(x) #x
        const std::string versionString = MPT_COMPOSE(STR, (RUNCPP2_CONFIG_VERSION));
    #undef STR
    void* userConfigIfstreamInstance = nullptr;
    void* versionIfstreamInstance = nullptr;
    
    auto setup = [  &configPath, 
                    &versionPath, 
                    &userConfigIfstreamInstance, 
                    &versionIfstreamInstance,
                    &versionString]
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
                        .WhenCalledWith<const ghc::filesystem::path&, CO_ANY_TYPE>(configPath, CO_ANY)
                        .Times(1)
                        .Returns<bool>(true)
                        .Expected();
        CO_INSTRUCT_REF (OverrideInstance, ghc::filesystem, Mock_exists)
                        .WhenCalledWith<const ghc::filesystem::path&, CO_ANY_TYPE>( versionPath, 
                                                                                    CO_ANY)
                        .Times(1)
                        .Returns<bool>(true)
                        .Expected();
        CO_INSTRUCT_REF (OverrideInstance, ghc::filesystem, Mock_is_directory)
                        .WhenCalledWith<const ghc::filesystem::path&, CO_ANY_TYPE>( configPath, 
                                                                                    CO_ANY)
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
        CO_INSTRUCT_REF (OverrideInstance, Mock_std::Mock_ifstream, operator!)
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
        CO_INSTRUCT_REF (OverrideInstance, Mock_std::Mock_ifstream, rdbuf)
                        .Returns<std::string>(versionString)
                        .If
                        (
                            [&versionIfstreamInstance](void* instance, ...) -> bool
                            {
                                return instance == versionIfstreamInstance;
                            }
                        )
                        .Times(1)
                        .Expected();
        CO_INSTRUCT_REF (OverrideInstance, runcpp2, WriteDefaultConfigs)
                        .Returns<DS::Result<void>>(DS_ERROR_MSG("Unexpected..."))
                        .ExpectedNotSatisfied();
    };
    
    auto commonDefaultOverrideSetup = []()
    {
        CO_INSTRUCT_REF(OverrideInstance, ghc::filesystem, Mock_exists).Returns<bool>(false);
        CO_INSTRUCT_REF(OverrideInstance, ghc::filesystem, Mock_is_directory).Returns<bool>(false);
        CO_INSTRUCT_REF(OverrideInstance, Mock_std::Mock_ifstream, operator!).Returns<bool>(true);
        CO_INSTRUCT_REF(OverrideInstance, Mock_std::Mock_ifstream, rdbuf).Returns<std::string>("");
        CO_INSTRUCT_REF(OverrideInstance, 
                        runcpp2, 
                        WriteDefaultConfigs).Returns<DS::Result<void>>(DS_ERROR_MSG("Unexpected..."));
    };
    
    auto cleanup = [&userConfigIfstreamInstance, &versionIfstreamInstance]
    {
        CO_CLEAR_ALL_INSTRUCTS(OverrideInstance);
        userConfigIfstreamInstance = nullptr;
        versionIfstreamInstance = nullptr;
    };

    const char* commonUserConfigForVersionFile = R"(
        PreferredProfile: "g++"
        Profiles:
        -   Name: "g++"
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

    //ReadUserConfig Should Not Write Default Configs When Updated Version File Exists
    {
        setup();
        
        CO_INSTRUCT_REF (OverrideInstance, Mock_std::Mock_ifstream, rdbuf)
                        .Returns<std::string>(commonUserConfigForVersionFile)
                        .If
                        (
                            [&userConfigIfstreamInstance](void* instance, ...) -> bool
                            {
                                return instance == userConfigIfstreamInstance;
                            }
                        )
                        .Times(1)
                        .Expected();
            
        std::vector<runcpp2::Data::Profile> profiles;
        std::string preferredProfile;
        commonDefaultOverrideSetup();
        
        runcpp2::ReadUserConfig(profiles, preferredProfile, configPath).DS_TRY();
        DS_ASSERT_EQ(profiles.size(), 1);
        DS_ASSERT_EQ(preferredProfile, "g++");
        DS_ASSERT_EQ(CO_GET_FAILED_FUNCTIONS(OverrideInstance).size(), 0);
    
        cleanup();
    }
    
    //ReadUserConfig Should Write Default Configs When Outdated Version File Exists
    {
        setup();
        
        CO_REMOVE_INSTRUCT_REF(OverrideInstance, runcpp2, WriteDefaultConfigs);
        CO_REMOVE_INSTRUCT_REF(OverrideInstance, Mock_std::Mock_ifstream, rdbuf);
        
        const std::string outdatedVersionString = std::to_string(RUNCPP2_CONFIG_VERSION - 1);
        CO_INSTRUCT_REF (OverrideInstance, Mock_std::Mock_ifstream, rdbuf)
                        .Returns<std::string>(outdatedVersionString)
                        .If
                        (
                            [&versionIfstreamInstance](void* instance, ...) -> bool
                            {
                                return instance == versionIfstreamInstance;
                            }
                        )
                        .Times(1)
                        .Expected();
        CO_INSTRUCT_REF (OverrideInstance, Mock_std::Mock_ifstream, rdbuf)
                        .Returns<std::string>(commonUserConfigForVersionFile)
                        .If
                        (
                            [&userConfigIfstreamInstance](void* instance, ...) -> bool
                            {
                                return instance == userConfigIfstreamInstance;
                            }
                        )
                        .Times(1)
                        .Expected();
        CO_INSTRUCT_REF (OverrideInstance, runcpp2, WriteDefaultConfigs)
                        .WhenCalledWith<const ghc::filesystem::path&, 
                                        bool, 
                                        bool>(  configPath, 
                                                false,      //writeUserConfig
                                                true)       //writeDefaultConfigs
                        .Returns<DS::Result<void>>({})
                        .Expected();
        
        std::vector<runcpp2::Data::Profile> profiles;
        std::string preferredProfile;
        commonDefaultOverrideSetup();
        
        runcpp2::ReadUserConfig(profiles, preferredProfile, configPath).DS_TRY();
        DS_ASSERT_EQ(profiles.size(), 1);
        DS_ASSERT_EQ(preferredProfile, "g++");
        DS_ASSERT_EQ(CO_GET_FAILED_FUNCTIONS(OverrideInstance).size(), 0);
        
        cleanup();
    }
    
    //ReadUserConfig Should Write Default Configs When Version File Doesn't Exists
    {
        setup();
        cleanup();
        
        CO_INSTRUCT_REF (OverrideInstance, ghc::filesystem, Mock_exists)
                        .WhenCalledWith<const ghc::filesystem::path&, CO_ANY_TYPE>( configPath, 
                                                                                    CO_ANY)
                        .Times(1)
                        .Returns<bool>(true)
                        .Expected();
        CO_INSTRUCT_REF (OverrideInstance, ghc::filesystem, Mock_exists)
                        .WhenCalledWith<const ghc::filesystem::path&, CO_ANY_TYPE>( versionPath, 
                                                                                    CO_ANY)
                        .Times(1)
                        .Returns<bool>(false)
                        .Expected();
        CO_INSTRUCT_REF (OverrideInstance, runcpp2, WriteDefaultConfigs)
                        .WhenCalledWith<const ghc::filesystem::path&, 
                                        bool, 
                                        bool>(  configPath, 
                                                false,      //writeUserConfig
                                                true)       //writeDefaultConfigs
                        .Returns<DS::Result<void>>({})
                        .Expected();
        CO_INSTRUCT_NO_REF  (OverrideInstance, Mock_ifstream)
                            .WhenCalledWith<const ghc::filesystem::path&>(configPath)
                            .WhenCalledExpectedly_Do
                            (
                                [&userConfigIfstreamInstance](void* instance, ...)
                                {
                                    userConfigIfstreamInstance = instance;
                                }
                            )
                            .Times(1)
                            .Expected();
        CO_INSTRUCT_REF (OverrideInstance, Mock_std::Mock_ifstream, operator!)
                        .If
                        (
                            [&userConfigIfstreamInstance, &versionIfstreamInstance]
                            (void* instance, ...) -> bool
                            {
                                return  instance == userConfigIfstreamInstance;
                            }
                        )
                        .Times(1)
                        .Returns<bool>(false)
                        .Expected();
        CO_INSTRUCT_REF (OverrideInstance, Mock_std::Mock_ifstream, rdbuf)
                        .Returns<std::string>(commonUserConfigForVersionFile)
                        .If
                        (
                            [&userConfigIfstreamInstance](void* instance, ...) -> bool
                            {
                                return instance == userConfigIfstreamInstance;
                            }
                        )
                        .Times(1)
                        .Expected();
        
        std::vector<runcpp2::Data::Profile> profiles;
        std::string preferredProfile;
        commonDefaultOverrideSetup();
        
        runcpp2::ReadUserConfig(profiles, preferredProfile, configPath).DS_TRY();
        DS_ASSERT_EQ(CO_GET_FAILED_FUNCTIONS(OverrideInstance).size(), 0);
    
        cleanup();
    }

    //ReadUserConfig Should Import Profile From External YAML File
    {
        setup();
        
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
        CO_INSTRUCT_REF (OverrideInstance, Mock_std::Mock_ifstream, rdbuf)
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
        std::string gccCompilerLinkerExpectedImportPath =   "some/config/dir/Default/"
                                                            "gccCompilerLinker.yaml";
        
        CO_INSTRUCT_REF (OverrideInstance, ghc::filesystem, Mock_exists)
                        .WhenCalledWith<const ghc::filesystem::path&, 
                                        CO_ANY_TYPE>(   gccExpectedImportPath, 
                                                        CO_ANY)
                        .Returns<bool>(true)
                        .Times(1)
                        .Expected();
        CO_INSTRUCT_REF (OverrideInstance, ghc::filesystem, Mock_exists)
                        .WhenCalledWith<const ghc::filesystem::path&, 
                                        CO_ANY_TYPE>(   filetypesExpectedImportPath, 
                                                        CO_ANY)
                        .Returns<bool>(true)
                        .Times(1)
                        .Expected();
        CO_INSTRUCT_REF (OverrideInstance, ghc::filesystem, Mock_exists)
                        .WhenCalledWith<const ghc::filesystem::path&, 
                                        CO_ANY_TYPE>(   gccCompilerLinkerExpectedImportPath, 
                                                        CO_ANY)
                        .Returns<bool>(true)
                        .Times(1)
                        .Expected();
        
        //Record if there's an ifstream created with import paths
        void* gccImportIfstreamInstance = nullptr;
        void* filetypesImportIfstreamInstance = nullptr;
        void* gccCompilerLinkerImportIfstreamInstance = nullptr;
        CO_INSTRUCT_NO_REF  (OverrideInstance, Mock_ifstream)
                            .WhenCalledWith<const ghc::filesystem::path>(gccExpectedImportPath)
                            .Times(1)
                            .WhenCalledExpectedly_Do
                            (
                                [&gccImportIfstreamInstance](void* instance, ...) 
                                {
                                    gccImportIfstreamInstance = instance;
                                }
                            )
                            .Expected();
        CO_INSTRUCT_NO_REF  (OverrideInstance, Mock_ifstream)
                            .WhenCalledWith<const ghc::filesystem::path>(filetypesExpectedImportPath)
                            .Times(1)
                            .WhenCalledExpectedly_Do
                            (
                                [&filetypesImportIfstreamInstance](void* instance, ...) 
                                {
                                    filetypesImportIfstreamInstance = instance;
                                }
                            )
                            .Expected();
        CO_INSTRUCT_NO_REF  (OverrideInstance, Mock_ifstream)
                            .WhenCalledWith <const ghc::filesystem::path>
                                            (gccCompilerLinkerExpectedImportPath)
                            .Times(1)
                            .WhenCalledExpectedly_Do
                            (
                                [&gccCompilerLinkerImportIfstreamInstance](void* instance, ...) 
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
        CO_INSTRUCT_REF (OverrideInstance, Mock_std::Mock_ifstream, rdbuf)
                        .Returns<std::string>(importedGccProfileYamlStr)
                        .Times(1)
                        .If
                        (
                            [&gccImportIfstreamInstance](void* instance, ...) -> bool
                            {
                                return instance == gccImportIfstreamInstance;
                            }
                        )
                        .Expected();
        CO_INSTRUCT_REF (OverrideInstance, Mock_std::Mock_ifstream, rdbuf)
                        .Returns<std::string>(importedFiletypesYamlStr)
                        .Times(1)
                        .If
                        (
                            [&filetypesImportIfstreamInstance](void* instance, ...) -> bool
                            {
                                return instance == filetypesImportIfstreamInstance;
                            }
                        )
                        .Expected();
        CO_INSTRUCT_REF (OverrideInstance, Mock_std::Mock_ifstream, rdbuf)
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
        CO_INSTRUCT_REF(OverrideInstance, Mock_std::Mock_ifstream, operator!).Returns<bool>(false);
        commonDefaultOverrideSetup();
        
        std::vector<runcpp2::Data::Profile> profiles;
        std::string preferredProfile;
        
        runcpp2::ReadUserConfig(profiles, preferredProfile, configPath).DS_TRY();
        DS_ASSERT_EQ(profiles.size(), 2);
        if(!profiles.empty())
        {
            DS_ASSERT_EQ(profiles[0].Name, "imported-profile");
            DS_ASSERT_EQ(   profiles[0] .Compiler
                                        .OutputTypes
                                        .Executable.at("DefaultPlatform")
                                        .Executable, 
                            "imported-gcc");
            DS_ASSERT_EQ(   profiles[0].FilesTypes.SharedLinkFile.Prefix.at("DefaultPlatform"),
                            "file-prefix");
            DS_ASSERT_TRUE( profiles[0].NameAliases.size() == 1 && 
                            profiles[0].NameAliases.find("g++") != profiles[0].NameAliases.end());
            DS_ASSERT_EQ(preferredProfile, "imported-profile");
        }
        DS_ASSERT_EQ(CO_GET_FAILED_FUNCTIONS(OverrideInstance).size(), 0);
        
        cleanup();
    };
    
    return {};
}

int main(int argc, char** argv)
{
    try
    {
        TestMain().DS_TRY_ACT(  ssLOG_LINE(CO_GET_FAILED_REPORT(OverrideInstance)); 
                                ssLOG_LINE(DS_TMP_ERROR.ToString()); 
                                return 1);
        return 0;
    }
    catch(std::exception& ex)
    {
        ssLOG_LINE(ex.what());
        return 1;
    }
    return 1;
}

