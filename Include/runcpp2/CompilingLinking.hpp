#ifndef RUNCPP2_COMPILING_LINKING_HPP
#define RUNCPP2_COMPILING_LINKING_HPP

#include "runcpp2/Data/Profile.hpp"
#include "runcpp2/Data/ScriptInfo.hpp"
#include <string>

namespace runcpp2 
{
    bool CompileAndLinkScript(  const std::string& scriptPath, 
                                const Data::ScriptInfo& scriptInfo,
                                const std::vector<Data::DependencyInfo*>& availableDependencies,
                                const Data::Profile& profile,
                                const std::vector<std::string>& copiedDependenciesBinariesPaths,
                                bool buildExecutable,
                                const std::string exeExt);
}

#endif
