#ifndef RUNCPP2_COMPILING_LINKING_HPP
#define RUNCPP2_COMPILING_LINKING_HPP

#include "runcpp2/Data/CompilerProfile.hpp"
#include "runcpp2/Data/ScriptInfo.hpp"
#include <string>

namespace runcpp2 
{
    bool CompileAndLinkScript(  const std::string& scriptPath, 
                                const ScriptInfo& scriptInfo,
                                const CompilerProfile& profile);
}

#endif