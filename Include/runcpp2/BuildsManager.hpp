#ifndef RUNCPP2_BUILDS_MANAGER_HPP
#define RUNCPP2_BUILDS_MANAGER_HPP

#include "ghc/filesystem.hpp"

#include <unordered_map>

#if INTERNAL_RUNCPP2_UNIT_TESTS
    class BuildsManagerAccessor;
#endif

namespace runcpp2
{
    class BuildsManager
    {
        #if INTERNAL_RUNCPP2_UNIT_TESTS
            friend class ::BuildsManagerAccessor;
        #endif
        
        private:
            const ghc::filesystem::path ConfigDirectory;
            
            std::unordered_map<std::string, std::string> Mappings;
            std::unordered_map<std::string, std::string> ReverseMappings;
            
            
            const ghc::filesystem::path BuildDirectory;
            const ghc::filesystem::path MappingsFile;
            bool Initialized;
            
            bool ParseMappings(const std::string& mappingsContent);
            
        public:
            BuildsManager(const ghc::filesystem::path& configDirectory);
            BuildsManager(const BuildsManager& other);
            ~BuildsManager();
        
            bool Initialize();
            bool CreateBuildMapping(const ghc::filesystem::path& scriptPath);
            bool RemoveBuildMapping(const ghc::filesystem::path& scriptPath);
            bool HasBuildMapping(const ghc::filesystem::path& scriptPath);
            bool GetBuildMapping(   const ghc::filesystem::path& scriptPath, 
                                    ghc::filesystem::path& outPath);
            bool RemoveAllBuildsMappings();
            bool SaveBuildsMappings();
    };
}

#endif
