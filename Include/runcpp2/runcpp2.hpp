#ifndef RUNCPP2_RUNCPP2_HPP
#define RUNCPP2_RUNCPP2_HPP

#include "runcpp2/CompilerProfile.hpp"
#include "runcpp2/ScriptInfo.hpp"

#include <string>
#include <vector>

namespace runcpp2
{
    //class YAML_NodeStruct
    //{
    //    public:
    //        virtual bool ParseYAML_Node(YAML::Node& profileNode) = 0;
    //        virtual std::string ToString() const = 0;
    //};
    
    bool ParseCompilerProfiles( const std::string& compilerProfilesString, 
                                std::vector<CompilerProfile>& outProfiles);
    
    bool ReadUserConfig(std::vector<CompilerProfile>& outProfiles);
    
    bool ParseScriptInfo(   const std::string& scriptInfo, 
                            ScriptInfo& outScriptInfo);
    
    bool RunScript(const std::string& scriptPath, const std::vector<CompilerProfile>& profiles);



}


#endif