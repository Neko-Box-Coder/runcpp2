#ifndef RUNCPP2_PROFILE_HELPER_HPP
#define RUNCPP2_PROFILE_HELPER_HPP

#include "runcpp2/Data/Profile.hpp"
#include "runcpp2/Data/ScriptInfo.hpp"

namespace runcpp2
{
    int GetPreferredProfileIndex(   const std::string& scriptPath,
                                    const Data::ScriptInfo& scriptInfo,
                                    const std::vector<Data::Profile>& profiles, 
                                    const std::string& configPreferredProfile);
}

#endif