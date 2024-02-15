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
                                std::vector<CompilerProfile>& outProfiles,
                                std::string& outPreferredProfile);
    
    bool ReadUserConfig(std::vector<CompilerProfile>& outProfiles, 
                        std::string& outPreferredProfile);
    
    bool GetScriptInfoString(   const std::string& scriptPath, 
                                std::string& outScriptInfoString);
    
    bool ParseScriptInfo(   const std::string& scriptInfo, 
                            ScriptInfo& outScriptInfo);
    
    bool CreateRuncpp2ScriptDirectory(const std::string& scriptPath);

    bool GetDependenciesPaths(  const std::vector<DependencyInfo>& dependencies,
                                std::vector<std::string>& copiesPaths,
                                std::vector<std::string>& sourcesPaths,
                                std::string runcpp2ScriptDir,
                                std::string scriptDir);
    
    bool PopulateLocalDependencies( const std::vector<DependencyInfo>& dependencies,
                                    const std::vector<std::string>& dependenciesCopiesPaths,
                                    const std::vector<std::string>& dependenciessourcesPaths,
                                    const std::string runcpp2ScriptDir);

    bool RunDependenciesSetupSteps( const std::vector<DependencyInfo>& dependencies,
                                    const std::vector<std::string>& dependenciesCopiesPaths);
    
    bool SetupScriptDependencies(   const std::string& scriptPath, 
                                    const ScriptInfo& scriptInfo,
                                    bool resetDependencies);
    
    int GetPerferredProfileIndex(   const std::string& scriptPath,
                                    const ScriptInfo& scriptInfo,
                                    const std::vector<CompilerProfile>& profiles, 
                                    const std::string& configPreferredProfile);
    
    bool CopyDependenciesBinaries(  const std::string& scriptPath, 
                                    const ScriptInfo& scriptInfo,
                                    const std::vector<std::string>& dependenciesCopiesPaths,
                                    const std::vector<CompilerProfile>& profiles,
                                    const std::string& configPreferredProfile);

    bool CompileAndLinkScript(  const std::string& scriptPath, 
                                const ScriptInfo& scriptInfo,
                                const std::vector<CompilerProfile>& profiles);

    bool IsProfileAvailableOnSystem(const CompilerProfile& profile);

    bool IsProfileValidForScript(   const CompilerProfile& profile, 
                                    const ScriptInfo& scriptInfo, 
                                    const std::string& scriptPath);

    bool RunScript( const std::string& scriptPath, 
                    const std::vector<CompilerProfile>& profiles,
                    const std::string& configPreferredProfile);



}


#endif