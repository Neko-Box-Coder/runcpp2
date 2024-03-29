#ifndef RUNCPP2_RUNCPP2_HPP
#define RUNCPP2_RUNCPP2_HPP

#include "runcpp2/Data/CompilerProfile.hpp"
#include "runcpp2/Data/ScriptInfo.hpp"

#include <string>
#include <vector>

namespace runcpp2
{
    enum class CmdOptions
    {
        NONE,
        SETUP,
        COUNT
    };

    //--------------------------------------------
    //Running
    //--------------------------------------------
    bool RunScript( const std::string& scriptPath, 
                    const std::vector<Data::CompilerProfile>& profiles,
                    const std::string& configPreferredProfile,
                    const std::unordered_map<CmdOptions, std::string> currentOptions,
                    const std::vector<std::string>& runArgs);
}


#endif