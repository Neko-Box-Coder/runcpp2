#ifndef RUNCPP2_COMPILING_LINKING_HPP
#define RUNCPP2_COMPILING_LINKING_HPP

#include "runcpp2/Data/CompilerProfile.hpp"
#include "runcpp2/Data/ScriptInfo.hpp"
#include <string>

namespace runcpp2 
{
    bool CompileScript( const std::string& scriptPath, 
                        const ScriptInfo& scriptInfo,
                        const CompilerProfile& profile,
                        std::string& outScriptObjectFilePath);
    
    bool LinkScript(    const std::string& scriptPath, 
                        const ScriptInfo& scriptInfo,
                        const CompilerProfile& profile,
                        const std::string& scriptObjectFilePath,
                        const std::vector<std::string>& copiedDependenciesBinariesNames);
    
    
    bool CompileAndLinkScript(  const std::string& scriptPath, 
                                const ScriptInfo& scriptInfo,
                                const CompilerProfile& profile,
                                const std::vector<std::string>& copiedDependenciesBinariesNames);
}

#endif