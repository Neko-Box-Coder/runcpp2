#ifndef RUNCPP2_DEPENDENCIES_SETUP_HELPER_HPP
#define RUNCPP2_DEPENDENCIES_SETUP_HELPER_HPP

#include "runcpp2/Data/DependencyInfo.hpp"
#include "runcpp2/Data/ScriptInfo.hpp"
#include "runcpp2/Data/CompilerProfile.hpp"
#include <vector>

namespace runcpp2
{
    bool IsDependencyAvailableForThisPlatform(const Data::DependencyInfo& dependency);
    
    
    bool SetupScriptDependencies(   const ProfileName& profileName,
                                    const std::string& scriptPath, 
                                    Data::ScriptInfo& scriptInfo,
                                    bool resetDependencies,
                                    std::vector<std::string>& outDependenciesLocalCopiesPaths,
                                    std::vector<std::string>& outDependenciesSourcePaths);

    bool CopyDependenciesBinaries(  const std::string& scriptPath, 
                                    const Data::ScriptInfo& scriptInfo,
                                    const std::vector<std::string>& dependenciesCopiesPaths,
                                    const Data::CompilerProfile& profile,
                                    std::vector<std::string>& outCopiedBinariesPaths);
}

#endif