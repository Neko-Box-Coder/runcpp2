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
    
    bool GetScriptInfoString(   const std::string& processedScriptPath, 
                                std::string& outScriptInfoString);
    
    bool ParseScriptInfo(   const std::string& scriptInfo, 
                            ScriptInfo& outScriptInfo);
    
    bool CreateRuncpp2ScriptDirectory(const std::string& processedScriptPath);
    
    bool SetupScriptDependencies(   const std::string& processedScriptPath, 
                                    const ScriptInfo& scriptInfo);
    
    bool CopyDependencies(  const std::string& processedScriptPath, 
                            const ScriptInfo& scriptInfo);

    bool CompileAndLinkScript(  const std::string& processedScriptPath, 
                                const ScriptInfo& scriptInfo,
                                const std::vector<CompilerProfile>& profiles);



    bool RunScript(const std::string& scriptPath, const std::vector<CompilerProfile>& profiles);



}


#endif