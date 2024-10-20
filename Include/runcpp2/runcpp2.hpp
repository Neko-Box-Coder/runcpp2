#ifndef RUNCPP2_RUNCPP2_HPP
#define RUNCPP2_RUNCPP2_HPP

#include "runcpp2/Data/Profile.hpp"
#include "runcpp2/Data/ScriptInfo.hpp"

#include <string>
#include <vector>

namespace runcpp2
{
    enum class CmdOptions
    {
        NONE,
        RESET_CACHE,
        RESET_USER_CONFIG,
        EXECUTABLE,
        HELP,
        REMOVE_DEPENDENCIES,
        LOCAL,
        SHOW_USER_CONFIG,
        SCRIPT_TEMPLATE,
        WATCH,
        BUILD,
        VERSION,
        COUNT
    };
    
    enum class PipelineResult
    {
        UNEXPECTED_FAILURE,
        SUCCESS,
        EMPTY_PROFILES,
        INVALID_SCRIPT_PATH,
        INVALID_CONFIG_PATH,
        INVALID_BUILD_DIR,
        INVALID_SCRIPT_INFO,
        NO_AVAILABLE_PROFILE,
        DEPENDENCIES_FAILED,
        COMPILE_LINK_FAILED,
        INVALID_PROFILE,
        RUN_SCRIPT_FAILED,
        COUNT
    };
    
    struct OptionInfo
    {
        CmdOptions Option;
        bool HasValue;
        std::string Value;
        
        OptionInfo(CmdOptions option, bool hasValue = false, const std::string& value = "")
            : Option(option), HasValue(hasValue), Value(value) {}
    };

    void GetDefaultScriptInfo(std::string& scriptInfo);

    PipelineResult StartPipeline(   const std::string& scriptPath, 
                                    const std::vector<Data::Profile>& profiles,
                                    const std::string& configPreferredProfile,
                                    const std::unordered_map<CmdOptions, std::string> currentOptions,
                                    const std::vector<std::string>& runArgs,
                                    const Data::ScriptInfo* lastScriptInfo,
                                    Data::ScriptInfo& outScriptInfo,
                                    const std::string& buildOutputDir,
                                    int& returnStatus);
}

#endif
