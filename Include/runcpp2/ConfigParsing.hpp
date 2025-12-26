#ifndef RUNCPP2_CONFIG_PARSING_HPP
#define RUNCPP2_CONFIG_PARSING_HPP

#include "runcpp2/Data/Profile.hpp"
#include "runcpp2/Data/ScriptInfo.hpp"

#include "ghc/filesystem.hpp"

#include <string>
#include <vector>
namespace runcpp2
{
    std::string GetConfigFilePath();
    
    bool WriteDefaultConfigs(   const ghc::filesystem::path& userConfigPath, 
                                const bool writeUserConfig,
                                const bool writeDefaultConfigs);
    
    bool ReadUserConfig(std::vector<Data::Profile>& outProfiles, 
                        std::string& outPreferredProfile,
                        const std::string& customConfigPath = "");
    
    bool ParseScriptInfo(   const std::string& scriptInfo, 
                            Data::ScriptInfo& outScriptInfo);
    
    bool ParseScriptInfo_LibYaml(   const std::string& scriptInfo, 
                                    Data::ScriptInfo& outScriptInfo);
}

#endif
