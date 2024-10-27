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

class BuildsManagerAccessor
{
    public:
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
        [  
            &mappingsFilePath,
            &scriptsPaths,
            &scriptsBuildsPaths
        ]
        (
            std::shared_ptr<CppOverride::OverrideResult>& returnMappingsPathExistsResult,
            std::shared_ptr<CppOverride::OverrideResult>& returnIfstreamResult,
            std::shared_ptr<CppOverride::OverrideResult>& returnIsOpenResult,
            std::shared_ptr<CppOverride::OverrideResult>& returnRdbufResult,
            std::string customContent
        )
        {
            //Checking mappings file path
            returnMappingsPathExistsResult = CppOverride::CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, Mock_exists)
                                .WhenCalledWith<const ghc::filesystem::path&, 
                                                CO_ANY_TYPE>(mappingsFilePath, CO_ANY)
                                .Returns<bool>(true)
                                .Times(1)
                                .AssignResult(returnMappingsPathExistsResult);
            //Open mappings file
            returnIfstreamResult = CppOverride::CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, Mock_ifstream)
                                .WhenCalledWith<const ghc::filesystem::path&>
                                    (mappingsFilePath)
                                .Returns<void>()
                                .Times(1)
                                .AssignResult(returnIfstreamResult);
            //Checking if mappings file is opened
            returnIsOpenResult = CppOverride::CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, is_open)
                                .Returns<bool>(true)
                                .Times(1)
                                .AssignResult(returnIsOpenResult);
            //Return file content
            std::string mappingsContent;
            if(!customContent.empty())
                mappingsContent = customContent;
            else
            {
                for(int i = 0; i < defaultMappingsCount; ++i)
                    mappingsContent += scriptsPaths.at(i) + "," + scriptsBuildsPaths.at(i) + "\n";
            }
            
            returnRdbufResult = CppOverride::CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, rdbuf)
                                .Returns<std::string>(mappingsContent)
                                .Times(1)
                                .AssignResult(returnRdbufResult);
        };
    
    ssTEST_INIT_TEST_GROUP();
    
    ssTEST_COMMON_SETUP
    {
        buildsManager.reset(new runcpp2::BuildsManager(configDirPath));
        CO_CLEAR_ALL_OVERRIDE_SETUP(OverrideInstance);
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
            std::shared_ptr<OverrideResult> result = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, Mock_create_directories)
                                .Times(1)
                                .Returns<bool>(false)
                                .AssignResult(result);
            ghc::filesystem::path outPath;
        );
        ssTEST_OUTPUT_ASSERT(buildsManager->CreateBuildMapping(scriptsPaths.front()) == false);
        ssTEST_OUTPUT_ASSERT(result->GetStatusCount() == 0);
        ssTEST_OUTPUT_ASSERT(buildsManager->RemoveBuildMapping(scriptsPaths.front()) == false);
        ssTEST_OUTPUT_ASSERT(buildsManager->HasBuildMapping(scriptsPaths.front()) == false);
        ssTEST_OUTPUT_ASSERT(buildsManager->GetBuildMapping(scriptsPaths.front(), outPath) == false);
        ssTEST_OUTPUT_ASSERT(buildsManager->RemoveAllBuildsMappings() == false);
        ssTEST_OUTPUT_ASSERT(buildsManager->SaveBuildsMappings() == false);
    };
    
    ssTEST("Initialize Should Try To Read Mappings File When It Exists")
    {
        //NOTE: Suppress error logs since we are overriding the mapping file content
        ssLOG_SET_CURRENT_THREAD_TARGET_LEVEL(ssLOG_LEVEL_FATAL);
        using namespace CppOverride;
        
        ssTEST_OUTPUT_SETUP
        (
            //Checking mappings file path
            std::shared_ptr<OverrideResult> pathExistsResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, Mock_exists)
                                .WhenCalledWith<const ghc::filesystem::path&, 
                                                CO_ANY_TYPE>(mappingsFilePath, CO_ANY)
                                .Returns<bool>(true)
                                .Times(1)
                                .AssignResult(pathExistsResult);
            //Open mappings file
            std::shared_ptr<OverrideResult> ifstreamResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, Mock_ifstream)
                                .WhenCalledWith<const ghc::filesystem::path&>(mappingsFilePath)
                                .Returns<void>()
                                .Times(1)
                                .AssignResult(ifstreamResult);
        );
        ssTEST_OUTPUT_EXECUTION( buildsManager->Initialize(); );
        ssTEST_OUTPUT_ASSERT("Checking mappings file path", pathExistsResult->LastStatusSucceed());
        ssTEST_OUTPUT_ASSERT("Open mappings file", ifstreamResult->LastStatusSucceed());
    };
    
    ssTEST("Initialize Should Create Mappings File When It Doesn't Exists")
    {
        using namespace CppOverride;
        ssTEST_OUTPUT_SETUP
        (
            //Checking mappings file path
            std::shared_ptr<OverrideResult> mappingsPathExistsResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, Mock_exists)
                                .WhenCalledWith<const ghc::filesystem::path&, 
                                                CO_ANY_TYPE>(mappingsFilePath, CO_ANY)
                                .Returns<bool>(false)
                                .Times(1)
                                .AssignResult(mappingsPathExistsResult);
            //Checking builds directory exist
            std::shared_ptr<OverrideResult> buildsDirExistsResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, Mock_exists)
                                .WhenCalledWith<const ghc::filesystem::path&, 
                                                CO_ANY_TYPE>(buildsDirPath, CO_ANY)
                                .Returns<bool>(false)
                                .Times(1)
                                .AssignResult(buildsDirExistsResult);
            //Create builds directory
            std::shared_ptr<OverrideResult> createBuildsDirResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, Mock_create_directories)
                                .WhenCalledWith<const ghc::filesystem::path&, 
                                                CO_ANY_TYPE>(buildsDirPath, CO_ANY)
                                .Returns<bool>(true)
                                .Times(1)
                                .AssignResult(createBuildsDirResult);
            //Creating mappings file
            std::shared_ptr<OverrideResult> ofstreamResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, Mock_ofstream)
                                .WhenCalledWith<const ghc::filesystem::path&>(mappingsFilePath)
                                .Returns<void>()
                                .Times(1)
                                .AssignResult(ofstreamResult);
            //Checking if mappings file is created
            std::shared_ptr<OverrideResult> ofstreamIsOpenResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, is_open)
                                .Returns<bool>(true)
                                .Times(1)
                                .AssignResult(ofstreamIsOpenResult);
            //Closing mappings file
            std::shared_ptr<OverrideResult> ofstreamCloseResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, close)
                                .Returns<void>()
                                .Times(1)
                                .AssignResult(ofstreamCloseResult);
        );
        ssTEST_OUTPUT_ASSERT(   "Initialize should succeed", buildsManager->Initialize(), true);
        ssTEST_OUTPUT_ASSERT(   "Checking mappings file path",
                                mappingsPathExistsResult->GetSucceedCount(), 1);
        ssTEST_OUTPUT_ASSERT(   "Checking builds directory exist",
                                buildsDirExistsResult->LastStatusSucceed());
        ssTEST_OUTPUT_ASSERT(   "Create builds directory",
                                createBuildsDirResult->LastStatusSucceed());
        ssTEST_OUTPUT_ASSERT(   "Creating mappings file",
                                ofstreamResult->LastStatusSucceed());
        ssTEST_OUTPUT_ASSERT(   "Checking if mappings file is created",
                                ofstreamIsOpenResult->LastStatusSucceed());
        ssTEST_OUTPUT_ASSERT(   "Closing mappings file",
                                ofstreamCloseResult->LastStatusSucceed());
    };
    
    ssTEST("Initialize Should Parse Mappings File Correctly When It Exists")
    {
        using namespace CppOverride;
        ssTEST_OUTPUT_SETUP
        (
            std::shared_ptr<OverrideResult> mappingsPathExistsResult = nullptr;
            std::shared_ptr<OverrideResult> ifstreamResult = nullptr;
            std::shared_ptr<OverrideResult> isOpenResult = nullptr;
            std::shared_ptr<OverrideResult> rdbufResult = nullptr;
            prepareInitialization(  mappingsPathExistsResult, ifstreamResult, 
                                    isOpenResult, rdbufResult, "");
            
            //When mapped build path exists
            std::shared_ptr<OverrideResult> mappedBuildPathExistsResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, Mock_exists)
                                .WhenCalledWith<const ghc::filesystem::path&, CO_ANY_TYPE>
                                    (buildsDirPath + "/" + scriptsBuildsPaths.front() , CO_ANY)
                                .Returns<bool>(true)
                                .Times(1)
                                .AssignResult(mappedBuildPathExistsResult);
            //When mapped build path doesn't exist
            std::shared_ptr<OverrideResult> mappedBuildPathNotExistsResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, Mock_exists)
                                .WhenCalledWith<const ghc::filesystem::path&, CO_ANY_TYPE>
                                    (buildsDirPath + "/" + scriptsBuildsPaths.at(1) , CO_ANY)
                                .Returns<bool>(false)
                                .Times(1)
                                .AssignResult(mappedBuildPathNotExistsResult);
            
            static_assert(defaultMappingsCount == 2, "Update test");
        );
        
        ssTEST_OUTPUT_ASSERT(   "Initialize should succeed",
                                buildsManager->Initialize(), true);
        ssTEST_OUTPUT_ASSERT(   "Checking mappings file path",
                                mappingsPathExistsResult->GetSucceedCount(), 1);
        ssTEST_OUTPUT_ASSERT(   "Open mappings file",
                                ifstreamResult->LastStatusSucceed());
        ssTEST_OUTPUT_ASSERT(   "Checking if mappings file is opened",
                                isOpenResult->LastStatusSucceed());
        ssTEST_OUTPUT_ASSERT(   "Return file content",
                                rdbufResult->LastStatusSucceed());
        ssTEST_OUTPUT_ASSERT(   "When mapped build path exists",
                                mappedBuildPathExistsResult->GetSucceedCount(), 1);
        ssTEST_OUTPUT_ASSERT(   "When mapped build path doesn't exist",
                                mappedBuildPathNotExistsResult->GetSucceedCount(), 1);
        
        ssTEST_OUTPUT_ASSERT(   "First build mapping should exist",
                                BuildsManagerAccessor   ::GetMappings(*buildsManager)
                                                        .count(scriptsPaths.front()) > 0);
        ssTEST_OUTPUT_ASSERT(   "Second build mapping should not exist",
                                BuildsManagerAccessor   ::GetMappings(*buildsManager)
                                                        .count(scriptsPaths.at(1)) == 0);
    };

    ssTEST("Initialize Should Return False When Failed to Parse Mappings File")
    {
        //NOTE: Suppress error logs since we are overriding the mapping file content
        ssLOG_SET_CURRENT_THREAD_TARGET_LEVEL(ssLOG_LEVEL_FATAL);
        using namespace CppOverride;
        
        ssTEST_OUTPUT_SETUP
        (
            std::shared_ptr<OverrideResult> mappingsPathExistsResult = nullptr;
            std::shared_ptr<OverrideResult> ifstreamResult = nullptr;
            std::shared_ptr<OverrideResult> isOpenResult = nullptr;
            std::shared_ptr<OverrideResult> rdbufResult = nullptr;
            
            std::string mappingsContent = "Invalid content";
            prepareInitialization(  mappingsPathExistsResult, ifstreamResult, 
                                    isOpenResult, rdbufResult, mappingsContent);
        );
        
        ssTEST_OUTPUT_ASSERT(   "Initialize should failed", buildsManager->Initialize(), false);
        ssTEST_OUTPUT_ASSERT(   "Checking mappings file path",
                                mappingsPathExistsResult->GetSucceedCount(), 1);
        ssTEST_OUTPUT_ASSERT(   "Open mappings file",
                                ifstreamResult->LastStatusSucceed());
        ssTEST_OUTPUT_ASSERT(   "Checking if mappings file is opened",
                                isOpenResult->LastStatusSucceed());
        ssTEST_OUTPUT_ASSERT(   "Return file content",
                                rdbufResult->LastStatusSucceed());
    };

    auto commonInitializeBuildsManager = [&]()
    {
        using namespace CppOverride;
        ssTEST_OUTPUT_SETUP
        (
            std::vector<std::shared_ptr<OverrideResult>> initializeResults(4, nullptr);
            
            //NOTE: This initializes the mapping for the first 2 scripts in scriptsPaths
            prepareInitialization(  initializeResults.at(0), initializeResults.at(1), 
                                    initializeResults.at(2), initializeResults.at(3), "");
            CO_SETUP_OVERRIDE   (OverrideInstance, Mock_exists)
                                .Times(2)
                                .Returns<bool>(true);
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
            std::shared_ptr<OverrideResult> hashResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, operator())
                                .WhenCalledWith<std::string>(scriptsPaths.at(2) + "0")
                                .Returns<std::size_t>(15)
                                .Times(1)
                                .AssignResult(hashResult);
            
            //New build mapping doesn't exist
            std::shared_ptr<OverrideResult> newMappedBuildPathExistsResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, Mock_exists)
                                .WhenCalledWith<const ghc::filesystem::path&, CO_ANY_TYPE>
                                    (buildsDirPath + "/" + scriptsBuildsPaths.at(2), CO_ANY)
                                .Returns<bool>(false)
                                .Times(1)
                                .AssignResult(newMappedBuildPathExistsResult);
            //Create new build mappings directory
            std::shared_ptr<OverrideResult> createNewMappedBuildPathResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, Mock_create_directories)
                                .WhenCalledWith<const ghc::filesystem::path&, CO_ANY_TYPE>
                                    (buildsDirPath + "/" + scriptsBuildsPaths.at(2), CO_ANY)
                                .Returns<bool>(true)
                                .Times(1)
                                .AssignResult(createNewMappedBuildPathResult);
        );
        ssTEST_OUTPUT_ASSERT(   "CreateBuildMapping should succeed",
                                buildsManager->CreateBuildMapping(scriptsPaths.at(2)), true);
        ssTEST_OUTPUT_ASSERT(   "Hash script path", hashResult->LastStatusSucceed());
        ssLOG_DEBUG("hashResult->GetStatusCount(): " << hashResult->GetStatusCount());
        
        ssTEST_OUTPUT_ASSERT(   "New build mapping doesn't exist", 
                                newMappedBuildPathExistsResult->LastStatusSucceed());
        ssTEST_OUTPUT_ASSERT(   "Create new build mappings directory", 
                                createNewMappedBuildPathResult->LastStatusSucceed());
        ssTEST_OUTPUT_ASSERT(   "New mapping exists",
                                BuildsManagerAccessor   ::GetMappings(*buildsManager)
                                                        .count(scriptsPaths.at(2)) == 1);
        ssTEST_OUTPUT_ASSERT(   "New mapping value is correct",
                                BuildsManagerAccessor   ::GetMappings(*buildsManager)
                                                        .at(scriptsPaths.at(2)) == scriptsBuildsPaths.at(2));
    };
    
    ssTEST("CreateBuildMapping Should Create Build Mapping When Hashed Output Is Not Unique")
    {
        using namespace CppOverride;
        commonInitializeBuildsManager();
        ssTEST_OUTPUT_SETUP
        (
            //Hash script path not unique (first attempt)
            std::shared_ptr<OverrideResult> hashNotUniqueResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, operator())
                                .WhenCalledWith(scriptsPaths.at(2) + "0")
                                .Returns<std::size_t>(10)
                                .Times(1)
                                .AssignResult(hashNotUniqueResult);
            //Hash script path unique (second attempt)
            std::shared_ptr<OverrideResult> hashUniqueResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, operator())
                                .WhenCalledWith(scriptsPaths.at(2) + "1")
                                .Returns<std::size_t>(15)
                                .Times(1)
                                .AssignResult(hashUniqueResult);
            CO_SETUP_OVERRIDE   (OverrideInstance, Mock_exists)
                                .WhenCalledWith<const ghc::filesystem::path&, CO_ANY_TYPE>
                                    (buildsDirPath + "/" + scriptsBuildsPaths.at(2), CO_ANY)
                                .Returns<bool>(false);
            CO_SETUP_OVERRIDE   (OverrideInstance, Mock_create_directories)
                                .WhenCalledWith<const ghc::filesystem::path&, CO_ANY_TYPE>
                                    (buildsDirPath + "/" + scriptsBuildsPaths.at(2), CO_ANY)
                                .Returns<bool>(true);
        );
        ssTEST_OUTPUT_ASSERT(   "CreateBuildMapping should succeed",
                                buildsManager->CreateBuildMapping(scriptsPaths.at(2)), true);
        ssTEST_OUTPUT_ASSERT(   "Hash script path not unique (first attempt)",
                                hashNotUniqueResult->GetSucceedCount() == 1);
        ssTEST_OUTPUT_ASSERT(   "Hash script path unique (second attempt)",
                                hashUniqueResult->LastStatusSucceed());
        ssTEST_OUTPUT_ASSERT(   "New mapping exists",
                                BuildsManagerAccessor   ::GetMappings(*buildsManager)
                                                        .count(scriptsPaths.at(2)) == 1);
        ssTEST_OUTPUT_ASSERT(   "New mapping value is correct",
                                BuildsManagerAccessor   ::GetMappings(*buildsManager)
                                                        .at(scriptsPaths.at(2)) == scriptsBuildsPaths.at(2));
    };
    
    ssTEST("CreateBuildMapping Should Return True When Mapping For Script Exists")
    {
        using namespace CppOverride;
        commonInitializeBuildsManager();
        ssTEST_OUTPUT_ASSERT(   "CreateBuildMapping should succeed",
                                buildsManager->CreateBuildMapping(scriptsPaths.at(1)), true);
        ssTEST_OUTPUT_ASSERT(BuildsManagerAccessor::GetMappings(*buildsManager).size() == 2);
    };
    
    ssTEST("RemoveBuildMapping Should Remove Build Mapping When It Exists")
    {
        using namespace CppOverride;
        commonInitializeBuildsManager();
        ssTEST_OUTPUT_ASSERT(   "RemoveBuildMapping should succeed",
                                buildsManager->RemoveBuildMapping(scriptsPaths.at(1)), true);
        ssTEST_OUTPUT_ASSERT(BuildsManagerAccessor::GetMappings(*buildsManager).size() == 1);
        if(BuildsManagerAccessor::GetMappings(*buildsManager).size() == 1)
        {
            ssTEST_OUTPUT_ASSERT(   BuildsManagerAccessor   ::GetMappings(*buildsManager)
                                                            .count(scriptsPaths.front()) == 1);
        }
    };
    
    ssTEST("RemoveBuildMapping Should Return True When It Doesn't Exists")
    {
        using namespace CppOverride;
        commonInitializeBuildsManager();
        ssTEST_OUTPUT_ASSERT(   "RemoveBuildMapping should succeed",
                                buildsManager->RemoveBuildMapping(scriptsPaths.at(2)), true);
        ssTEST_OUTPUT_ASSERT(BuildsManagerAccessor::GetMappings(*buildsManager).size() == 2);
    };
    
    ssTEST("HasBuildMapping Should Return True Where It Exists")
    {
        using namespace CppOverride;
        commonInitializeBuildsManager();
        ssTEST_OUTPUT_ASSERT(   "HasBuildMapping should succeed",
                                buildsManager->HasBuildMapping(scriptsPaths.at(1)), true);
    };
    
    ssTEST("HasBuildMapping Should Return False Where It Doesn't Exist")
    {
        using namespace CppOverride;
        commonInitializeBuildsManager();
        ssTEST_OUTPUT_ASSERT(   "HasBuildMapping should Failed",
                                buildsManager->HasBuildMapping(scriptsPaths.at(2)), false);
    };
    
    ssTEST("GetBuildMapping Should Return Correct Mapping When It Exists")
    {
        using namespace CppOverride;
        commonInitializeBuildsManager();
        
        ssTEST_OUTPUT_SETUP
        (
            //CreateBuildMapping should not be called
            std::shared_ptr<OverrideResult> createBuildMappingResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, CreateBuildMapping)
                                .Returns<bool>(false)
                                .OverrideObject(buildsManager.get())
                                .AssignResult(createBuildMappingResult);
            ghc::filesystem::path outPath;
        );
        ssTEST_OUTPUT_ASSERT(   "GetBuildMapping should Succeed",
                                buildsManager->GetBuildMapping(scriptsPaths.at(0), outPath), true);
        ssTEST_OUTPUT_ASSERT(   "CreateBuildMapping should not be called", 
                                createBuildMappingResult->GetStatusCount() == 0);
        ssTEST_OUTPUT_ASSERT(   outPath == buildsDirPath + "/" + scriptsBuildsPaths.at(0));
    };
    
    ssTEST("GetBuildMapping Should Create Build Mapping When It Doesn't Exists")
    {
        using namespace CppOverride;
        commonInitializeBuildsManager();
        
        ssTEST_OUTPUT_SETUP
        (
            //CreateBuildMapping should be called
            std::shared_ptr<OverrideResult> createBuildMappingResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, CreateBuildMapping)
                                .WhenCalledWith<const ghc::filesystem::path&>(scriptsPaths.at(2))
                                .Returns<bool>(true)
                                .Times(1)
                                .OverrideObject(buildsManager.get())
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
                                .AssignResult(createBuildMappingResult);
            ghc::filesystem::path outPath;
        );
        ssTEST_OUTPUT_ASSERT(   "GetBuildMapping should Succeed",
                                buildsManager->GetBuildMapping(scriptsPaths.at(2), outPath), true);
        ssTEST_OUTPUT_ASSERT(   "CreateBuildMapping should be called", 
                                createBuildMappingResult->LastStatusSucceed());
        ssTEST_OUTPUT_ASSERT(   outPath == buildsDirPath + "/" + scriptsBuildsPaths.at(2));
    };
    
    ssTEST("RemoveAllBuildsMappings Should Remove All Build Mappings")
    {
        using namespace CppOverride;
        commonInitializeBuildsManager();
        ssTEST_OUTPUT_ASSERT(buildsManager->RemoveAllBuildsMappings() == true);
        ssTEST_OUTPUT_ASSERT(BuildsManagerAccessor::GetMappings(*buildsManager).empty());
        ssTEST_OUTPUT_ASSERT(BuildsManagerAccessor::GetReverseMappings(*buildsManager).empty());
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
        ssTEST_OUTPUT_ASSERT(BuildsManagerAccessor::GetMappings(*buildsManager).size() == 3);

        ssTEST_OUTPUT_SETUP
        (
            //Output to mappings file
            std::shared_ptr<OverrideResult> ofstreamResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, Mock_ofstream)
                                .WhenCalledWith<const ghc::filesystem::path&>(mappingsFilePath)
                                .Returns<void>()
                                .Times(1)
                                .AssignResult(ofstreamResult);
            //Checking if mappings file is created
            std::shared_ptr<OverrideResult> ofstreamIsOpenResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, is_open)
                                .Returns<bool>(true)
                                .Times(1)
                                .AssignResult(ofstreamIsOpenResult);
            //Closing mappings file
            std::shared_ptr<OverrideResult> ofstreamCloseResult = CreateOverrideResult();
            std::string writeResult;
            CO_SETUP_OVERRIDE   (OverrideInstance, close)
                                .Returns<void>()
                                .Times(1)
                                .WhenCalledExpectedly_Do
                                (
                                    [&](void* instance, const std::vector<void*>&)
                                    {
                                        writeResult = 
                                            static_cast<std::Mock_ofstream*>(instance)->
                                            StringStream.str();
                                    }
                                )
                                .AssignResult(ofstreamCloseResult);
        );
        
        ssTEST_OUTPUT_ASSERT(   buildsManager->SaveBuildsMappings() == true);
        ssTEST_OUTPUT_ASSERT(   "Output to mappings file",
                                ofstreamResult->LastStatusSucceed());
        ssTEST_OUTPUT_ASSERT(   "Checking if mappings file is created",
                                ofstreamIsOpenResult->LastStatusSucceed());
        ssTEST_OUTPUT_ASSERT(   "Closing mappings file",
                                ofstreamCloseResult->LastStatusSucceed());
        
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
    };
    
    
    ssTEST_END_TEST_GROUP();
    
    return 0;
}



