#ifndef RUNCPP2_DEPENDENCIES_SETUP_HELPER_HPP
#define RUNCPP2_DEPENDENCIES_SETUP_HELPER_HPP

#include "runcpp2/Data/DependencyInfo.hpp"
#include "runcpp2/Data/ScriptInfo.hpp"
#include <vector>
namespace runcpp2
{
    bool GetDependenciesPaths(  const std::vector<DependencyInfo>& dependencies,
                                std::vector<std::string>& copiesPaths,
                                std::vector<std::string>& sourcesPaths,
                                std::string runcpp2ScriptDir,
                                std::string scriptDir);
    
    bool PopulateLocalDependencies( const std::vector<DependencyInfo>& dependencies,
                                    const std::vector<std::string>& dependenciesCopiesPaths,
                                    const std::vector<std::string>& dependenciesSourcesPaths,
                                    const std::string runcpp2ScriptDir);

    bool RunDependenciesSetupSteps( const ProfileName& profileName,
                                    const std::vector<DependencyInfo>& dependencies,
                                    const std::vector<std::string>& dependenciesCopiesPaths);
    
    bool SetupScriptDependencies(   const ProfileName& profileName,
                                    const std::string& scriptPath, 
                                    const ScriptInfo& scriptInfo,
                                    bool resetDependencies,
                                    std::vector<std::string>& outDependenciesLocalCopiesPaths,
                                    std::vector<std::string>& outDependenciesSourcePaths);
}

#endif