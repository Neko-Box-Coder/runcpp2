#ifndef RUNCPP2_INCLUDE_MANAGER_HPP
#define RUNCPP2_INCLUDE_MANAGER_HPP

#include "ghc/filesystem.hpp"
#include <unordered_map>
#include <vector>

namespace runcpp2
{
    class IncludeManager
    {
    public:
        IncludeManager() = default;
        ~IncludeManager() = default;
        
        bool Initialize(const ghc::filesystem::path& buildDir);
        
        bool WriteIncludeRecord(const ghc::filesystem::path& sourceFile,
                                const std::vector<ghc::filesystem::path>& includes);
        
        bool ReadIncludeRecord( const ghc::filesystem::path& sourceFile,
                                std::vector<ghc::filesystem::path>& outIncludes,
                                ghc::filesystem::file_time_type& outRecordTime);
        
        bool NeedsUpdate(   const ghc::filesystem::path& sourceFile,
                            const std::vector<ghc::filesystem::path>& includes,
                            const ghc::filesystem::file_time_type& recordTime) const;
        
    private:
        ghc::filesystem::path GetRecordPath(const ghc::filesystem::path& sourceFile) const;
        
        ghc::filesystem::path IncludeRecordDir;
    };
}

#endif 