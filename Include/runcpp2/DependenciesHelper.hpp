#ifndef RUNCPP2_DEPENDENCIES_SETUP_HELPER_HPP
#define RUNCPP2_DEPENDENCIES_SETUP_HELPER_HPP

#include "runcpp2/Data/DependencyInfo.hpp"
#include "runcpp2/Data/ScriptInfo.hpp"
#include "runcpp2/Data/Profile.hpp"

#include "ghc/filesystem.hpp"
#include <vector>

namespace runcpp2
{
    bool GetDependenciesPaths(  const std::vector<Data::DependencyInfo*>& availableDependencies,
                                std::vector<std::string>& outCopiesPaths,
                                std::vector<std::string>& outSourcesPaths,
                                const ghc::filesystem::path& scriptPath,
                                const ghc::filesystem::path& buildDir);
    
    bool IsDependencyAvailableForThisPlatform(const Data::DependencyInfo& dependency);
    
    bool CleanupDependencies(   const runcpp2::Data::Profile& profile,
                                const Data::ScriptInfo& scriptInfo,
                                const std::vector<Data::DependencyInfo*>& availableDependencies,
                                const std::vector<std::string>& dependenciesLocalCopiesPaths);
    
    bool SetupDependencies( const runcpp2::Data::Profile& profile,
                            const ghc::filesystem::path& buildDir,
                            const Data::ScriptInfo& scriptInfo,
                            std::vector<Data::DependencyInfo*>& availableDependencies,
                            const std::vector<std::string>& dependenciesLocalCopiesPaths,
                            const std::vector<std::string>& dependenciesSourcePaths);

    bool BuildDependencies( const runcpp2::Data::Profile& profile,
                            const Data::ScriptInfo& scriptInfo,
                            const std::vector<Data::DependencyInfo*>& availableDependencies,
                            const std::vector<std::string>& dependenciesLocalCopiesPaths);

    bool CopyDependenciesBinaries(  const ghc::filesystem::path& buildDir,
                                    const std::vector<Data::DependencyInfo*>& availableDependencies,
                                    const std::vector<std::string>& dependenciesCopiesPaths,
                                    const Data::Profile& profile,
                                    std::vector<std::string>& outCopiedBinariesPaths);
}

#endif
