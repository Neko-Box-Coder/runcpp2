#ifndef RUNCPP2_COMPILER_PROFILE_HELPER_HPP
#define RUNCPP2_COMPILER_PROFILE_HELPER_HPP

#include "runcpp2/Data/CompilerProfile.hpp"
#include "runcpp2/Data/ScriptInfo.hpp"
namespace runcpp2
{
    bool IsProfileAvailableOnSystem(const CompilerProfile& profile);

    bool IsProfileValidForScript(   const CompilerProfile& profile, 
                                    const ScriptInfo& scriptInfo, 
                                    const std::string& scriptPath);

    std::vector<ProfileName> GetAvailableProfiles(  const std::vector<CompilerProfile>& profiles,
                                                    const ScriptInfo& scriptInfo,
                                                    const std::string& scriptPath);

    int GetPreferredProfileIndex(   const std::string& scriptPath,
                                    const ScriptInfo& scriptInfo,
                                    const std::vector<CompilerProfile>& profiles, 
                                    const std::string& configPreferredProfile);
}

#endif