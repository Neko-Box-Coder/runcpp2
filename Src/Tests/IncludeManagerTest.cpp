#include "runcpp2/IncludeManager.hpp"

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
    
    ssTEST_COMMON_SETUP
    {
        includeManager.reset(new runcpp2::IncludeManager());
        CO_CLEAR_ALL_OVERRIDE_SETUP(OverrideInstance);
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
            std::shared_ptr<OverrideResult> dirExistsResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, Mock_exists)
                                .WhenCalledWith<const ghc::filesystem::path&, 
                                                CO_ANY_TYPE>(includeMapsPath, CO_ANY)
                                .Returns<bool>(false)
                                .Times(1)
                                .AssignResult(dirExistsResult);
            
            //Create directory
            std::shared_ptr<OverrideResult> createDirResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, Mock_create_directories)
                                .WhenCalledWith<const ghc::filesystem::path&, 
                                                CO_ANY_TYPE>(includeMapsPath, CO_ANY)
                                .Returns<bool>(true)
                                .Times(1)
                                .AssignResult(createDirResult);
        );
        
        ssTEST_OUTPUT_ASSERT(   "Initialize should succeed",
                                includeManager->Initialize(buildDirPath), true);
        ssTEST_OUTPUT_ASSERT(   "Check if directory exists",
                                dirExistsResult->LastStatusSucceed());
        ssTEST_OUTPUT_ASSERT(   "Create directory",
                                createDirResult->LastStatusSucceed());
        ssTEST_OUTPUT_ASSERT(   "IncludeRecordDir should be set correctly",
                                IncludeManagerAccessor::GetIncludeRecordDir(*includeManager) == 
                                includeMapsPath);
    };
    
    ssTEST("Initialize Should Succeed When Directory Already Exists")
    {
        using namespace CppOverride;
        ssTEST_OUTPUT_SETUP
        (
            //Check if directory exists
            std::shared_ptr<OverrideResult> dirExistsResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, Mock_exists)
                                .WhenCalledWith<const ghc::filesystem::path&, 
                                                CO_ANY_TYPE>(includeMapsPath, CO_ANY)
                                .Returns<bool>(true)
                                .Times(1)
                                .AssignResult(dirExistsResult);
            
            //Create directory should not be called
            std::shared_ptr<OverrideResult> createDirResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, Mock_create_directories)
                                .Times(0)
                                .AssignResult(createDirResult);
        );
        
        ssTEST_OUTPUT_ASSERT(   "Initialize should succeed",
                                includeManager->Initialize(buildDirPath), true);
        ssTEST_OUTPUT_ASSERT(   "Check if directory exists",
                                dirExistsResult->LastStatusSucceed());
        ssTEST_OUTPUT_ASSERT(   "Create directory should not be called",
                                createDirResult->GetStatusCount() == 0);
        ssTEST_OUTPUT_ASSERT(   "IncludeRecordDir should be set correctly",
                                IncludeManagerAccessor::GetIncludeRecordDir(*includeManager) == 
                                includeMapsPath);
    };

    ssTEST("Initialize Should Fail When Directory Creation Fails")
    {
        using namespace CppOverride;
        ssTEST_OUTPUT_SETUP
        (
            //Check if directory exists
            std::shared_ptr<OverrideResult> dirExistsResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, Mock_exists)
                                .WhenCalledWith<const ghc::filesystem::path&, 
                                                CO_ANY_TYPE>(includeMapsPath, CO_ANY)
                                .Returns<bool>(false)
                                .Times(1)
                                .AssignResult(dirExistsResult);
            
            //Create directory fails
            std::shared_ptr<OverrideResult> createDirResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, Mock_create_directories)
                                .WhenCalledWith<const ghc::filesystem::path&, 
                                                CO_ANY_TYPE>(includeMapsPath, CO_ANY)
                                .Returns<bool>(false)
                                .Times(1)
                                .AssignResult(createDirResult);
        );
        
        ssTEST_OUTPUT_ASSERT(   "Initialize should fail",
                                includeManager->Initialize(buildDirPath), false);
        ssTEST_OUTPUT_ASSERT(   "Check if directory exists",
                                dirExistsResult->LastStatusSucceed());
        ssTEST_OUTPUT_ASSERT(   "Create directory should be called",
                                createDirResult->LastStatusSucceed());
    };
    
    ssTEST("WriteIncludeRecord Should Write Multiple Includes Successfully")
    {
        using namespace CppOverride;
        ssTEST_OUTPUT_SETUP
        (
            //Initialize first
            CO_SETUP_OVERRIDE   (OverrideInstance, Mock_exists)
                                .WhenCalledWith<const ghc::filesystem::path&, 
                                                CO_ANY_TYPE>(includeMapsPath, CO_ANY)
                                .Returns<bool>(true);
            
            //Mock GetRecordPath
            const std::string recordPath = absPathPrefix + "/tmp/Build/IncludeMaps/test.Includes";
            std::shared_ptr<OverrideResult> getRecordPathResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, GetRecordPath)
                                .WhenCalledWith<const ghc::filesystem::path&>(sourcePaths[0])
                                .Returns<ghc::filesystem::path>(recordPath)
                                .Times(1)
                                .OverrideObject(includeManager.get())
                                .AssignResult(getRecordPathResult);
            
            //Open output file
            std::shared_ptr<OverrideResult> ofstreamResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, Mock_ofstream)
                                .WhenCalledWith<const ghc::filesystem::path&>(recordPath)
                                .Returns<void>()
                                .Times(1)
                                .AssignResult(ofstreamResult);
            
            //Check if file is opened
            std::shared_ptr<OverrideResult> isOpenResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, is_open)
                                .Returns<bool>(true)
                                .Times(1)
                                .AssignResult(isOpenResult);

            //Capture content on close
            std::string writeResult;
            std::shared_ptr<OverrideResult> closeResult = CreateOverrideResult();
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
                                .AssignResult(closeResult);
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
        
        ssTEST_OUTPUT_ASSERT(   "WriteIncludeRecord should succeed",
                                includeManager->WriteIncludeRecord(sourcePaths[0], includes), true);
        ssTEST_OUTPUT_ASSERT(   "GetRecordPath should be called",
                                getRecordPathResult->LastStatusSucceed());
        ssTEST_OUTPUT_ASSERT(   "Open output file",
                                ofstreamResult->LastStatusSucceed());
        ssTEST_OUTPUT_ASSERT(   "Check if file is opened",
                                isOpenResult->LastStatusSucceed());
        ssTEST_OUTPUT_ASSERT(   "File should be closed",
                                closeResult->LastStatusSucceed());
        
        //Verify content
        for(const ghc::filesystem::path& include : includes)
        {
            ssTEST_OUTPUT_ASSERT(   "Include path should be in content",
                                    writeResult.find(include.string()) != std::string::npos);
            ssTEST_OUTPUT_VALUES_WHEN_FAILED(writeResult, include.string());
        }
    };

    ssTEST("WriteIncludeRecord Should Fail With Invalid Paths")
    {
        using namespace CppOverride;
        ssTEST_OUTPUT_SETUP
        (
            //Initialize first
            CO_SETUP_OVERRIDE   (OverrideInstance, Mock_exists)
                                .Returns<bool>(true);
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
            std::shared_ptr<OverrideResult> getRecordPathResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, GetRecordPath)
                                .WhenCalledWith<const ghc::filesystem::path&>(sourcePaths[0])
                                .Returns<ghc::filesystem::path>(recordPath)
                                .Times(1)
                                .OverrideObject(includeManager.get())
                                .AssignResult(getRecordPathResult);

            std::shared_ptr<OverrideResult> ofstreamResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, Mock_ofstream)
                                .WhenCalledWith<const ghc::filesystem::path&>(recordPath)
                                .Returns<void>()
                                .Times(1)
                                .AssignResult(ofstreamResult);
            
            std::shared_ptr<OverrideResult> isOpenResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, is_open)
                                .Returns<bool>(false)
                                .Times(1)
                                .AssignResult(isOpenResult);
        );
        
        ssTEST_OUTPUT_ASSERT(   "WriteIncludeRecord should fail when file open fails",
                                includeManager->WriteIncludeRecord(sourcePaths[0], includes), false);
        ssTEST_OUTPUT_ASSERT(   "GetRecordPath should be called",
                                getRecordPathResult->LastStatusSucceed());
        ssTEST_OUTPUT_ASSERT(   "Open output file attempt",
                                ofstreamResult->LastStatusSucceed());
        ssTEST_OUTPUT_ASSERT(   "Check if file is opened",
                                isOpenResult->LastStatusSucceed());
    };
    
    ssTEST("ReadIncludeRecord Should Read Multiple Includes Successfully")
    {
        using namespace CppOverride;
        ssTEST_OUTPUT_SETUP
        (
            //Initialize first
            CO_SETUP_OVERRIDE   (OverrideInstance, Mock_exists)
                                .WhenCalledWith<const ghc::filesystem::path&, 
                                                CO_ANY_TYPE>(includeMapsPath, CO_ANY)
                                .Returns<bool>(true);
            
            //Mock GetRecordPath
            const std::string recordPath = absPathPrefix + "/tmp/Build/IncludeMaps/test.Includes";
            std::shared_ptr<OverrideResult> getRecordPathResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, GetRecordPath)
                                .WhenCalledWith<const ghc::filesystem::path&>(sourcePaths[0])
                                .Returns<ghc::filesystem::path>(recordPath)
                                .Times(1)
                                .OverrideObject(includeManager.get())
                                .AssignResult(getRecordPathResult);
            
            //Check if record file exists
            std::shared_ptr<OverrideResult> recordExistsResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, Mock_exists)
                                .WhenCalledWith<const ghc::filesystem::path&, 
                                                CO_ANY_TYPE>(recordPath, CO_ANY)
                                .Returns<bool>(true)
                                .Times(1)
                                .AssignResult(recordExistsResult);
            
            //Mock last_write_time
            const auto recordTime = ghc::filesystem::file_time_type::clock::now();
            std::shared_ptr<OverrideResult> lastWriteResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, Mock_last_write_time)
                                .WhenCalledWith<const ghc::filesystem::path&, 
                                                CO_ANY_TYPE>(recordPath, CO_ANY)
                                .Returns<ghc::filesystem::file_time_type>(recordTime)
                                .Times(1)
                                .AssignResult(lastWriteResult);
            
            //Open input file
            std::shared_ptr<OverrideResult> ifstreamResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, Mock_ifstream)
                                .WhenCalledWith<const ghc::filesystem::path&>(recordPath)
                                .Returns<void>()
                                .Times(1)
                                .AssignResult(ifstreamResult);
            
            //Check if file is opened
            std::shared_ptr<OverrideResult> isOpenResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, is_open)
                                .Returns<bool>(true)
                                .Times(1)
                                .AssignResult(isOpenResult);
            
            //Mock getline to return includes
            std::shared_ptr<OverrideResult> getlineResult = CreateOverrideResult();
            int callCount = 0;
            CO_SETUP_OVERRIDE   (OverrideInstance, Mock_getline)
                                .Returns<bool>(true)
                                .Times(3)
                                .WhenCalledExpectedly_Do
                                (
                                    [&callCount, &includePaths](void*, const std::vector<void*>& args)
                                    {
                                        std::string& line = *static_cast<std::string*>(args[1]);
                                        line = includePaths[callCount++];
                                    }
                                )
                                .AssignResult(getlineResult);
            
            //Mock getline to return false for end of file
            std::shared_ptr<OverrideResult> endOfFileResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, Mock_getline)
                                .Returns<bool>(false)
                                .Times(1)
                                .AssignResult(endOfFileResult);
        );
        
        //Initialize first
        ssTEST_OUTPUT_ASSERT(   "Initialize should succeed",
                                includeManager->Initialize(buildDirPath), true);

        //Read includes
        std::vector<ghc::filesystem::path> outIncludes;
        ghc::filesystem::file_time_type outRecordTime;
        
        ssTEST_OUTPUT_ASSERT(   "ReadIncludeRecord should succeed",
                                includeManager->ReadIncludeRecord(sourcePaths[0], outIncludes, outRecordTime), true);
        ssTEST_OUTPUT_ASSERT(   "GetRecordPath should be called",
                                getRecordPathResult->LastStatusSucceed());
        ssTEST_OUTPUT_ASSERT(   "Check if record exists",
                                recordExistsResult->LastStatusSucceed());
        ssTEST_OUTPUT_ASSERT(   "Get last write time",
                                lastWriteResult->LastStatusSucceed());
        ssTEST_OUTPUT_ASSERT(   "Open input file",
                                ifstreamResult->LastStatusSucceed());
        ssTEST_OUTPUT_ASSERT(   "Check if file is opened",
                                isOpenResult->LastStatusSucceed());
        ssTEST_OUTPUT_ASSERT(   "Read includes with getline",
                                getlineResult->GetSucceedCount(), 3);
        ssTEST_OUTPUT_ASSERT(   "Read includes with getline eof",
                                endOfFileResult->GetStatusCount(), 1);
        
        //Verify content
        ssTEST_OUTPUT_ASSERT(   "Number of includes should match",
                                outIncludes.size() == 3);
        for(std::size_t i = 0; i < outIncludes.size(); ++i)
        {
            ssTEST_OUTPUT_ASSERT(   "Include path should match",
                                    outIncludes[i] == includePaths[i]);
            ssTEST_OUTPUT_VALUES_WHEN_FAILED(outIncludes[i].string(), includePaths[i]);
        }
        
        ssTEST_OUTPUT_ASSERT(   "Record time should match",
                                outRecordTime == recordTime);
    };

    ssTEST("ReadIncludeRecord Should Fail When Record Doesn't Exist")
    {
        using namespace CppOverride;
        ssTEST_OUTPUT_SETUP
        (
            //Initialize first
            CO_SETUP_OVERRIDE   (OverrideInstance, Mock_exists)
                                .Times(1)
                                .Returns<bool>(true);
        );
        
        //Initialize first
        ssTEST_OUTPUT_ASSERT(   "Initialize should succeed",
                                includeManager->Initialize(buildDirPath), true);

        ssTEST_OUTPUT_SETUP
        (
            //Test when record doesn't exist
            const std::string recordPath = absPathPrefix + "/tmp/Build/IncludeMaps/test.Includes";
            std::shared_ptr<OverrideResult> getRecordPathResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, GetRecordPath)
                                .WhenCalledWith<const ghc::filesystem::path&>(sourcePaths[0])
                                .Returns<ghc::filesystem::path>(recordPath)
                                .Times(1)
                                .OverrideObject(includeManager.get())
                                .AssignResult(getRecordPathResult);

            std::shared_ptr<OverrideResult> recordExistsResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, Mock_exists)
                                .WhenCalledWith<const ghc::filesystem::path&, 
                                                CO_ANY_TYPE>(recordPath, CO_ANY)
                                .Returns<bool>(false)
                                .Times(1)
                                .AssignResult(recordExistsResult);
        );
        
        std::vector<ghc::filesystem::path> outIncludes;
        ghc::filesystem::file_time_type outRecordTime;
        ssTEST_OUTPUT_ASSERT(   "ReadIncludeRecord should fail when record doesn't exist",
                                includeManager->ReadIncludeRecord(sourcePaths[0], outIncludes, outRecordTime), false);
        ssTEST_OUTPUT_ASSERT(   "GetRecordPath should be called",
                                getRecordPathResult->LastStatusSucceed());
        ssTEST_OUTPUT_ASSERT(   "Check if record exists",
                                recordExistsResult->LastStatusSucceed());
    };

    ssTEST("ReadIncludeRecord Should Fail When File Open Fails")
    {
        using namespace CppOverride;
        ssTEST_OUTPUT_SETUP
        (
            //Initialize first
            CO_SETUP_OVERRIDE   (OverrideInstance, Mock_exists)
                                .Returns<bool>(true);
        );
        
        //Initialize first
        ssTEST_OUTPUT_ASSERT(   "Initialize should succeed",
                                includeManager->Initialize(buildDirPath), true);

        ssTEST_OUTPUT_SETUP
        (
            //Test when file open fails
            const std::string recordPath = absPathPrefix + "/tmp/Build/IncludeMaps/test.Includes";
            std::shared_ptr<OverrideResult> getRecordPathResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, GetRecordPath)
                                .WhenCalledWith<const ghc::filesystem::path&>(sourcePaths[0])
                                .Returns<ghc::filesystem::path>(recordPath)
                                .Times(1)
                                .OverrideObject(includeManager.get())
                                .AssignResult(getRecordPathResult);

            CO_SETUP_OVERRIDE   (OverrideInstance, Mock_exists)
                                .WhenCalledWith<const ghc::filesystem::path&, 
                                                CO_ANY_TYPE>(recordPath, CO_ANY)
                                .Returns<bool>(true)
                                .Times(1);

            const auto recordTime = ghc::filesystem::file_time_type::clock::now();
            std::shared_ptr<OverrideResult> lastWriteResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, Mock_last_write_time)
                                .WhenCalledWith<const ghc::filesystem::path&, 
                                                CO_ANY_TYPE>(recordPath, CO_ANY)
                                .Returns<ghc::filesystem::file_time_type>(recordTime)
                                .Times(1)
                                .AssignResult(lastWriteResult);

            std::shared_ptr<OverrideResult> ifstreamResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, Mock_ifstream)
                                .WhenCalledWith<const ghc::filesystem::path&>(recordPath)
                                .Returns<void>()
                                .Times(1)
                                .AssignResult(ifstreamResult);
            
            std::shared_ptr<OverrideResult> isOpenResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, is_open)
                                .Returns<bool>(false)
                                .Times(1)
                                .AssignResult(isOpenResult);
        );
        
        std::vector<ghc::filesystem::path> outIncludes;
        ghc::filesystem::file_time_type outRecordTime;
        ssTEST_OUTPUT_ASSERT(   "ReadIncludeRecord should fail when file open fails",
                                includeManager->ReadIncludeRecord(  sourcePaths[0], 
                                                                    outIncludes, 
                                                                    outRecordTime), 
                                false);
        ssTEST_OUTPUT_ASSERT(   "GetRecordPath should be called",
                                getRecordPathResult->LastStatusSucceed());
        ssTEST_OUTPUT_ASSERT(   "Get last write time",
                                lastWriteResult->LastStatusSucceed());
        ssTEST_OUTPUT_ASSERT(   "Open input file attempt",
                                ifstreamResult->LastStatusSucceed());
        ssTEST_OUTPUT_ASSERT(   "Check if file is opened",
                                isOpenResult->LastStatusSucceed());
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
            std::shared_ptr<OverrideResult> sourceTimeResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, Mock_last_write_time)
                                .WhenCalledWith<const ghc::filesystem::path&, 
                                                CO_ANY_TYPE>(sourcePaths[0], CO_ANY)
                                .Returns<ghc::filesystem::file_time_type>(sourceTime)
                                .Times(1)
                                .AssignResult(sourceTimeResult);
            
            //Mock include file time (shouldn't be called since source is newer)
            std::shared_ptr<OverrideResult> includeTimeResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, Mock_last_write_time)
                                .WhenCalledWith<const ghc::filesystem::path&, 
                                                CO_ANY_TYPE>(includePaths[0], CO_ANY)
                                .Returns<ghc::filesystem::file_time_type>(includeTime)
                                .Times(0)
                                .AssignResult(includeTimeResult);
            
            //Mock include file exists (shouldn't be called since source is newer)
            std::shared_ptr<OverrideResult> includeExistsResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, Mock_exists)
                                .WhenCalledWith<const ghc::filesystem::path&, 
                                                CO_ANY_TYPE>(includePaths[0], CO_ANY)
                                .Returns<bool>(true)
                                .Times(0)
                                .AssignResult(includeExistsResult);
            
            std::vector<ghc::filesystem::path> includes = { includePaths[0] };
        );
        
        ssTEST_OUTPUT_ASSERT(   "NeedsUpdate should return true when source is newer",
                                includeManager->NeedsUpdate(sourcePaths[0], includes, recordTime), true);
        ssTEST_OUTPUT_ASSERT(   "Source time should be checked",
                                sourceTimeResult->LastStatusSucceed());
        ssTEST_OUTPUT_ASSERT(   "Include time should not be checked",
                                includeTimeResult->GetStatusCount() == 0);
        ssTEST_OUTPUT_ASSERT(   "Include exists should not be checked",
                                includeExistsResult->GetStatusCount() == 0);
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
            std::shared_ptr<OverrideResult> sourceTimeResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, Mock_last_write_time)
                                .WhenCalledWith<const ghc::filesystem::path&, 
                                                CO_ANY_TYPE>(sourcePaths[0], CO_ANY)
                                .Returns<ghc::filesystem::file_time_type>(sourceTime)
                                .Times(1)
                                .AssignResult(sourceTimeResult);
            
            //Mock include file exists
            std::shared_ptr<OverrideResult> includeExistsResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, Mock_exists)
                                .WhenCalledWith<const ghc::filesystem::path&, 
                                                CO_ANY_TYPE>(includePaths[0], CO_ANY)
                                .Returns<bool>(true)
                                .Times(1)
                                .AssignResult(includeExistsResult);
            
            //Mock include file time
            std::shared_ptr<OverrideResult> includeTimeResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, Mock_last_write_time)
                                .WhenCalledWith<const ghc::filesystem::path&, 
                                                CO_ANY_TYPE>(includePaths[0], CO_ANY)
                                .Returns<ghc::filesystem::file_time_type>(includeTime)
                                .Times(1)
                                .AssignResult(includeTimeResult);
            
            std::vector<ghc::filesystem::path> includes = { includePaths[0] };
        );
        
        ssTEST_OUTPUT_ASSERT(   "NeedsUpdate should return true when include is newer",
                                includeManager->NeedsUpdate(sourcePaths[0], includes, recordTime), 
                                true);
        ssTEST_OUTPUT_ASSERT(   "Source time should be checked",
                                sourceTimeResult->GetSucceedCount(), 1);
        ssTEST_OUTPUT_ASSERT(   "Include exists should be checked",
                                includeExistsResult->GetSucceedCount(), 1);
        ssTEST_OUTPUT_ASSERT(   "Include time should be checked",
                                includeTimeResult->GetSucceedCount(), 1);
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
            std::shared_ptr<OverrideResult> sourceTimeResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, Mock_last_write_time)
                                .WhenCalledWith<const ghc::filesystem::path&, 
                                                CO_ANY_TYPE>(sourcePaths[0], CO_ANY)
                                .Returns<ghc::filesystem::file_time_type>(sourceTime)
                                .Times(1)
                                .AssignResult(sourceTimeResult);
            
            //Mock include file exists
            std::shared_ptr<OverrideResult> includeExistsResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, Mock_exists)
                                .WhenCalledWith<const ghc::filesystem::path&, 
                                                CO_ANY_TYPE>(includePaths[0], CO_ANY)
                                .Returns<bool>(true)
                                .Times(1)
                                .AssignResult(includeExistsResult);
            
            //Mock include file time
            std::shared_ptr<OverrideResult> includeTimeResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, Mock_last_write_time)
                                .WhenCalledWith<const ghc::filesystem::path&, 
                                                CO_ANY_TYPE>(includePaths[0], CO_ANY)
                                .Returns<ghc::filesystem::file_time_type>(includeTime)
                                .Times(1)
                                .AssignResult(includeTimeResult);
            
            std::vector<ghc::filesystem::path> includes = { includePaths[0] };
        );
        
        ssTEST_OUTPUT_ASSERT(   "NeedsUpdate should return false when record is newest",
                                includeManager->NeedsUpdate(sourcePaths[0], includes, recordTime), 
                                false);
        ssTEST_OUTPUT_ASSERT(   "Source time should be checked",
                                sourceTimeResult->GetSucceedCount(), 1);
        ssTEST_OUTPUT_ASSERT(   "Include exists should be checked",
                                includeExistsResult->GetSucceedCount(), 1);
        ssTEST_OUTPUT_ASSERT(   "Include time should be checked",
                                includeTimeResult->GetSucceedCount(), 1);
    };

    ssTEST("NeedsUpdate Should Handle Missing Include Files")
    {
        using namespace CppOverride;
        
        ssTEST_OUTPUT_SETUP
        (
            //Setup timestamps
            const auto recordTime = ghc::filesystem::file_time_type::clock::now();
            const auto sourceTime = recordTime - std::chrono::seconds(1);
            
            //Mock source file time
            std::shared_ptr<OverrideResult> sourceTimeResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, Mock_last_write_time)
                                .WhenCalledWith<const ghc::filesystem::path&, 
                                                CO_ANY_TYPE>(sourcePaths[0], CO_ANY)
                                .Returns<ghc::filesystem::file_time_type>(sourceTime)
                                .Times(1)
                                .AssignResult(sourceTimeResult);
            
            //Mock first include file exists (false)
            std::shared_ptr<OverrideResult> include1ExistsResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, Mock_exists)
                                .WhenCalledWith<const ghc::filesystem::path&, 
                                                CO_ANY_TYPE>(includePaths[0], CO_ANY)
                                .Returns<bool>(false)
                                .Times(1)
                                .AssignResult(include1ExistsResult);
            
            //Mock second include file exists (true)
            std::shared_ptr<OverrideResult> include2ExistsResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, Mock_exists)
                                .WhenCalledWith<const ghc::filesystem::path&, 
                                                CO_ANY_TYPE>(includePaths[1], CO_ANY)
                                .AssignResult(include2ExistsResult);
            
            //Mock second include file time
            std::shared_ptr<OverrideResult> include2TimeResult = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, Mock_last_write_time)
                                .WhenCalledWith<const ghc::filesystem::path&, 
                                                CO_ANY_TYPE>(includePaths[1], CO_ANY)
                                .AssignResult(include2TimeResult);
            
            std::vector<ghc::filesystem::path> includes = { includePaths[0], includePaths[1] };
        );
        
        ssTEST_OUTPUT_ASSERT(   "NeedsUpdate should return true when include is newer",
                                includeManager->NeedsUpdate(sourcePaths[0], includes, recordTime), 
                                true);
        ssTEST_OUTPUT_ASSERT(   "Source time should be checked",
                                sourceTimeResult->GetSucceedCount(), 1);
        ssTEST_OUTPUT_ASSERT(   "First include exists should be checked",
                                include1ExistsResult->GetSucceedCount(), 1);
        ssTEST_OUTPUT_ASSERT(   "Second include exists should not be checked",
                                include2ExistsResult->GetStatusCount(), 0);
        ssTEST_OUTPUT_ASSERT(   "Second include time should not be checked",
                                include2TimeResult->GetStatusCount(), 0);
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
            CO_SETUP_OVERRIDE   (OverrideInstance, Mock_exists)
                                .Returns<bool>(true);
            
            
            
            //Mock hash for first source file
            std::shared_ptr<OverrideResult> hash1Result = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, operator())
                                .If
                                (
                                    [&sourcePaths, ProcessPath](void*, const std::vector<void*>& args)
                                    {
                                        if(args.empty())
                                            return false;
                                        if( ProcessPath(*static_cast<std::string*>(args.at(0))) == 
                                            sourcePaths[0])
                                        {
                                            return true;
                                        }
                                        return false;
                                    }
                                )
                                .Returns<std::size_t>(123)
                                .Times(1)
                                .AssignResult(hash1Result);
            
            //Mock hash for second source file
            std::shared_ptr<OverrideResult> hash2Result = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, operator())
                                .If
                                (
                                    [&sourcePaths, ProcessPath](void*, const std::vector<void*>& args)
                                    {
                                        if(args.empty())
                                            return false;
                                        if( ProcessPath(*static_cast<std::string*>(args.at(0))) == 
                                            sourcePaths[1])
                                        {
                                            return true;
                                        }
                                        return false;
                                    }
                                )
                                .Returns<std::size_t>(456)
                                .Times(1)
                                .AssignResult(hash2Result);
            
            //Mock hash for third source file
            std::shared_ptr<OverrideResult> hash3Result = CreateOverrideResult();
            CO_SETUP_OVERRIDE   (OverrideInstance, operator())
                                .If
                                (
                                    [&sourcePaths, ProcessPath](void*, const std::vector<void*>& args)
                                    {
                                        if(args.empty())
                                            return false;
                                        if( ProcessPath(*static_cast<std::string*>(args.at(0))) == 
                                            sourcePaths[2])
                                        {
                                            return true;
                                        }
                                        return false;
                                    }
                                )
                                .Returns<std::size_t>(789)
                                .Times(1)
                                .AssignResult(hash3Result);
        );
        
        //Initialize first
        ssTEST_OUTPUT_ASSERT(   "Initialize should succeed",
                                includeManager->Initialize(buildDirPath), true);

        ssTEST_OUTPUT_SETUP
        (
            //Get paths for different source files
            ghc::filesystem::path path1 = 
                IncludeManagerAccessor::GetRecordPath(*includeManager, sourcePaths[0]);
            ghc::filesystem::path path2 = 
                IncludeManagerAccessor::GetRecordPath(*includeManager, sourcePaths[1]);
            ghc::filesystem::path path3 = 
                IncludeManagerAccessor::GetRecordPath(*includeManager, sourcePaths[2]);
        );
        
        //Verify exact paths
        ssTEST_OUTPUT_ASSERT(   "Path1 should match expected",
                                path1 == includeMapsPath + "/123.Includes");
        ssTEST_OUTPUT_ASSERT(   "Path2 should match expected",
                                path2 == includeMapsPath + "/456.Includes");
        ssTEST_OUTPUT_ASSERT(   "Path3 should match expected",
                                path3 == includeMapsPath + "/789.Includes");
        
        //Verify hash was called for each source file
        ssTEST_OUTPUT_ASSERT(   "Hash should be called for first source file",
                                hash1Result->GetSucceedCount(), 1);
        ssTEST_OUTPUT_ASSERT(   "Hash should be called for second source file",
                                hash2Result->GetSucceedCount(), 1);
        ssTEST_OUTPUT_ASSERT(   "Hash should be called for third source file",
                                hash3Result->GetSucceedCount(), 1);
        
        ssTEST_OUTPUT_VALUES_WHEN_FAILED(   hash1Result->GetStatusCount(), 
                                            hash2Result->GetStatusCount(),
                                            hash3Result->GetStatusCount());
    };
    
    ssTEST_END_TEST_GROUP();
    
    return 0;
} 
