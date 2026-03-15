#ifndef RUNCPP2_INCLUDE_MANAGER_HPP
#define RUNCPP2_INCLUDE_MANAGER_HPP

#include "runcpp2/Data/ParseCommon.hpp"

#if !defined(NOMINMAX)
    #define NOMINMAX 1
#endif

#include "ssLogger/ssLog.hpp"
#include "ghc/filesystem.hpp"

#include <fstream>
#include <vector>
#include <chrono>
#include <cstddef>
#include <string>
#include <system_error>

#if defined(INTERNAL_RUNCPP2_UNIT_TESTS) && \
    INTERNAL_RUNCPP2_UNIT_TESTS == INTERNAL_RUNCPP2_UNIT_TESTS_INCLUDE_MANAGER
    
    struct IncludeManagerAccessor;
    #include "Tests/IncludeManager/MockComponents.hpp"
#else
    #define CO_NO_OVERRIDE 1
    #include "CppOverride.hpp"
#endif

namespace runcpp2
{
    class IncludeManager
    {
    public:
        IncludeManager() = default;
        ~IncludeManager() = default;
        
        inline bool Initialize(const ghc::filesystem::path& buildDir)
        {
            INTERNAL_RUNCPP2_SAFE_START();
            ssLOG_FUNC_DEBUG();
            
            IncludeRecordDir = buildDir / "IncludeMaps";
            
            std::error_code e;
            if(!ghc::filesystem::exists(IncludeRecordDir, e))
            {
                if(!ghc::filesystem::create_directories(IncludeRecordDir, e))
                {
                    ssLOG_ERROR("Failed to create IncludeMaps directory: " << IncludeRecordDir);
                    return false;
                }
            }
            
            return true;
            INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
        }
        
        inline bool WriteIncludeRecord( const ghc::filesystem::path& sourceFile,
                                        const std::vector<ghc::filesystem::path>& includes)
        {
            INTERNAL_RUNCPP2_SAFE_START();
            ssLOG_FUNC_DEBUG();

            if(!sourceFile.is_absolute())
            {
                ssLOG_ERROR("Source file is not absolute: " << sourceFile);
                return false;
            }
            
            ghc::filesystem::path recordPath = GetRecordPath(sourceFile);
            
            std::ofstream recordFile(recordPath);
            if(!recordFile.is_open())
            {
                ssLOG_ERROR("Failed to open include record file: " << recordPath);
                return false;
            }
            
            for(const ghc::filesystem::path& include : includes)
                recordFile << include.string() << "\n";
            
            recordFile.close();
            return true;
            INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
        }
        
        inline bool ReadIncludeRecord(  const ghc::filesystem::path& sourceFile,
                                        std::vector<ghc::filesystem::path>& outIncludes,
                                        ghc::filesystem::file_time_type& outRecordTime)
        {
            INTERNAL_RUNCPP2_SAFE_START();
            ssLOG_FUNC_DEBUG();
            
            if(!sourceFile.is_absolute())
            {
                ssLOG_ERROR("Source file is not absolute: " << sourceFile);
                return false;
            }
            
            outIncludes.clear();
            ghc::filesystem::path recordPath = GetRecordPath(sourceFile);
            
            std::error_code e;
            if(!ghc::filesystem::exists(recordPath, e))
                return false;
            
            outRecordTime = ghc::filesystem::last_write_time(recordPath, e);
            
            std::ifstream recordFile(recordPath);
            if(!recordFile.is_open())
            {
                ssLOG_ERROR("Failed to open include record file: " << recordPath);
                return false;
            }
            
            std::string line;
            while(std::getline(recordFile, line))
            {
                if(!line.empty())
                    outIncludes.push_back(ghc::filesystem::path(line));
            }
            
            return true;
            INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
        }
        
        inline bool NeedsUpdate(const ghc::filesystem::path& sourceFile,
                                const std::vector<ghc::filesystem::path>& includes,
                                const ghc::filesystem::file_time_type& recordTime) const
        {
            INTERNAL_RUNCPP2_SAFE_START();
            ssLOG_FUNC_DEBUG();
            
            ssLOG_DEBUG("Checking includes for " << sourceFile.string());
            
            std::error_code e;
            ghc::filesystem::file_time_type sourceTime = ghc::filesystem::last_write_time(sourceFile, e);
            
            ssLOG_DEBUG("sourceTime: " << sourceTime.time_since_epoch().count());
            ssLOG_DEBUG("recordTime: " << recordTime.time_since_epoch().count());
            
            if(sourceTime > recordTime)
            {
                ssLOG_DEBUG("Source file newer than include record");
                return true;
            }
            
            for(const ghc::filesystem::path& include : includes)
            {
                if(!ghc::filesystem::exists(include, e))
                {
                    ssLOG_DEBUG("Include file does not exist: " << include.string());
                    return true;
                }
                
                ghc::filesystem::file_time_type includeTime = 
                    ghc::filesystem::last_write_time(include, e);
                
                if(includeTime > recordTime)
                {
                    ssLOG_DEBUG("Include time for " << include.string() << 
                                " is newer than record time");
                    return true;
                }
            }
            
            ssLOG_DEBUG("No update needed for " << sourceFile.string());
            return false;
            
            INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(true);
        }
        
    private:
        #if defined(INTERNAL_RUNCPP2_UNIT_TESTS) && \
            INTERNAL_RUNCPP2_UNIT_TESTS == INTERNAL_RUNCPP2_UNIT_TESTS_INCLUDE_MANAGER
            
            friend struct ::IncludeManagerAccessor;
        #endif
        
        inline ghc::filesystem::path GetRecordPath(const ghc::filesystem::path& sourceFile) const
        {
            CO_INSERT_MEMBER_IMPL(OverrideInstance, ghc::filesystem::path, (sourceFile));
            
            ghc::filesystem::path cleanSourceFile = sourceFile.lexically_normal();
            std::size_t pathHash = std::hash<std::string>{}(cleanSourceFile.string());
            return IncludeRecordDir / (std::to_string(pathHash) + ".Includes");
        }
        
        ghc::filesystem::path IncludeRecordDir;
    };
}

#if defined(INTERNAL_RUNCPP2_UNIT_TESTS) && \
    INTERNAL_RUNCPP2_UNIT_TESTS == INTERNAL_RUNCPP2_UNIT_TESTS_INCLUDE_MANAGER
    
    #include "Tests/IncludeManager/UndefMocks.hpp"
#endif

#endif
