#ifndef RUNCPP2_DEPENDENCIES_SETUP_HELPER_HPP
#define RUNCPP2_DEPENDENCIES_SETUP_HELPER_HPP

#include "runcpp2/Data/DependencyInfo.hpp"
#include "runcpp2/Data/ScriptInfo.hpp"
#include "runcpp2/Data/CompilerProfile.hpp"
#include <vector>

namespace runcpp2
{
    bool IsDependencyAvailableForThisPlatform(const DependencyInfo& dependency);
    
    
    bool SetupScriptDependencies(   const ProfileName& profileName,
                                    const std::string& scriptPath, 
                                    ScriptInfo& scriptInfo,
                                    bool resetDependencies,
                                    std::vector<std::string>& outDependenciesLocalCopiesPaths,
                                    std::vector<std::string>& outDependenciesSourcePaths);

    bool CopyDependenciesBinaries(  const std::string& scriptPath, 
                                    const ScriptInfo& scriptInfo,
                                    const std::vector<std::string>& dependenciesCopiesPaths,
                                    const CompilerProfile& profile,
                                    std::vector<std::string>& outCopiedBinariesNames);
}

#endif