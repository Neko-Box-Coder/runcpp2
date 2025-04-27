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

int main(int argc, char** argv)
{
    ssTEST_INIT_TEST_GROUP();
    
    ssTEST_PARSE_ARGS(argc, argv);
    
    using namespace CppOverride;
    
    std::string configPath;
    std::shared_ptr<OverrideResult> existsResult = CreateOverrideResult();
    std::shared_ptr<OverrideResult> isDirResult = CreateOverrideResult();
    std::shared_ptr<OverrideResult> ifstreamResult = CreateOverrideResult();
    std::shared_ptr<OverrideResult> ifstreamSucceedResult = CreateOverrideResult();
    std::shared_ptr<OverrideResult> writeDefaultConfigResult = CreateOverrideResult();
    std::shared_ptr<OverrideResult> rdbufResult = CreateOverrideResult();
    
    ssTEST_COMMON_SETUP
    {
        CO_CLEAR_ALL_OVERRIDE_SETUP(OverrideInstance);
        ssLOG_SET_CURRENT_THREAD_TARGET_LEVEL(ssLOG_LEVEL_WARNING);
        
        configPath = "TestConfig.yaml";
        CO_SETUP_OVERRIDE   (OverrideInstance, Mock_exists)
                            .WhenCalledWith<const std::string&, CO_ANY_TYPE>(configPath, CO_ANY)
                            .Returns<bool>(true)
                            .AssignResult(existsResult);
        
        CO_SETUP_OVERRIDE   (OverrideInstance, Mock_is_directory)
                            .WhenCalledWith<const std::string&, CO_ANY_TYPE>(configPath, CO_ANY)
                            .Returns<bool>(false)
                            .AssignResult(isDirResult);
        
        CO_SETUP_OVERRIDE   (OverrideInstance, Mock_ifstream)
                            .Returns<void>()
                            .AssignResult(ifstreamResult);
        
        CO_SETUP_OVERRIDE   (OverrideInstance, operator!)
                            .Returns<bool>(false)
                            .AssignResult(ifstreamSucceedResult);
        
        CO_SETUP_OVERRIDE   (OverrideInstance, WriteDefaultConfig)
                            .Returns<bool>(false)
                            .AssignResult(writeDefaultConfigResult);
    };
    
    auto commonOverrideStatusCheck = [&]()
    {
        ssTEST_OUTPUT_ASSERT(existsResult->LastStatusSucceed());
        ssTEST_OUTPUT_ASSERT(isDirResult->LastStatusSucceed());
        ssTEST_OUTPUT_ASSERT(ifstreamResult->LastStatusSucceed());
        ssTEST_OUTPUT_ASSERT(ifstreamSucceedResult->LastStatusSucceed());
        ssTEST_OUTPUT_ASSERT(rdbufResult->LastStatusSucceed());
        ssTEST_OUTPUT_ASSERT(   "WriteDefaultConfig() should not be called", 
                                writeDefaultConfigResult->GetStatusCount(), 0);
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
            
            CO_SETUP_OVERRIDE   (OverrideInstance, rdbuf)
                                .Returns<std::string>(yamlStr)
                                .Times(1)
                                .AssignResult(rdbufResult);
            
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
            
            CO_SETUP_OVERRIDE   (OverrideInstance, rdbuf)
                                .Returns<std::string>(yamlStr)
                                .Times(2)
                                .AssignResult(rdbufResult);
            
            std::shared_ptr<OverrideResult> getPlatformNamesResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, GetPlatformNames)
                                .Returns<std::vector<std::string>>({"MacOS", 
                                                                    "Unix", 
                                                                    "DefaultPlatform"})
                                .Times(1)
                                .AssignResult(getPlatformNamesResult);
            
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
            CO_SETUP_OVERRIDE   (OverrideInstance, GetPlatformNames)
                                .Returns<std::vector<std::string>>({"Linux", 
                                                                    "Unix", 
                                                                    "DefaultPlatform"})
                                .Times(1)
                                .AssignResult(getPlatformNamesResult);
            profiles.clear();
            preferredProfile.clear();
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

    ssTEST_END_TEST_GROUP();

    return 0;
} 
