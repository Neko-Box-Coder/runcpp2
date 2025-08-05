#include "runcpp2/BuildsManager.hpp"

#include "ssTest.hpp"
#include "CppOverride.hpp"
#include "ssLogger/ssLog.hpp"

#define INTERNAL_RUNCPP2_UNDEF_MOCKS 1
#include "Tests/BuildsManager/MockComponents.hpp"

#include <memory>

#if !INTERNAL_RUNCPP2_UNIT_TESTS || !defined(INTERNAL_RUNCPP2_UNIT_TESTS)
    static_assert(false, "INTERNAL_RUNCPP2_UNIT_TESTS not defined");
#endif

CO_DECLARE_INSTANCE(OverrideInstance);

struct BuildsManagerAccessor
{
    inline static std::unordered_map<std::string, std::string>& 
    GetMappings(runcpp2::BuildsManager& buildsManager)
    {
        return buildsManager.Mappings;
    }
    
    inline static std::unordered_map<std::string, std::string>& 
    GetReverseMappings(runcpp2::BuildsManager& buildsManager)
    {
        return buildsManager.ReverseMappings;
    }
};

int main(int argc, char** argv)
{
    #if defined(_WIN32)
        const std::string absPathPrefix = "C:";
    #elif defined(__unix__) || defined(__APPLE__)
        const std::string absPathPrefix;
    #endif
    
    std::unique_ptr<runcpp2::BuildsManager> buildsManager(nullptr);
    std::vector<std::string> scriptsPaths = 
    {
        absPathPrefix + "/tmp/Scripts/test.cpp",
        absPathPrefix + "/tmp/Scripts/test2.cpp",
        absPathPrefix + "/tmp/Scripts/test3.cpp",
    };
    std::vector<std::string> scriptsBuildsPaths = 
    {
        "./5",
        "./10",
        "./15"
    };
    
    //NOTE: Workaround for MSVC
    static constexpr int defaultMappingsCount = 2;
    const std::string configDirPath = absPathPrefix + "/tmp/Config";
    const std::string buildsDirPath = configDirPath + "/CachedBuilds";
    const std::string mappingsFilePath = buildsDirPath + "/Mappings.csv";
    
    auto prepareInitialization = 
        [&mappingsFilePath, &scriptsPaths, &scriptsBuildsPaths](std::string customContent)
        {
            //Checking mappings file path
            CO_INSTRUCT_REF (OverrideInstance, ghc::filesystem, Mock_exists)
                            .WhenCalledWith<const ghc::filesystem::path&, 
                                            CO_ANY_TYPE>(mappingsFilePath, CO_ANY)
                            .Returns<bool>(true)
                            .Times(1)
                            .Expected();
            //Open mappings file
            CO_INSTRUCT_NO_REF  (OverrideInstance, Mock_ifstream)
                                .WhenCalledWith<const ghc::filesystem::path&>(mappingsFilePath)
                                .Returns<void>()
                                .Times(1)
                                .Expected();
            //Checking if mappings file is opened
            CO_INSTRUCT_REF (OverrideInstance, Mock_std::Mock_ifstream, is_open)
                            .Returns<bool>(true)
                            .Times(1)
                            .Expected();
            //Return file content
            std::string mappingsContent;
            if(!customContent.empty())
                mappingsContent = customContent;
            else
            {
                for(int i = 0; i < defaultMappingsCount; ++i)
                    mappingsContent += scriptsPaths.at(i) + "," + scriptsBuildsPaths.at(i) + "\n";
            }
            
            CO_INSTRUCT_REF (OverrideInstance, Mock_std::Mock_ifstream, rdbuf)
                            .Returns<std::string>(mappingsContent)
                            .Times(1)
                            .Expected();
        };
    
    ssTEST_INIT_TEST_GROUP();
    
    ssTEST_PARSE_ARGS(argc, argv);
    
    ssTEST_COMMON_SETUP
    {
        buildsManager.reset(new runcpp2::BuildsManager(configDirPath));
        CO_CLEAR_ALL_INSTRUCTS(OverrideInstance);
    };
    
    ssTEST_COMMON_CLEANUP
    {
        ssLOG_SET_CURRENT_THREAD_TARGET_LEVEL(ssLOG_LEVEL_DEBUG);
    };
    
    ssTEST("Methods Should Return False When Not Initialized")
    {
        using namespace CppOverride;

        ssTEST_OUTPUT_SETUP
        (
            CO_INSTRUCT_REF (OverrideInstance, ghc::filesystem, Mock_create_directories)
                            .Returns<bool>(false)
                            .ExpectedNotSatisfied();
            ghc::filesystem::path outPath;
        );
        ssTEST_OUTPUT_ASSERT("", buildsManager->CreateBuildMapping(scriptsPaths.front()), false);
        ssTEST_OUTPUT_ASSERT("", buildsManager->RemoveBuildMapping(scriptsPaths.front()), false);
        ssTEST_OUTPUT_ASSERT("", buildsManager->HasBuildMapping(scriptsPaths.front()), false);
        ssTEST_OUTPUT_ASSERT("", buildsManager->GetBuildMapping(scriptsPaths.front(), outPath), false);
        ssTEST_OUTPUT_ASSERT("", buildsManager->RemoveAllBuildsMappings(), false);
        ssTEST_OUTPUT_ASSERT("", buildsManager->SaveBuildsMappings(), false);
        ssTEST_OUTPUT_ASSERT("", CO_GET_FAILED_FUNCTIONS(OverrideInstance).size(), 0);
        ssTEST_OUTPUT_VALUES_WHEN_FAILED("\n" + CO_GET_FAILED_REPORT(OverrideInstance));
    };
    
    ssTEST("Initialize Should Try To Read Mappings File When It Exists")
    {
        //NOTE: Suppress error logs since we are overriding the mapping file content
        ssLOG_SET_CURRENT_THREAD_TARGET_LEVEL(ssLOG_LEVEL_FATAL);
        using namespace CppOverride;
        
        ssTEST_OUTPUT_SETUP
        (
            //Checking mappings file path
            CO_INSTRUCT_REF (OverrideInstance, ghc::filesystem, Mock_exists)
                            .WhenCalledWith<const ghc::filesystem::path&, 
                                            CO_ANY_TYPE>(mappingsFilePath, CO_ANY)
                            .Returns<bool>(true)
                            .Times(1)
                            .Expected();
            //Open mappings file
            CO_INSTRUCT_NO_REF  (OverrideInstance, Mock_ifstream)
                                .WhenCalledWith<const ghc::filesystem::path&>(mappingsFilePath)
                                .Returns<void>()
                                .Times(1)
                                .Expected();
        );
        ssTEST_OUTPUT_EXECUTION( buildsManager->Initialize(); );
        ssTEST_OUTPUT_ASSERT("", CO_GET_FAILED_FUNCTIONS(OverrideInstance).size(), 0);
        ssTEST_OUTPUT_VALUES_WHEN_FAILED("\n" + CO_GET_FAILED_REPORT(OverrideInstance));
    };
    
    ssTEST("Initialize Should Create Mappings File When It Doesn't Exists")
    {
        using namespace CppOverride;
        ssTEST_OUTPUT_SETUP
        (
            //Checking mappings file path
            CO_INSTRUCT_REF (OverrideInstance, ghc::filesystem, Mock_exists)
                            .WhenCalledWith<const ghc::filesystem::path&, 
                                            CO_ANY_TYPE>(mappingsFilePath, CO_ANY)
                            .Returns<bool>(false)
                            .Times(1)
                            .Expected();
            //Checking builds directory exist
            CO_INSTRUCT_REF (OverrideInstance, ghc::filesystem, Mock_exists)
                            .WhenCalledWith<const ghc::filesystem::path&, 
                                            CO_ANY_TYPE>(buildsDirPath, CO_ANY)
                            .Returns<bool>(false)
                            .Times(1)
                            .Expected();
            //Create builds directory
            CO_INSTRUCT_REF (OverrideInstance, ghc::filesystem, Mock_create_directories)
                            .WhenCalledWith<const ghc::filesystem::path&, 
                                            CO_ANY_TYPE>(buildsDirPath, CO_ANY)
                            .Returns<bool>(true)
                            .Times(1)
                            .Expected();
            //Creating mappings file
            CO_INSTRUCT_NO_REF  (OverrideInstance, Mock_ofstream)
                                .WhenCalledWith<const ghc::filesystem::path&>(mappingsFilePath)
                                .Returns<void>()
                                .Times(1)
                                .Expected();
            //Checking if mappings file is created
            CO_INSTRUCT_REF (OverrideInstance, Mock_std::Mock_ofstream, is_open)
                            .Returns<bool>(true)
                            .Times(1)
                            .Expected();
            //Closing mappings file
            CO_INSTRUCT_REF (OverrideInstance, Mock_std::Mock_ofstream, close)
                            .Returns<void>()
                            .Times(1)
                            .Expected();
        );
        ssTEST_OUTPUT_ASSERT("Initialize should succeed", buildsManager->Initialize(), true);
        ssTEST_OUTPUT_ASSERT("", CO_GET_FAILED_FUNCTIONS(OverrideInstance).size(), 0);
        ssTEST_OUTPUT_VALUES_WHEN_FAILED("\n" + CO_GET_FAILED_REPORT(OverrideInstance));
    };
    
    ssTEST("Initialize Should Parse Mappings File Correctly When It Exists")
    {
        using namespace CppOverride;
        ssTEST_OUTPUT_SETUP
        (
            prepareInitialization("");
            
            //When mapped build path exists
            CO_INSTRUCT_REF (OverrideInstance, ghc::filesystem, Mock_exists)
                            .WhenCalledWith<const ghc::filesystem::path&, CO_ANY_TYPE>
                            (
                                buildsDirPath + "/" + scriptsBuildsPaths.front(), 
                                CO_ANY
                            )
                            .Returns<bool>(true)
                            .Times(1)
                            .Expected();
            //When mapped build path doesn't exist
            CO_INSTRUCT_REF (OverrideInstance, ghc::filesystem, Mock_exists)
                            .WhenCalledWith<const ghc::filesystem::path&, CO_ANY_TYPE>
                            (
                                buildsDirPath + "/" + scriptsBuildsPaths.at(1), 
                                CO_ANY
                            )
                            .Returns<bool>(false)
                            .Times(1)
                            .Expected();
            
            static_assert(defaultMappingsCount == 2, "Update test");
        );
        
        ssTEST_OUTPUT_ASSERT(   "Initialize should succeed",
                                buildsManager->Initialize(), true);
        ssTEST_OUTPUT_ASSERT(   "First build mapping should exist",
                                BuildsManagerAccessor::GetMappings  (*buildsManager)
                                                                    .count(scriptsPaths.front()), 
                                0, 
                                >);
        ssTEST_OUTPUT_ASSERT(   "Second build mapping should not exist",
                                BuildsManagerAccessor::GetMappings  (*buildsManager)
                                                                    .count(scriptsPaths.at(1)), 
                                0);
        ssTEST_OUTPUT_ASSERT("", CO_GET_FAILED_FUNCTIONS(OverrideInstance).size(), 0);
        ssTEST_OUTPUT_VALUES_WHEN_FAILED("\n" + CO_GET_FAILED_REPORT(OverrideInstance));
    };

    ssTEST("Initialize Should Return False When Failed to Parse Mappings File")
    {
        //NOTE: Suppress error logs since we are overriding the mapping file content
        ssLOG_SET_CURRENT_THREAD_TARGET_LEVEL(ssLOG_LEVEL_FATAL);
        using namespace CppOverride;
        
        ssTEST_OUTPUT_SETUP
        (
            std::string mappingsContent = "Invalid content";
            prepareInitialization(mappingsContent);
        );
        
        ssTEST_OUTPUT_ASSERT("Initialize should failed", buildsManager->Initialize(), false);
        ssTEST_OUTPUT_ASSERT("", CO_GET_FAILED_FUNCTIONS(OverrideInstance).size(), 0);
        ssTEST_OUTPUT_VALUES_WHEN_FAILED("\n" + CO_GET_FAILED_REPORT(OverrideInstance));
    };

    auto commonInitializeBuildsManager = [&]()
    {
        using namespace CppOverride;
        ssTEST_OUTPUT_SETUP
        (
            //NOTE: This initializes the mapping for the first 2 scripts in scriptsPaths
            prepareInitialization("");
            for(int i = 0; i < defaultMappingsCount; ++i)
            {
                CO_INSTRUCT_REF (OverrideInstance, ghc::filesystem, Mock_exists)
                                .WhenCalledWith<const ghc::filesystem::path&, CO_ANY_TYPE>
                                    (buildsDirPath + "/" + scriptsBuildsPaths.at(i), CO_ANY)
                                .Times(1)
                                .Returns<bool>(true)
                                .Expected();
            }
            
            static_assert(defaultMappingsCount == 2, "Update test");
        );
        ssTEST_OUTPUT_ASSERT(   "Initialize should succeed",
                                buildsManager->Initialize(), true);
        ssTEST_OUTPUT_ASSERT(   "Mapped builds imported mappings file", 
                                BuildsManagerAccessor::GetMappings(*buildsManager).size() == 2);
    };

    ssTEST("CreateBuildMapping Should Create Build Mapping When Hashed Output Is Unique")
    {
        using namespace CppOverride;
        commonInitializeBuildsManager();
        ssTEST_OUTPUT_SETUP
        (
            //Hash script path
            CO_INSTRUCT_REF (OverrideInstance, Mock_std::Mock_hash<std::string>, operator())
                            .WhenCalledWith<std::string>(scriptsPaths.at(2) + "0")
                            .Returns<std::size_t>(15)
                            .Times(1)
                            .Expected();
            //New build mapping doesn't exist
            CO_INSTRUCT_REF (OverrideInstance, ghc::filesystem, Mock_exists)
                            .WhenCalledWith<const ghc::filesystem::path&, CO_ANY_TYPE>
                                (buildsDirPath + "/" + scriptsBuildsPaths.at(2), CO_ANY)
                            .Returns<bool>(false)
                            .Times(1)
                            .Expected();
            //Create new build mappings directory
            CO_INSTRUCT_REF (OverrideInstance, ghc::filesystem, Mock_create_directories)
                            .WhenCalledWith<const ghc::filesystem::path&, CO_ANY_TYPE>
                                (buildsDirPath + "/" + scriptsBuildsPaths.at(2), CO_ANY)
                            .Returns<bool>(true)
                            .Times(1)
                            .Expected();
        );
        ssTEST_OUTPUT_ASSERT(   "CreateBuildMapping should succeed",
                                buildsManager->CreateBuildMapping(scriptsPaths.at(2)), true);
        ssTEST_OUTPUT_ASSERT(   "New mapping exists",
                                BuildsManagerAccessor   ::GetMappings(*buildsManager)
                                                        .count(scriptsPaths.at(2)) == 1);
        ssTEST_OUTPUT_ASSERT(   "New mapping value is correct",
                                BuildsManagerAccessor::GetMappings  (*buildsManager)
                                                                    .at(scriptsPaths.at(2)),
                                scriptsBuildsPaths.at(2));
        ssTEST_OUTPUT_ASSERT("", CO_GET_FAILED_FUNCTIONS(OverrideInstance).size(), 0);
        ssTEST_OUTPUT_VALUES_WHEN_FAILED("\n" + CO_GET_FAILED_REPORT(OverrideInstance));
    };
    
    ssTEST("CreateBuildMapping Should Create Build Mapping When Hashed Output Is Not Unique")
    {
        using namespace CppOverride;
        commonInitializeBuildsManager();
        ssTEST_OUTPUT_SETUP
        (
            //Hash script path not unique (first attempt)
            CO_INSTRUCT_REF (OverrideInstance, Mock_std::Mock_hash<std::string>, operator())
                            .WhenCalledWith(scriptsPaths.at(2) + "0")
                            .Returns<std::size_t>(10)
                            .Times(1)
                            .Expected();
            //Hash script path unique (second attempt)
            CO_INSTRUCT_REF (OverrideInstance, Mock_std::Mock_hash<std::string>, operator())
                            .WhenCalledWith(scriptsPaths.at(2) + "1")
                            .Returns<std::size_t>(15)
                            .Times(1)
                            .Expected();
            CO_INSTRUCT_REF (OverrideInstance, ghc::filesystem, Mock_exists)
                            .WhenCalledWith<const ghc::filesystem::path&, CO_ANY_TYPE>
                                (buildsDirPath + "/" + scriptsBuildsPaths.at(2), CO_ANY)
                            .Returns<bool>(false);
            CO_INSTRUCT_REF (OverrideInstance, ghc::filesystem, Mock_create_directories)
                            .WhenCalledWith<const ghc::filesystem::path&, CO_ANY_TYPE>
                                (buildsDirPath + "/" + scriptsBuildsPaths.at(2), CO_ANY)
                            .Returns<bool>(true);
        );
        ssTEST_OUTPUT_ASSERT(   "CreateBuildMapping should succeed",
                                buildsManager->CreateBuildMapping(scriptsPaths.at(2)), true);
        ssTEST_OUTPUT_ASSERT(   "New mapping exists",
                                BuildsManagerAccessor::GetMappings  (*buildsManager)
                                                                    .count(scriptsPaths.at(2)), 
                                1);
        ssTEST_OUTPUT_ASSERT(   "New mapping value is correct",
                                BuildsManagerAccessor::GetMappings  (*buildsManager)
                                                                    .at(scriptsPaths.at(2)),
                                scriptsBuildsPaths.at(2));
        ssTEST_OUTPUT_ASSERT("", CO_GET_FAILED_FUNCTIONS(OverrideInstance).size(), 0);
        ssTEST_OUTPUT_VALUES_WHEN_FAILED("\n" + CO_GET_FAILED_REPORT(OverrideInstance));
    };
    
    ssTEST("CreateBuildMapping Should Return True When Mapping For Script Exists")
    {
        using namespace CppOverride;
        commonInitializeBuildsManager();
        ssTEST_OUTPUT_ASSERT(   "CreateBuildMapping should succeed",
                                buildsManager->CreateBuildMapping(scriptsPaths.at(1)), 
                                true);
        ssTEST_OUTPUT_ASSERT("", BuildsManagerAccessor::GetMappings(*buildsManager).size(), 2);
        ssTEST_OUTPUT_ASSERT("", CO_GET_FAILED_FUNCTIONS(OverrideInstance).size(), 0);
        ssTEST_OUTPUT_VALUES_WHEN_FAILED("\n" + CO_GET_FAILED_REPORT(OverrideInstance));
    };
    
    ssTEST("RemoveBuildMapping Should Remove Build Mapping When It Exists")
    {
        using namespace CppOverride;
        commonInitializeBuildsManager();
        ssTEST_OUTPUT_ASSERT(   "RemoveBuildMapping should succeed",
                                buildsManager->RemoveBuildMapping(scriptsPaths.at(1)), true);
        ssTEST_OUTPUT_ASSERT("", BuildsManagerAccessor::GetMappings(*buildsManager).size(), 1);
        if(BuildsManagerAccessor::GetMappings(*buildsManager).size() == 1)
        {
            ssTEST_OUTPUT_ASSERT(   "",
                                    BuildsManagerAccessor::GetMappings  (*buildsManager)
                                                                        .count(scriptsPaths.front()),
                                    1);
        }
        ssTEST_OUTPUT_ASSERT("", CO_GET_FAILED_FUNCTIONS(OverrideInstance).size(), 0);
        ssTEST_OUTPUT_VALUES_WHEN_FAILED("\n" + CO_GET_FAILED_REPORT(OverrideInstance));
    };
    
    ssTEST("RemoveBuildMapping Should Return True When It Doesn't Exists")
    {
        using namespace CppOverride;
        commonInitializeBuildsManager();
        ssTEST_OUTPUT_ASSERT(   "RemoveBuildMapping should succeed",
                                buildsManager->RemoveBuildMapping(scriptsPaths.at(2)), 
                                true);
        ssTEST_OUTPUT_ASSERT("", BuildsManagerAccessor::GetMappings(*buildsManager).size(), 2);
        ssTEST_OUTPUT_ASSERT("", CO_GET_FAILED_FUNCTIONS(OverrideInstance).size(), 0);
        ssTEST_OUTPUT_VALUES_WHEN_FAILED("\n" + CO_GET_FAILED_REPORT(OverrideInstance));
    };
    
    ssTEST("HasBuildMapping Should Return True Where It Exists")
    {
        using namespace CppOverride;
        commonInitializeBuildsManager();
        ssTEST_OUTPUT_ASSERT(   "HasBuildMapping should succeed",
                                buildsManager->HasBuildMapping(scriptsPaths.at(1)), 
                                true);
        ssTEST_OUTPUT_ASSERT("", CO_GET_FAILED_FUNCTIONS(OverrideInstance).size(), 0);
        ssTEST_OUTPUT_VALUES_WHEN_FAILED("\n" + CO_GET_FAILED_REPORT(OverrideInstance));
    };
    
    ssTEST("HasBuildMapping Should Return False Where It Doesn't Exist")
    {
        using namespace CppOverride;
        commonInitializeBuildsManager();
        ssTEST_OUTPUT_ASSERT(   "HasBuildMapping should Failed",
                                buildsManager->HasBuildMapping(scriptsPaths.at(2)), 
                                false);
        ssTEST_OUTPUT_ASSERT("", CO_GET_FAILED_FUNCTIONS(OverrideInstance).size(), 0);
        ssTEST_OUTPUT_VALUES_WHEN_FAILED("\n" + CO_GET_FAILED_REPORT(OverrideInstance));
    };
    
    ssTEST("GetBuildMapping Should Return Correct Mapping When It Exists")
    {
        using namespace CppOverride;
        commonInitializeBuildsManager();
        
        ssTEST_OUTPUT_SETUP
        (
            //CreateBuildMapping should not be called
            CO_INSTRUCT_REF (OverrideInstance, runcpp2::BuildsManager, CreateBuildMapping)
                            .Returns<bool>(false)
                            .MatchesObject(buildsManager.get())
                            .ExpectedNotSatisfied();
            ghc::filesystem::path outPath;
        );
        ssTEST_OUTPUT_ASSERT(   "GetBuildMapping should Succeed",
                                buildsManager->GetBuildMapping(scriptsPaths.at(0), outPath), 
                                true);
        ssTEST_OUTPUT_ASSERT(outPath == buildsDirPath + "/" + scriptsBuildsPaths.at(0));
        ssTEST_OUTPUT_ASSERT("", CO_GET_FAILED_FUNCTIONS(OverrideInstance).size(), 0);
        ssTEST_OUTPUT_VALUES_WHEN_FAILED("\n" + CO_GET_FAILED_REPORT(OverrideInstance));
    };
    
    ssTEST("GetBuildMapping Should Create Build Mapping When It Doesn't Exists")
    {
        using namespace CppOverride;
        commonInitializeBuildsManager();
        
        ssTEST_OUTPUT_SETUP
        (
            //CreateBuildMapping should be called
            CO_INSTRUCT_REF (OverrideInstance, runcpp2::BuildsManager, CreateBuildMapping)
                            .WhenCalledWith<const ghc::filesystem::path&>(scriptsPaths.at(2))
                            .Returns<bool>(true)
                            .Times(1)
                            .MatchesObject(buildsManager.get())
                            .WhenCalledExpectedly_Do
                            (
                                [&buildsManager, &scriptsPaths, &scriptsBuildsPaths](...)
                                {
                                    BuildsManagerAccessor::GetMappings(*buildsManager)
                                        [scriptsPaths.at(2)] = scriptsBuildsPaths.at(2);
                                    
                                    BuildsManagerAccessor::GetReverseMappings(*buildsManager)
                                        [scriptsBuildsPaths.at(2)] = scriptsPaths.at(2);
                                }
                            )
                            .Expected();
            ghc::filesystem::path outPath;
        );
        ssTEST_OUTPUT_ASSERT(   "GetBuildMapping should Succeed",
                                buildsManager->GetBuildMapping(scriptsPaths.at(2), outPath), 
                                true);
        ssTEST_OUTPUT_ASSERT(outPath == buildsDirPath + "/" + scriptsBuildsPaths.at(2));
        ssTEST_OUTPUT_ASSERT("", CO_GET_FAILED_FUNCTIONS(OverrideInstance).size(), 0);
        ssTEST_OUTPUT_VALUES_WHEN_FAILED("\n" + CO_GET_FAILED_REPORT(OverrideInstance));
    };
    
    ssTEST("RemoveAllBuildsMappings Should Remove All Build Mappings")
    {
        using namespace CppOverride;
        commonInitializeBuildsManager();
        ssTEST_OUTPUT_ASSERT(buildsManager->RemoveAllBuildsMappings() == true);
        ssTEST_OUTPUT_ASSERT(BuildsManagerAccessor::GetMappings(*buildsManager).empty());
        ssTEST_OUTPUT_ASSERT(BuildsManagerAccessor::GetReverseMappings(*buildsManager).empty());
        ssTEST_OUTPUT_ASSERT("", CO_GET_FAILED_FUNCTIONS(OverrideInstance).size(), 0);
        ssTEST_OUTPUT_VALUES_WHEN_FAILED("\n" + CO_GET_FAILED_REPORT(OverrideInstance));
    };
    
    ssTEST("SaveBuildsMappings Should Save Existing Mappings To Disk")
    {
        using namespace CppOverride;
        commonInitializeBuildsManager();
        
        //Adding new script
        ssTEST_OUTPUT_EXECUTION
        (
            BuildsManagerAccessor::GetMappings(*buildsManager)[scriptsPaths.at(2)] = 
                scriptsBuildsPaths.at(2);
            BuildsManagerAccessor::GetReverseMappings(*buildsManager)[scriptsBuildsPaths.at(2)] = 
                scriptsPaths.at(2);
        );
        ssTEST_OUTPUT_ASSERT("", BuildsManagerAccessor::GetMappings(*buildsManager).size(), 3);
        ssTEST_OUTPUT_ASSERT("", CO_GET_FAILED_FUNCTIONS(OverrideInstance).size(), 0);

        ssTEST_OUTPUT_SETUP
        (
            //Output to mappings file
            CO_INSTRUCT_NO_REF  (OverrideInstance, Mock_ofstream)
                                .WhenCalledWith<const ghc::filesystem::path&>(mappingsFilePath)
                                .Returns<void>()
                                .Times(1)
                                .Expected();
            //Removing previous override on is_open, checking if mappings file is created
            CO_REMOVE_INSTRUCT_REF(OverrideInstance, Mock_std::Mock_ofstream, is_open);
            CO_INSTRUCT_REF (OverrideInstance, Mock_std::Mock_ofstream, is_open)
                            .Returns<bool>(true)
                            .Times(1)
                            .Expected();
            //Closing mappings file
            std::string writeResult;
            CO_INSTRUCT_REF (OverrideInstance, Mock_std::Mock_ofstream, close)
                            .Returns<void>()
                            .Times(1)
                            .WhenCalledExpectedly_Do
                            (
                                [&](void* instance, const std::vector<CppOverride::TypedDataInfo>&)
                                {
                                    writeResult = 
                                        static_cast<Mock_std::Mock_ofstream*>(instance) ->StringStream
                                                                                        .str();
                                }
                            )
                            .Expected();
        );
        
        ssTEST_OUTPUT_ASSERT("", buildsManager->SaveBuildsMappings(), true);
        
        static_assert(defaultMappingsCount == 2, "Update test");
        constexpr int outputCount = 3;
        for(int i = 0; i < outputCount; ++i)
        {
            std::string currentMapping = scriptsPaths.at(i) + "," + scriptsBuildsPaths.at(i);
            ssTEST_OUTPUT_ASSERT(writeResult.find(currentMapping) != std::string::npos);
            ssTEST_OUTPUT_VALUES_WHEN_FAILED(writeResult, currentMapping);
        }
        
        //Count occurrence of newlines
        //credit: https://stackoverflow.com/a/8614196
        {
            int occurrences = 0;
            std::string::size_type start = 0;

            while ((start = writeResult.find("\n", start)) != std::string::npos)
            {
                ++occurrences;
                start += std::string("\n").length();
            }
            
            ssTEST_OUTPUT_ASSERT("Number of newlines must be n", occurrences, outputCount);
        }
        
        ssTEST_OUTPUT_ASSERT("", CO_GET_FAILED_FUNCTIONS(OverrideInstance).size(), 0);
        ssTEST_OUTPUT_VALUES_WHEN_FAILED("\n" + CO_GET_FAILED_REPORT(OverrideInstance));
    };
    
    
    ssTEST_END_TEST_GROUP();
    
    return 0;
}



