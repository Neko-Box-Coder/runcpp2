#ifndef RUNCPP2_COMPILER_PROFILE_HELPER_HPP
#define RUNCPP2_COMPILER_PROFILE_HELPER_HPP

#include "runcpp2/Data/CompilerProfile.hpp"
#include "runcpp2/Data/ScriptInfo.hpp"

namespace runcpp2
{
    int GetPreferredProfileIndex(   const std::string& scriptPath,
                                    const Data::ScriptInfo& scriptInfo,
                                    const std::vector<Data::CompilerProfile>& profiles, 
                                    const std::string& configPreferredProfile);
}

#endif