#include "runcpp2/IncludeManager.hpp"
#include "runcpp2/Data/ParseCommon.hpp"
#include "ssLogger/ssLog.hpp"

#include <fstream>

namespace runcpp2
{
    bool IncludeManager::Initialize(const ghc::filesystem::path& buildDir)
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
    
    bool IncludeManager::WriteIncludeRecord(const ghc::filesystem::path& sourceFile,
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
        
        return true;
        INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
    }
    
    bool IncludeManager::ReadIncludeRecord( const ghc::filesystem::path& sourceFile,
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
    
    bool IncludeManager::NeedsUpdate(   const ghc::filesystem::path& sourceFile,
                                        const std::vector<ghc::filesystem::path>& includes,
                                        const ghc::filesystem::file_time_type& recordTime) const
    {
        INTERNAL_RUNCPP2_SAFE_START();
        ssLOG_FUNC_DEBUG();
        
        std::error_code e;
        ghc::filesystem::file_time_type sourceTime = ghc::filesystem::last_write_time(sourceFile, e);
        
        if(sourceTime > recordTime)
            return true;
        
        for(const ghc::filesystem::path& include : includes)
        {
            if(ghc::filesystem::exists(include, e))
            {
                ghc::filesystem::file_time_type includeTime = 
                    ghc::filesystem::last_write_time(include, e);
                if(includeTime > recordTime)
                    return true;
            }
        }
        
        return false;
        INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(true);
    }
    
    ghc::filesystem::path IncludeManager::GetRecordPath(const ghc::filesystem::path& sourceFile) const
    {
        std::size_t pathHash = std::hash<std::string>{}(sourceFile.string());
        return IncludeRecordDir / (std::to_string(pathHash) + ".Includes");
    }
} 