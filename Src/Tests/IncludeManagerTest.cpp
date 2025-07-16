#define private public
#include "runcpp2/IncludeManager.hpp"
#undef private

#include "ssTest.hpp"
#include "CppOverride.hpp"
#include "ssLogger/ssLog.hpp"

CO_DECLARE_INSTANCE(OverrideInstance);

#define INTERNAL_RUNCPP2_UNDEF_MOCKS 1
#include "Tests/IncludeManager/MockComponents.hpp"

#include <memory>

#if !INTERNAL_RUNCPP2_UNIT_TESTS || !defined(INTERNAL_RUNCPP2_UNIT_TESTS)
    static_assert(false, "INTERNAL_RUNCPP2_UNIT_TESTS not defined");
#endif

class IncludeManagerAccessor
{
    public:
        inline static const ghc::filesystem::path& 
        GetIncludeRecordDir(const runcpp2::IncludeManager& includeManager)
        {
            return includeManager.IncludeRecordDir;
        }
        
        inline static ghc::filesystem::path 
        GetRecordPath(  const runcpp2::IncludeManager& includeManager, 
                        const ghc::filesystem::path& sourceFile)
        {
            return includeManager.GetRecordPath(sourceFile);
        }
};

int main(int argc, char** argv)
{
    #if defined(_WIN32)
        const std::string absPathPrefix = "C:";
    #elif defined(__unix__) || defined(__APPLE__)
        const std::string absPathPrefix;
    #endif
    
    std::unique_ptr<runcpp2::IncludeManager> includeManager(nullptr);
    const std::string buildDirPath = absPathPrefix + "/tmp/Build";
    const std::string includeMapsPath = buildDirPath + "/IncludeMaps";
    
    std::vector<std::string> sourcePaths = 
    {
        absPathPrefix + "/tmp/Source/test.cpp",
        absPathPrefix + "/tmp/Source/test2.cpp",
        absPathPrefix + "/tmp/Source/test3.cpp"
    };
    
    std::vector<std::string> includePaths = 
    {
        absPathPrefix + "/tmp/Include/header1.hpp",
        absPathPrefix + "/tmp/Include/header2.hpp",
        absPathPrefix + "/tmp/Include/header3.hpp"
    };
    
    ssTEST_INIT_TEST_GROUP();
    ssTEST_PARSE_ARGS(argc, argv);
    
    ssTEST_COMMON_SETUP
    {
        includeManager.reset(new runcpp2::IncludeManager());
        CO_CLEAR_ALL_INSTRUCTS(OverrideInstance);
    };
    
    ssTEST_COMMON_CLEANUP
    {
        ssLOG_SET_CURRENT_THREAD_TARGET_LEVEL(ssLOG_LEVEL_DEBUG);
    };

    ssTEST("Initialize Should Create Directory When It Doesn't Exist")
    {
        using namespace CppOverride;
        ssTEST_OUTPUT_SETUP
        (
            //Check if directory exists
            CO_INSTRUCT_REF (OverrideInstance, ghc::filesystem, Mock_exists)
                            .WhenCalledWith<const ghc::filesystem::path&, 
                                            CO_ANY_TYPE>(includeMapsPath, CO_ANY)
                            .Returns<bool>(false)
                            .Times(1)
                            .Expected();
            //Create directory
            CO_INSTRUCT_REF (OverrideInstance, ghc::filesystem, Mock_create_directories)
                            .WhenCalledWith<const ghc::filesystem::path&, 
                                            CO_ANY_TYPE>(includeMapsPath, CO_ANY)
                            .Returns<bool>(true)
                            .Times(1)
                            .Expected();
        );
        
        ssTEST_OUTPUT_ASSERT(   "Initialize should succeed",
                                includeManager->Initialize(buildDirPath), true);
        ssTEST_OUTPUT_ASSERT(   "IncludeRecordDir should be set correctly",
                                IncludeManagerAccessor::GetIncludeRecordDir(*includeManager) == 
                                includeMapsPath);
        ssTEST_OUTPUT_ASSERT("", CO_GET_FAILED_FUNCTIONS(OverrideInstance).size(), 0);
    };
    
    ssTEST("Initialize Should Succeed When Directory Already Exists")
    {
        using namespace CppOverride;
        ssTEST_OUTPUT_SETUP
        (
            //Check if directory exists
            CO_INSTRUCT_REF (OverrideInstance, ghc::filesystem, Mock_exists)
                            .WhenCalledWith<const ghc::filesystem::path&, 
                                            CO_ANY_TYPE>(includeMapsPath, CO_ANY)
                            .Returns<bool>(true)
                            .Times(1)
                            .Expected();
            //Create directory should not be called
            CO_INSTRUCT_REF (OverrideInstance, ghc::filesystem, Mock_create_directories)
                            .ExpectedNotSatisfied();
        );
        
        ssTEST_OUTPUT_ASSERT(   "Initialize should succeed",
                                includeManager->Initialize(buildDirPath), true);
        ssTEST_OUTPUT_ASSERT(   "IncludeRecordDir should be set correctly",
                                IncludeManagerAccessor::GetIncludeRecordDir(*includeManager) == 
                                includeMapsPath);
        ssTEST_OUTPUT_ASSERT("", CO_GET_FAILED_FUNCTIONS(OverrideInstance).size(), 0);
    };

    ssTEST("Initialize Should Fail When Directory Creation Fails")
    {
        using namespace CppOverride;
        ssTEST_OUTPUT_SETUP
        (
            //Check if directory exists
            CO_INSTRUCT_REF (OverrideInstance, ghc::filesystem, Mock_exists)
                            .WhenCalledWith<const ghc::filesystem::path&, 
                                            CO_ANY_TYPE>(includeMapsPath, CO_ANY)
                            .Returns<bool>(false)
                            .Times(1)
                            .Expected();
            //Create directory fails
            CO_INSTRUCT_REF (OverrideInstance, ghc::filesystem, Mock_create_directories)
                            .WhenCalledWith<const ghc::filesystem::path&, 
                                            CO_ANY_TYPE>(includeMapsPath, CO_ANY)
                            .Returns<bool>(false)
                            .Times(1)
                            .Expected();
        );
        
        ssTEST_OUTPUT_ASSERT(   "Initialize should fail",
                                includeManager->Initialize(buildDirPath), false);
        ssTEST_OUTPUT_ASSERT("", CO_GET_FAILED_FUNCTIONS(OverrideInstance).size(), 0);
    };
    
    ssTEST("WriteIncludeRecord Should Write Multiple Includes Successfully")
    {
        using namespace CppOverride;
        ssTEST_OUTPUT_SETUP
        (
            //Initialize first
            CO_INSTRUCT_REF (OverrideInstance, ghc::filesystem, Mock_exists)
                            .WhenCalledWith<const ghc::filesystem::path&, 
                                            CO_ANY_TYPE>(includeMapsPath, CO_ANY)
                            .Times(1)
                            .Returns<bool>(true)
                            .Expected();
            
            //Mock GetRecordPath
            const std::string recordPath = absPathPrefix + "/tmp/Build/IncludeMaps/test.Includes";
            
            CO_INSTRUCT_REF (OverrideInstance, runcpp2::IncludeManager, GetRecordPath)
                            .WhenCalledWith<const ghc::filesystem::path&>(sourcePaths[0])
                            .Returns<ghc::filesystem::path>(recordPath)
                            .Times(1)
                            .MatchesObject(includeManager.get())
                            .Expected();
            //Open output file
            CO_INSTRUCT_NO_REF  (OverrideInstance, Mock_ofstream)
                                .WhenCalledWith<const ghc::filesystem::path&>(recordPath)
                                .Returns<void>()
                                .Times(1)
                                .Expected();
            //Check if file is opened
            CO_INSTRUCT_REF   (OverrideInstance, std::Mock_ifstream, is_open)
                                .Returns<bool>(true)
                                .Times(1)
                                .Expected();
            //Capture content on close
            std::string writeResult;
            CO_INSTRUCT_REF (OverrideInstance, std::Mock_ofstream, close)
                                .Returns<void>()
                                .Times(1)
                                .WhenCalledExpectedly_Do
                                (
                                    [&](void* instance, ...)
                                    {
                                        writeResult = 
                                            static_cast<std::Mock_ofstream*>(instance)->
                                            StringStream.str();
                                    }
                                )
                                .Expected();
        );
        
        //Initialize first
        ssTEST_OUTPUT_ASSERT(   "Initialize should succeed",
                                includeManager->Initialize(buildDirPath), true);

        //Write includes
        std::vector<ghc::filesystem::path> includes = 
        {
            includePaths[0],
            includePaths[1],
            includePaths[2]
        };
        
        ssTEST_OUTPUT_EXECUTION
        (
            bool writeIncludeRecordResult = 
                includeManager->WriteIncludeRecord(sourcePaths[0], includes);
        );
        
        ssTEST_OUTPUT_ASSERT("WriteIncludeRecord should succeed", writeIncludeRecordResult, true);
        //Verify content
        for(const ghc::filesystem::path& include : includes)
        {
            ssTEST_OUTPUT_ASSERT(   "Include path should be in content",
                                    writeResult.find(include.string()) != std::string::npos);
            ssTEST_OUTPUT_VALUES_WHEN_FAILED(writeResult, include.string());
        }
        
        ssTEST_OUTPUT_ASSERT("", CO_GET_FAILED_FUNCTIONS(OverrideInstance).size(), 0);
    };

    ssTEST("WriteIncludeRecord Should Fail With Invalid Paths")
    {
        using namespace CppOverride;
        ssTEST_OUTPUT_SETUP
        (
            //Initialize first
            CO_INSTRUCT_REF (OverrideInstance, ghc::filesystem, Mock_exists)
                            .WhenCalledWith<const ghc::filesystem::path&, 
                                            CO_ANY_TYPE>(includeMapsPath, CO_ANY)
                            .Times(1)
                            .Returns<bool>(true)
                            .Expected();
        );
        
        //Initialize first
        ssTEST_OUTPUT_ASSERT(   "Initialize should succeed",
                                includeManager->Initialize(buildDirPath), true);

        //Test with relative source path
        std::vector<ghc::filesystem::path> includes = { includePaths[0] };
        ssTEST_OUTPUT_ASSERT(   "WriteIncludeRecord should fail with relative source path",
                                includeManager->WriteIncludeRecord("relative/path", includes), false);

        ssTEST_OUTPUT_SETUP
        (
            //Test when file open fails
            const std::string recordPath = absPathPrefix + "/tmp/Build/IncludeMaps/test.Includes";
            CO_INSTRUCT_REF (OverrideInstance, runcpp2::IncludeManager, GetRecordPath)
                            .WhenCalledWith<const ghc::filesystem::path&>(sourcePaths[0])
                            .Returns<ghc::filesystem::path>(recordPath)
                            .Times(1)
                            .MatchesObject(includeManager.get())
                            .Expected();
            CO_INSTRUCT_NO_REF  (OverrideInstance, Mock_ofstream)
                                .WhenCalledWith<const ghc::filesystem::path&>(recordPath)
                                .Returns<void>()
                                .Times(1)
                                .Expected();
            CO_INSTRUCT_REF (OverrideInstance, std::Mock_ofstream, is_open)
                            .Returns<bool>(false)
                            .Times(1)
                            .Expected();
        );
        
        ssTEST_OUTPUT_ASSERT(   "WriteIncludeRecord should fail when file open fails",
                                includeManager->WriteIncludeRecord(sourcePaths[0], includes), false);
        ssTEST_OUTPUT_ASSERT("", CO_GET_FAILED_FUNCTIONS(OverrideInstance).size(), 0);
    };
    
    ssTEST_ONLY_THIS("ReadIncludeRecord Should Read Multiple Includes Successfully")
    {
        using namespace CppOverride;
        ssTEST_OUTPUT_SETUP
        (
            //Initialize first
            CO_INSTRUCT_REF (OverrideInstance, ghc::filesystem, Mock_exists)
                            .WhenCalledWith<const ghc::filesystem::path&, 
                                            CO_ANY_TYPE>(includeMapsPath, CO_ANY)
                            .Times(1)
                            .Returns<bool>(true)
                            .Expected();
            //Mock GetRecordPath
            const std::string recordPath = absPathPrefix + "/tmp/Build/IncludeMaps/test.Includes";
            CO_INSTRUCT_REF (OverrideInstance, runcpp2::IncludeManager, GetRecordPath)
                            .WhenCalledWith<const ghc::filesystem::path&>(sourcePaths[0])
                            .Returns<ghc::filesystem::path>(recordPath)
                            .Times(1)
                            .MatchesObject(includeManager.get())
                            .Expected();
            //Check if record file exists
            CO_INSTRUCT_REF (OverrideInstance, ghc::filesystem, Mock_exists)
                            .WhenCalledWith<const ghc::filesystem::path&, 
                                            CO_ANY_TYPE>(recordPath, CO_ANY)
                            .Returns<bool>(true)
                            .Times(1)
                            .Expected();
            //Mock last_write_time
            const auto recordTime = ghc::filesystem::file_time_type::clock::now();
            CO_INSTRUCT_REF (OverrideInstance, ghc::filesystem, Mock_last_write_time)
                            .WhenCalledWith<const ghc::filesystem::path&, 
                                            CO_ANY_TYPE>(recordPath, CO_ANY)
                            .Returns<ghc::filesystem::file_time_type>(recordTime)
                            .Times(1)
                            .Expected();
            //Open input file
            CO_INSTRUCT_NO_REF  (OverrideInstance, Mock_ifstream)
                                .WhenCalledWith<const ghc::filesystem::path&>(recordPath)
                                .Returns<void>()
                                .Times(1)
                                .Expected();
            //Check if file is opened
            CO_INSTRUCT_REF (OverrideInstance, std::Mock_ifstream, is_open)
                            .Returns<bool>(true)
                            .Times(1)
                            .Expected();
            //Mock getline to return includes
            int callCount = 0;
            CO_INSTRUCT_REF (OverrideInstance, std, Mock_getline)
                            .Returns<bool>(true)
                            .WhenCalledExpectedly_Do
                            (
                                [&callCount, &includePaths]
                                (
                                    void*, 
                                    std::vector<CppOverride::TypedDataInfo>& args
                                )
                                {
                                    if(args.at(1).IsType<std::string&>())
                                    {
                                        *(args.at(1).GetTypedDataPtr<std::string&>()) = 
                                            includePaths[callCount++];
                                    }
                                }
                            )
                            .Times(3);
            //Mock getline to return false for end of file
            CO_INSTRUCT_REF (OverrideInstance, std, Mock_getline)
                            .Returns<bool>(false)
                            .Times(1)
                            .Expected();
        );
        
        //Initialize first
        ssTEST_OUTPUT_ASSERT(   "Initialize should succeed",
                                includeManager->Initialize(buildDirPath), true);
        //Read includes
        std::vector<ghc::filesystem::path> outIncludes;
        ghc::filesystem::file_time_type outRecordTime;
        ssTEST_OUTPUT_ASSERT(   "ReadIncludeRecord should succeed",
                                includeManager->ReadIncludeRecord(  sourcePaths[0], 
                                                                    outIncludes, 
                                                                    outRecordTime), 
                                true);
        //Verify content
        ssTEST_OUTPUT_ASSERT(   "Number of includes should match",
                                outIncludes.size() == 3);
        for(std::size_t i = 0; i < outIncludes.size(); ++i)
        {
            ssTEST_OUTPUT_ASSERT("Include path should match", outIncludes[i], includePaths[i]);
            ssTEST_OUTPUT_VALUES_WHEN_FAILED(outIncludes[i].string(), includePaths[i]);
        }
        
        ssTEST_OUTPUT_ASSERT("Record time should match", outRecordTime == recordTime);
        ssTEST_OUTPUT_ASSERT("", CO_GET_FAILED_FUNCTIONS(OverrideInstance).size(), 0);
        ssTEST_OUTPUT_VALUES_WHEN_FAILED("\n" + CO_GET_FAILED_REPORT(OverrideInstance));
    };

    ssTEST("ReadIncludeRecord Should Fail When Record Doesn't Exist")
    {
        using namespace CppOverride;
        ssTEST_OUTPUT_SETUP
        (
            //Initialize first
            CO_INSTRUCT_REF (OverrideInstance, ghc::filesystem, Mock_exists)
                            .WhenCalledWith<const ghc::filesystem::path&, 
                                            CO_ANY_TYPE>(includeMapsPath, CO_ANY)
                            .Times(1)
                            .Returns<bool>(true)
                            .Expected();
        );
        
        //Initialize first
        ssTEST_OUTPUT_ASSERT(   "Initialize should succeed",
                                includeManager->Initialize(buildDirPath), true);

        ssTEST_OUTPUT_SETUP
        (
            //Test when record doesn't exist
            const std::string recordPath = absPathPrefix + "/tmp/Build/IncludeMaps/test.Includes";
            CO_INSTRUCT_REF (OverrideInstance, runcpp2::IncludeManager, GetRecordPath)
                            .WhenCalledWith<const ghc::filesystem::path&>(sourcePaths[0])
                            .Returns<ghc::filesystem::path>(recordPath)
                            .Times(1)
                            .MatchesObject(includeManager.get())
                            .Expected();
            CO_INSTRUCT_REF (OverrideInstance, ghc::filesystem, Mock_exists)
                            .WhenCalledWith<const ghc::filesystem::path&, 
                                            CO_ANY_TYPE>(recordPath, CO_ANY)
                            .Returns<bool>(false)
                            .Times(1)
                            .Expected();
        );
        
        std::vector<ghc::filesystem::path> outIncludes;
        ghc::filesystem::file_time_type outRecordTime;
        ssTEST_OUTPUT_ASSERT(   "ReadIncludeRecord should fail when record doesn't exist",
                                includeManager->ReadIncludeRecord(  sourcePaths[0], 
                                                                    outIncludes, 
                                                                    outRecordTime), 
                                false);
        ssTEST_OUTPUT_ASSERT("", CO_GET_FAILED_FUNCTIONS(OverrideInstance).size(), 0);
    };

    ssTEST("ReadIncludeRecord Should Fail When File Open Fails")
    {
        using namespace CppOverride;
        ssTEST_OUTPUT_SETUP
        (
            //Initialize first
            CO_INSTRUCT_REF (OverrideInstance, ghc::filesystem, Mock_exists)
                            .WhenCalledWith<const ghc::filesystem::path&, 
                                            CO_ANY_TYPE>(includeMapsPath, CO_ANY)
                            .Times(1)
                            .Returns<bool>(true)
                            .Expected();
        );
        
        //Initialize first
        ssTEST_OUTPUT_ASSERT(   "Initialize should succeed",
                                includeManager->Initialize(buildDirPath), true);

        ssTEST_OUTPUT_SETUP
        (
            //Test when file open fails
            const std::string recordPath = absPathPrefix + "/tmp/Build/IncludeMaps/test.Includes";
            CO_INSTRUCT_REF (OverrideInstance, runcpp2::IncludeManager, GetRecordPath)
                            .WhenCalledWith<const ghc::filesystem::path&>(sourcePaths[0])
                            .Returns<ghc::filesystem::path>(recordPath)
                            .Times(1)
                            .MatchesObject(includeManager.get())
                            .Expected();
            CO_INSTRUCT_REF   (OverrideInstance, ghc::filesystem, Mock_exists)
                                .WhenCalledWith<const ghc::filesystem::path&, 
                                                CO_ANY_TYPE>(recordPath, CO_ANY)
                                .Returns<bool>(true)
                                .Times(1);

            const auto recordTime = ghc::filesystem::file_time_type::clock::now();
            CO_INSTRUCT_REF (OverrideInstance, ghc::filesystem, Mock_last_write_time)
                            .WhenCalledWith<const ghc::filesystem::path&, 
                                            CO_ANY_TYPE>(recordPath, CO_ANY)
                            .Returns<ghc::filesystem::file_time_type>(recordTime)
                            .Times(1)
                            .Expected();
            CO_INSTRUCT_NO_REF  (OverrideInstance, Mock_ifstream)
                                .WhenCalledWith<const ghc::filesystem::path&>(recordPath)
                                .Returns<void>()
                                .Times(1)
                                .Expected()
            CO_INSTRUCT_REF (OverrideInstance, std::Mock_ifstream, is_open)
                            .Returns<bool>(false)
                            .Times(1)
                            .Expected();
        );
        
        std::vector<ghc::filesystem::path> outIncludes;
        ghc::filesystem::file_time_type outRecordTime;
        ssTEST_OUTPUT_ASSERT(   "ReadIncludeRecord should fail when file open fails",
                                includeManager->ReadIncludeRecord(  sourcePaths[0], 
                                                                    outIncludes, 
                                                                    outRecordTime), 
                                false);
        ssTEST_OUTPUT_ASSERT("", CO_GET_FAILED_FUNCTIONS(OverrideInstance).size(), 0);
    };
    
    ssTEST("NeedsUpdate Should Return True When Source Is Newer")
    {
        using namespace CppOverride;
        
        //Setup timestamps
        const auto recordTime = ghc::filesystem::file_time_type::clock::now();
        const auto sourceTime = recordTime + std::chrono::seconds(1);
        const auto includeTime = recordTime - std::chrono::seconds(1);
        
        ssTEST_OUTPUT_SETUP
        (
            //Mock source file time
            CO_INSTRUCT_REF (OverrideInstance, ghc::filesystem, Mock_last_write_time)
                            .WhenCalledWith<const ghc::filesystem::path&, 
                                            CO_ANY_TYPE>(sourcePaths[0], CO_ANY)
                            .Returns<ghc::filesystem::file_time_type>(sourceTime)
                            .Times(1)
                            .Expected();
            //Mock include file time (shouldn't be called since source is newer than record time)
            CO_INSTRUCT_REF (OverrideInstance, ghc::filesystem, Mock_last_write_time)
                            .WhenCalledWith<const ghc::filesystem::path&, 
                                            CO_ANY_TYPE>(includePaths[0], CO_ANY)
                            .Returns<ghc::filesystem::file_time_type>(includeTime)
                            .ExpectedNotSatisfied();
            //Mock include file exists (shouldn't be called since source is newer than record time)
            CO_INSTRUCT_REF (OverrideInstance, ghc::filesystem, Mock_exists)
                            .WhenCalledWith<const ghc::filesystem::path&, 
                                            CO_ANY_TYPE>(includePaths[0], CO_ANY)
                            .Returns<bool>(true)
                            .ExpectedNotSatisfied();
            
            std::vector<ghc::filesystem::path> includes = { includePaths[0] };
        );
        
        ssTEST_OUTPUT_ASSERT(   "NeedsUpdate should return true when source is newer",
                                includeManager->NeedsUpdate(sourcePaths[0], includes, recordTime), 
                                true);
        ssTEST_OUTPUT_ASSERT("", CO_GET_FAILED_FUNCTIONS(OverrideInstance).size(), 0);
    };

    ssTEST("NeedsUpdate Should Return True When Include Is Newer")
    {
        using namespace CppOverride;
        
        ssTEST_OUTPUT_SETUP
        (
            //Setup timestamps
            const auto recordTime = ghc::filesystem::file_time_type::clock::now();
            const auto sourceTime = recordTime - std::chrono::seconds(1);
            const auto includeTime = recordTime + std::chrono::seconds(1);
            
            //Mock source file time
            CO_INSTRUCT_REF (OverrideInstance, ghc::filesystem, Mock_last_write_time)
                            .WhenCalledWith<const ghc::filesystem::path&, 
                                            CO_ANY_TYPE>(sourcePaths[0], CO_ANY)
                            .Returns<ghc::filesystem::file_time_type>(sourceTime)
                            .Times(1)
                            .Expected();
            //Mock include file exists
            CO_INSTRUCT_REF (OverrideInstance, ghc::filesystem, Mock_exists)
                            .WhenCalledWith<const ghc::filesystem::path&, 
                                            CO_ANY_TYPE>(includePaths[0], CO_ANY)
                            .Returns<bool>(true)
                            .Times(1)
                            .Expected();
            //Mock include file time
            CO_INSTRUCT_REF (OverrideInstance, ghc::filesystem, Mock_last_write_time)
                            .WhenCalledWith<const ghc::filesystem::path&, 
                                            CO_ANY_TYPE>(includePaths[0], CO_ANY)
                            .Returns<ghc::filesystem::file_time_type>(includeTime)
                            .Times(1)
                            .Expected();
            
            std::vector<ghc::filesystem::path> includes = { includePaths[0] };
        );
        
        ssTEST_OUTPUT_ASSERT(   "NeedsUpdate should return true when include is newer",
                                includeManager->NeedsUpdate(sourcePaths[0], includes, recordTime), 
                                true);
        ssTEST_OUTPUT_ASSERT("", CO_GET_FAILED_FUNCTIONS(OverrideInstance).size(), 0);
    };

    ssTEST("NeedsUpdate Should Return False When Record Is Newest")
    {
        using namespace CppOverride;
        
        ssTEST_OUTPUT_SETUP
        (
            //Setup timestamps
            const auto recordTime = ghc::filesystem::file_time_type::clock::now();
            const auto sourceTime = recordTime - std::chrono::seconds(1);
            const auto includeTime = recordTime - std::chrono::seconds(2);
            
            //Mock source file time
            CO_INSTRUCT_REF (OverrideInstance, ghc::filesystem, Mock_last_write_time)
                            .WhenCalledWith<const ghc::filesystem::path&, 
                                            CO_ANY_TYPE>(sourcePaths[0], CO_ANY)
                            .Returns<ghc::filesystem::file_time_type>(sourceTime)
                            .Times(1)
                            .Expected();
            
            //Mock include file exists
            CO_INSTRUCT_REF (OverrideInstance, ghc::filesystem, Mock_exists)
                            .WhenCalledWith<const ghc::filesystem::path&, 
                                            CO_ANY_TYPE>(includePaths[0], CO_ANY)
                            .Returns<bool>(true)
                            .Times(1)
                            .Expected();
            
            //Mock include file time
            CO_INSTRUCT_REF (OverrideInstance, ghc::filesystem, Mock_last_write_time)
                            .WhenCalledWith<const ghc::filesystem::path&, 
                                        CO_ANY_TYPE>(includePaths[0], CO_ANY)
                            .Returns<ghc::filesystem::file_time_type>(includeTime)
                            .Times(1)
                            .Expected();
            
            std::vector<ghc::filesystem::path> includes = { includePaths[0] };
        );
        
        ssTEST_OUTPUT_ASSERT(   "NeedsUpdate should return false when record is newest",
                                includeManager->NeedsUpdate(sourcePaths[0], includes, recordTime), 
                                false);
        ssTEST_OUTPUT_ASSERT("", CO_GET_FAILED_FUNCTIONS(OverrideInstance).size(), 0);
    };

    ssTEST("NeedsUpdate Should Return True When Missing Include File")
    {
        using namespace CppOverride;
        
        ssTEST_OUTPUT_SETUP
        (
            //Setup timestamps
            const auto recordTime = ghc::filesystem::file_time_type::clock::now();
            const auto sourceTime = recordTime - std::chrono::seconds(1);
            
            //Mock source file time
            CO_INSTRUCT_REF (OverrideInstance, ghc::filesystem, Mock_last_write_time)
                            .WhenCalledWith<const ghc::filesystem::path&, 
                                            CO_ANY_TYPE>(sourcePaths[0], CO_ANY)
                            .Returns<ghc::filesystem::file_time_type>(sourceTime)
                            .Times(1)
                            .Expected();
            
            //Mock first include file exists (false)
            CO_INSTRUCT_REF (OverrideInstance, ghc::filesystem, Mock_exists)
                            .WhenCalledWith<const ghc::filesystem::path&, 
                                            CO_ANY_TYPE>(includePaths[0], CO_ANY)
                            .Returns<bool>(false)
                            .Times(1)
                            .Expected();
            
            //Mock second include file exists (true)
            CO_INSTRUCT_REF (OverrideInstance, ghc::filesystem, Mock_exists)
                            .WhenCalledWith<const ghc::filesystem::path&, 
                                            CO_ANY_TYPE>(includePaths[1], CO_ANY)
                            .Times(0)
                            .Expected();
            
            //Mock second include file time
            CO_INSTRUCT_REF (OverrideInstance, ghc::filesystem, Mock_last_write_time)
                            .WhenCalledWith<const ghc::filesystem::path&, 
                                            CO_ANY_TYPE>(includePaths[1], CO_ANY)
                            .Times(0)
                            .Expected();
            
            std::vector<ghc::filesystem::path> includes = { includePaths[0], includePaths[1] };
        );
        
        ssTEST_OUTPUT_ASSERT(   "NeedsUpdate should return true when include is newer",
                                includeManager->NeedsUpdate(sourcePaths[0], includes, recordTime), 
                                true);
        ssTEST_OUTPUT_ASSERT("", CO_GET_FAILED_FUNCTIONS(OverrideInstance).size(), 0);
        ssTEST_OUTPUT_VALUES_WHEN_FAILED(CO_GET_FAILED_REPORT(OverrideInstance));
    };
    
    ssTEST("GetRecordPath Should Generate Valid And Unique Paths")
    {
        using namespace CppOverride;
        
        auto ProcessPath = 
            [](const std::string& path)
            {
                std::string processedPath;
                for(int i = 0; i < path.size(); ++i)
                {
                    if(path[i] == '\\')
                        processedPath += '/';
                    else
                        processedPath += path[i];
                }
                
                return processedPath;
            };
        
        ssTEST_OUTPUT_SETUP
        (
            //Initialize first
            CO_INSTRUCT_REF (OverrideInstance, ghc::filesystem, Mock_exists)
                            .Returns<bool>(true)
                            .Expected();
            
            //Mock hash for first source file
            CO_INSTRUCT_REF (OverrideInstance, std::Mock_hash<std::string>, operator())
                            .If
                            (
                                [&sourcePaths, ProcessPath]
                                (
                                    void*, 
                                    const std::vector<CppOverride::TypedDataInfo>& args
                                )
                                {
                                    if(args.empty())
                                        return false;
                                    if( args.at(0).IsType<std::string>() &&
                                        ProcessPath(*args.at(0).GetTypedDataPtr<std::string>()) == 
                                            sourcePaths.at(0))
                                    {
                                        return true;
                                    }
                                    return false;
                                }
                            )
                            .Returns<std::size_t>(123)
                            .Times(1)
                            .Expected();
            
            //Mock hash for second source file
            CO_INSTRUCT_REF (OverrideInstance, std::Mock_hash<std::string>, operator())
                            .If
                            (
                                [&sourcePaths, ProcessPath]
                                (
                                    void*, 
                                    const std::vector<CppOverride::TypedDataInfo>& args
                                )
                                {
                                    if(args.empty())
                                        return false;
                                    if( args.at(0).IsType<std::string>() &&
                                        ProcessPath(*args.at(0).GetTypedDataPtr<std::string>()) == 
                                            sourcePaths.at(1))
                                    {
                                        return true;
                                    }
                                    return false;
                                }
                            )
                            .Returns<std::size_t>(456)
                            .Times(1)
                            .Expected();
            
            //Mock hash for third source file
            CO_INSTRUCT_REF (OverrideInstance, std::Mock_hash<std::string>, operator())
                            .If
                            (
                                [&sourcePaths, ProcessPath]
                                (
                                    void*, 
                                    const std::vector<CppOverride::TypedDataInfo>& args
                                )
                                {
                                    if(args.empty())
                                        return false;
                                    if( args.at(0).IsType<std::string>() &&
                                        ProcessPath(*args.at(0).GetTypedDataPtr<std::string>()) == 
                                            sourcePaths.at(2))
                                    {
                                        return true;
                                    }
                                    return false;
                                }
                            )
                            .Returns<std::size_t>(789)
                            .Times(1)
                            .Expected();
        );
        
        ssTEST_OUTPUT_EXECUTION
        (
            //Initialize first
            bool initializeResult = includeManager->Initialize(buildDirPath);
            //Get paths for different source files
            ghc::filesystem::path path1 = 
                IncludeManagerAccessor::GetRecordPath(*includeManager, sourcePaths[0]);
            ghc::filesystem::path path2 = 
                IncludeManagerAccessor::GetRecordPath(*includeManager, sourcePaths[1]);
            ghc::filesystem::path path3 = 
                IncludeManagerAccessor::GetRecordPath(*includeManager, sourcePaths[2]);
        );
        
        ssTEST_OUTPUT_ASSERT("Initialize should succeed", initializeResult, true);
        //Verify exact paths
        ssTEST_OUTPUT_ASSERT(   "Path1 should match expected",
                                path1 == includeMapsPath + "/123.Includes");
        ssTEST_OUTPUT_ASSERT(   "Path2 should match expected",
                                path2 == includeMapsPath + "/456.Includes");
        ssTEST_OUTPUT_ASSERT(   "Path3 should match expected",
                                path3 == includeMapsPath + "/789.Includes");
        ssTEST_OUTPUT_ASSERT("", CO_GET_FAILED_FUNCTIONS(OverrideInstance).size(), 0);
    };
    
    ssTEST_END_TEST_GROUP();
    
    return 0;
} 
