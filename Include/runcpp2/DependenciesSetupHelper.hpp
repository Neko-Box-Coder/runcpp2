#ifndef RUNCPP2_DEPENDENCIES_SETUP_HELPER_HPP
#define RUNCPP2_DEPENDENCIES_SETUP_HELPER_HPP

#include "runcpp2/Data/DependencyInfo.hpp"
#include "runcpp2/Data/ScriptInfo.hpp"
#include "runcpp2/Data/Profile.hpp"
#include <vector>

namespace runcpp2
{
    bool IsDependencyAvailableForThisPlatform(const Data::DependencyInfo& dependency);
    
    
    bool SetupScriptDependencies(   const Data::Profile& profile,
                                    const std::string& scriptPath, 
                                    Data::ScriptInfo& scriptInfo,
                                    bool resetDependencies,
                                    std::vector<std::string>& outDependenciesLocalCopiesPaths,
                                    std::vector<std::string>& outDependenciesSourcePaths);

    bool CopyDependenciesBinaries(  const std::string& scriptPath, 
                                    const Data::ScriptInfo& scriptInfo,
                                    const std::vector<std::string>& dependenciesCopiesPaths,
                                    const Data::Profile& profile,
                                    std::vector<std::string>& outCopiedBinariesPaths);
}

#endif
