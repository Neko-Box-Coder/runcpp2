#ifndef RUNCPP2_CONFIG_PARSING_HPP
#define RUNCPP2_CONFIG_PARSING_HPP

#include "runcpp2/Data/Profile.hpp"
#include "runcpp2/Data/ScriptInfo.hpp"
#include <string>
#include <vector>
namespace runcpp2
{
    std::string GetConfigFilePath();
    
    bool WriteDefaultConfig(const std::string& userConfigPath);
    
    bool ReadUserConfig(std::vector<Data::Profile>& outProfiles, 
                        std::string& outPreferredProfile);
    
    bool ParseScriptInfo(   const std::string& scriptInfo, 
                            Data::ScriptInfo& outScriptInfo);
}

#endif