#ifndef RUNCPP2_RUNCPP2_HPP
#define RUNCPP2_RUNCPP2_HPP

#include "runcpp2/Data/CompilerProfile.hpp"
#include "runcpp2/Data/ScriptInfo.hpp"

#include <string>
#include <vector>

namespace runcpp2
{
    bool CreateRuncpp2ScriptDirectory(const std::string& scriptPath);

    bool CopyDependenciesBinaries(  const std::string& scriptPath, 
                                    const ScriptInfo& scriptInfo,
                                    const std::vector<std::string>& dependenciesCopiesPaths,
                                    const CompilerProfile& profile);

    //--------------------------------------------
    //Running
    //--------------------------------------------
    bool RunScript( const std::string& scriptPath, 
                    const std::vector<CompilerProfile>& profiles,
                    const std::string& configPreferredProfile,
                    const std::vector<std::string>& runArgs);



}


#endif