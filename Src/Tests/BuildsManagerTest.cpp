#include "runcpp2/BuildsManager.hpp"

#include "DSResult/DSResult.hpp"
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

DS::Result<void> TestMain()
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
    
    auto setup = [&buildsManager, &configDirPath]()
    {
        buildsManager.reset(new runcpp2::BuildsManager(configDirPath));
        CO_CLEAR_ALL_INSTRUCTS(OverrideInstance);
    };
    
    auto cleanup = []()
    {
        ssLOG_SET_CURRENT_THREAD_TARGET_LEVEL(ssLOG_LEVEL_DEBUG);
    };
    
    //Methods Should Return False When Not Initialized
    {
        setup();
        
        using namespace CppOverride;

        CO_INSTRUCT_REF (OverrideInstance, ghc::filesystem, Mock_create_directories)
                        .Returns<bool>(false)
                        .ExpectedNotSatisfied();
        ghc::filesystem::path outPath;
        
        DS_ASSERT_FALSE(buildsManager->CreateBuildMapping(scriptsPaths.front()));
        DS_ASSERT_FALSE(buildsManager->RemoveBuildMapping(scriptsPaths.front()));
        DS_ASSERT_FALSE(buildsManager->HasBuildMapping(scriptsPaths.front()));
        DS_ASSERT_FALSE(buildsManager->GetBuildMapping(scriptsPaths.front(), outPath));
        DS_ASSERT_FALSE(buildsManager->RemoveAllBuildsMappings());
        DS_ASSERT_FALSE(buildsManager->SaveBuildsMappings());
        DS_ASSERT_EQ(CO_GET_FAILED_FUNCTIONS(OverrideInstance).size(), 0);
        cleanup();
    }
    
    //Initialize Should Try To Read Mappings File When It Exists
    {
        setup();

        //NOTE: Suppress error logs since we are overriding the mapping file content
        ssLOG_SET_CURRENT_THREAD_TARGET_LEVEL(ssLOG_LEVEL_FATAL);
        using namespace CppOverride;
        
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
        
        buildsManager->Initialize();
        
        DS_ASSERT_EQ(CO_GET_FAILED_FUNCTIONS(OverrideInstance).size(), 0);
        cleanup();
    }
    
    //Initialize Should Create Mappings File When It Doesn't Exists
    {
        setup();
        
        using namespace CppOverride;
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
        
        DS_ASSERT_TRUE(buildsManager->Initialize());
        DS_ASSERT_EQ(CO_GET_FAILED_FUNCTIONS(OverrideInstance).size(), 0);
        
        cleanup();
    }
    
    //Initialize Should Parse Mappings File Correctly When It Exists
    {
        setup();
        
        using namespace CppOverride;
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
        
        DS_ASSERT_TRUE(buildsManager->Initialize());
        DS_ASSERT_GT(   BuildsManagerAccessor::GetMappings(*buildsManager).count(scriptsPaths.front()), 
                        0);
        DS_ASSERT_EQ(BuildsManagerAccessor::GetMappings(*buildsManager).count(scriptsPaths.at(1)), 0);
        DS_ASSERT_EQ(CO_GET_FAILED_FUNCTIONS(OverrideInstance).size(), 0);
    
        cleanup();
    }

    //Initialize Should Return False When Failed to Parse Mappings File
    {
        setup();
        
        //NOTE: Suppress error logs since we are overriding the mapping file content
        ssLOG_SET_CURRENT_THREAD_TARGET_LEVEL(ssLOG_LEVEL_FATAL);
        using namespace CppOverride;
        
        std::string mappingsContent = "Invalid content";
        prepareInitialization(mappingsContent);
        
        DS_ASSERT_FALSE(buildsManager->Initialize());
        DS_ASSERT_EQ(CO_GET_FAILED_FUNCTIONS(OverrideInstance).size(), 0);
        
        cleanup();
    };

    auto commonInitializeBuildsManager = [&]() -> DS::Result<void>
    {
        using namespace CppOverride;
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
        
        DS_ASSERT_TRUE(buildsManager->Initialize());
        DS_ASSERT_EQ(BuildsManagerAccessor::GetMappings(*buildsManager).size(), 2);
        
        return {};
    };

    //CreateBuildMapping Should Create Build Mapping When Hashed Output Is Unique
    {
        setup();
        
        using namespace CppOverride;
        commonInitializeBuildsManager().DS_TRY();
        
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
        
        DS_ASSERT_TRUE(buildsManager->CreateBuildMapping(scriptsPaths.at(2)));
        DS_ASSERT_EQ(BuildsManagerAccessor::GetMappings(*buildsManager).count(scriptsPaths.at(2)), 1);
        DS_ASSERT_EQ(   BuildsManagerAccessor::GetMappings(*buildsManager).at(scriptsPaths.at(2)),
                        scriptsBuildsPaths.at(2));
        DS_ASSERT_EQ(CO_GET_FAILED_FUNCTIONS(OverrideInstance).size(), 0);
        
        cleanup();
    }
    
    //CreateBuildMapping Should Create Build Mapping When Hashed Output Is Not Unique
    {
        setup();
        
        using namespace CppOverride;
        commonInitializeBuildsManager().DS_TRY();
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
        
        DS_ASSERT_TRUE(buildsManager->CreateBuildMapping(scriptsPaths.at(2)));
        DS_ASSERT_EQ(BuildsManagerAccessor::GetMappings(*buildsManager).count(scriptsPaths.at(2)), 1);
        DS_ASSERT_EQ(   BuildsManagerAccessor::GetMappings(*buildsManager).at(scriptsPaths.at(2)),
                        scriptsBuildsPaths.at(2));
        DS_ASSERT_EQ(CO_GET_FAILED_FUNCTIONS(OverrideInstance).size(), 0);
    
        cleanup();
    }
    
    //CreateBuildMapping Should Return True When Mapping For Script Exists
    {
        setup();
        
        using namespace CppOverride;
        commonInitializeBuildsManager().DS_TRY();
        DS_ASSERT_TRUE(buildsManager->CreateBuildMapping(scriptsPaths.at(1)));
        DS_ASSERT_EQ(BuildsManagerAccessor::GetMappings(*buildsManager).size(), 2);
        DS_ASSERT_EQ(CO_GET_FAILED_FUNCTIONS(OverrideInstance).size(), 0);
    
        cleanup();
    }
    
    //RemoveBuildMapping Should Remove Build Mapping When It Exists
    {
        setup();
        
        using namespace CppOverride;
        commonInitializeBuildsManager().DS_TRY();
        DS_ASSERT_TRUE(buildsManager->RemoveBuildMapping(scriptsPaths.at(1)));
        DS_ASSERT_EQ(BuildsManagerAccessor::GetMappings(*buildsManager).size(), 1);
        
        if(BuildsManagerAccessor::GetMappings(*buildsManager).size() == 1)
        {
            DS_ASSERT_EQ(   BuildsManagerAccessor::GetMappings  (*buildsManager)
                                                                .count(scriptsPaths.front()),
                            1);
        }
        DS_ASSERT_EQ(CO_GET_FAILED_FUNCTIONS(OverrideInstance).size(), 0);
        
        cleanup();
    }
    
    //RemoveBuildMapping Should Return True When It Doesn't Exists
    {
        setup();
        
        using namespace CppOverride;
        commonInitializeBuildsManager().DS_TRY();
        DS_ASSERT_TRUE(buildsManager->RemoveBuildMapping(scriptsPaths.at(2)));
        DS_ASSERT_EQ(BuildsManagerAccessor::GetMappings(*buildsManager).size(), 2);
        DS_ASSERT_EQ(CO_GET_FAILED_FUNCTIONS(OverrideInstance).size(), 0);
    
        cleanup();
    }
    
    //HasBuildMapping Should Return True Where It Exists
    {
        setup();
        
        using namespace CppOverride;
        commonInitializeBuildsManager().DS_TRY();
        DS_ASSERT_TRUE(buildsManager->HasBuildMapping(scriptsPaths.at(1)));
        DS_ASSERT_EQ(CO_GET_FAILED_FUNCTIONS(OverrideInstance).size(), 0);
        
        cleanup();
    }
    
    //HasBuildMapping Should Return False Where It Doesn't Exist
    {
        setup();
        
        using namespace CppOverride;
        commonInitializeBuildsManager();
        DS_ASSERT_FALSE(buildsManager->HasBuildMapping(scriptsPaths.at(2)));
        DS_ASSERT_EQ(CO_GET_FAILED_FUNCTIONS(OverrideInstance).size(), 0);
        
        cleanup();
    }
    
    //GetBuildMapping Should Return Correct Mapping When It Exists
    {
        setup();
        
        using namespace CppOverride;
        commonInitializeBuildsManager().DS_TRY();
        
        //CreateBuildMapping should not be called
        CO_INSTRUCT_REF (OverrideInstance, runcpp2::BuildsManager, CreateBuildMapping)
                        .Returns<bool>(false)
                        .MatchesObject(buildsManager.get())
                        .ExpectedNotSatisfied();
        ghc::filesystem::path outPath;
        
        DS_ASSERT_TRUE(buildsManager->GetBuildMapping(scriptsPaths.at(0), outPath));
        DS_ASSERT_EQ(outPath, buildsDirPath + "/" + scriptsBuildsPaths.at(0));
        DS_ASSERT_EQ(CO_GET_FAILED_FUNCTIONS(OverrideInstance).size(), 0);
        
        cleanup();
    }
    
    //GetBuildMapping Should Create Build Mapping When It Doesn't Exists
    {
        setup();
        
        using namespace CppOverride;
        commonInitializeBuildsManager().DS_TRY();
        
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
        
        DS_ASSERT_TRUE(buildsManager->GetBuildMapping(scriptsPaths.at(2), outPath));
        
        DS_ASSERT_EQ(outPath, buildsDirPath + "/" + scriptsBuildsPaths.at(2));
        DS_ASSERT_EQ(CO_GET_FAILED_FUNCTIONS(OverrideInstance).size(), 0);
        
        cleanup();
    }
    
    //RemoveAllBuildsMappings Should Remove All Build Mappings
    {
        setup();
        
        using namespace CppOverride;
        commonInitializeBuildsManager().DS_TRY();
        
        DS_ASSERT_TRUE(buildsManager->RemoveAllBuildsMappings());
        DS_ASSERT_TRUE(BuildsManagerAccessor::GetMappings(*buildsManager).empty());
        DS_ASSERT_TRUE(BuildsManagerAccessor::GetReverseMappings(*buildsManager).empty());
        DS_ASSERT_EQ(CO_GET_FAILED_FUNCTIONS(OverrideInstance).size(), 0);
        
        cleanup();
    }
    
    //SaveBuildsMappings Should Save Existing Mappings To Disk
    {
        setup();
        
        using namespace CppOverride;
        commonInitializeBuildsManager().DS_TRY();
        
        //Adding new script
        BuildsManagerAccessor::GetMappings(*buildsManager)[scriptsPaths.at(2)] = 
            scriptsBuildsPaths.at(2);
        BuildsManagerAccessor::GetReverseMappings(*buildsManager)[scriptsBuildsPaths.at(2)] = 
            scriptsPaths.at(2);
        DS_ASSERT_EQ(BuildsManagerAccessor::GetMappings(*buildsManager).size(), 3);
        DS_ASSERT_EQ(CO_GET_FAILED_FUNCTIONS(OverrideInstance).size(), 0);

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
        
        DS_ASSERT_TRUE(buildsManager->SaveBuildsMappings());
        
        static_assert(defaultMappingsCount == 2, "Update test");
        constexpr int outputCount = 3;
        for(int i = 0; i < outputCount; ++i)
        {
            std::string currentMapping = scriptsPaths.at(i) + "," + scriptsBuildsPaths.at(i);
            DS_ASSERT_NOT_EQ(writeResult.find(currentMapping), std::string::npos);
            //ssTEST_OUTPUT_VALUES_WHEN_FAILED(writeResult, currentMapping);
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
            
            DS_ASSERT_EQ(occurrences, outputCount);
        }
        
        DS_ASSERT_EQ(CO_GET_FAILED_FUNCTIONS(OverrideInstance).size(), 0);
    
        cleanup();
    }
    
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
