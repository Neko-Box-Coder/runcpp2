#ifndef RUNCPP2_RUNCPP2_HPP
#define RUNCPP2_RUNCPP2_HPP

#include "runcpp2/Data/CmdOptions.hpp"
#include "runcpp2/Data/Profile.hpp"
#include "runcpp2/Data/ScriptInfo.hpp"
#include "runcpp2/Data/PipelineResult.hpp"

#include <string>
#include <vector>

namespace runcpp2
{
    struct OptionInfo
    {
        CmdOptions Option;
        bool HasValue;
        std::string Value;
        
        OptionInfo(CmdOptions option, bool hasValue = false, const std::string& value = "")
            : Option(option), HasValue(hasValue), Value(value) {}
    };

    void GetDefaultScriptInfo(std::string& scriptInfo);

    void SetLogLevel(const std::string& logLevel);

    PipelineResult CheckSourcesNeedUpdate(  const std::string& scriptPath,
                                            const std::vector<Data::Profile>& profiles,
                                            const std::string& configPreferredProfile,
                                            const Data::ScriptInfo& scriptInfo,
                                            const std::unordered_map<   CmdOptions, 
                                                                        std::string>& currentOptions,
                                            bool& outNeedsUpdate);

    PipelineResult StartPipeline(   const std::string& scriptPath, 
                                    const std::vector<Data::Profile>& profiles,
                                    const std::string& configPreferredProfile,
                                    const std::unordered_map<CmdOptions, std::string> currentOptions,
                                    const std::vector<std::string>& runArgs,
                                    const Data::ScriptInfo* lastScriptInfo,
                                    Data::ScriptInfo& outScriptInfo,
                                    const std::string& buildOutputDir,
                                    int& returnStatus);

    std::string PipelineResultToString(PipelineResult result);
}

#endif
