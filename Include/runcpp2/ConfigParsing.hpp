#ifndef RUNCPP2_CONFIG_PARSING_HPP
#define RUNCPP2_CONFIG_PARSING_HPP

#include "runcpp2/Data/Profile.hpp"
#include "runcpp2/Data/ScriptInfo.hpp"

#include "ghc/filesystem.hpp"

#include <string>
#include <vector>
namespace runcpp2
{
    DS::Result<std::string> GetConfigFilePath();
    
    DS::Result<void> WriteDefaultConfigs(   const ghc::filesystem::path& userConfigPath, 
                                            const bool writeUserConfig,
                                            const bool writeDefaultConfigs);
    
    DS::Result<void> ReadUserConfig(std::vector<Data::Profile>& outProfiles, 
                                    std::string& outPreferredProfile,
                                    const std::string& customConfigPath = "");
    
    DS::Result<void> ParseScriptInfo(const std::string& scriptInfo, Data::ScriptInfo& outScriptInfo);
}

#endif
