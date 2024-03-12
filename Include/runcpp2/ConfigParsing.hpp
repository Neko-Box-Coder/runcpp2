#ifndef RUNCPP2_CONFIG_PARSING_HPP
#define RUNCPP2_CONFIG_PARSING_HPP

#include "runcpp2/Data/CompilerProfile.hpp"
#include "runcpp2/Data/ScriptInfo.hpp"
#include <string>
#include <vector>
namespace runcpp2
{
    bool ReadUserConfig(std::vector<CompilerProfile>& outProfiles, 
                        std::string& outPreferredProfile);
    
    bool ParseScriptInfo(   const std::string& scriptInfo, 
                            ScriptInfo& outScriptInfo);
}

#endif