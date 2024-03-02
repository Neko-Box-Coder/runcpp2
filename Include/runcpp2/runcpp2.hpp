#ifndef RUNCPP2_RUNCPP2_HPP
#define RUNCPP2_RUNCPP2_HPP

#include "runcpp2/Data/CompilerProfile.hpp"
#include "runcpp2/Data/ScriptInfo.hpp"

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
    
    //--------------------------------------------
    //YAML Parsing
    //--------------------------------------------
    bool ParseCompilerProfiles( const std::string& compilerProfilesString, 
                                std::vector<CompilerProfile>& outProfiles,
                                std::string& outPreferredProfile);
    
    bool ReadUserConfig(std::vector<CompilerProfile>& outProfiles, 
                        std::string& outPreferredProfile);
    
    bool ParseScriptInfo(   const std::string& scriptInfo, 
                            ScriptInfo& outScriptInfo);
    
    //--------------------------------------------
    //Directory setup
    //--------------------------------------------
    bool CreateRuncpp2ScriptDirectory(const std::string& scriptPath);


    //--------------------------------------------
    //Dependencies setup
    //--------------------------------------------
    bool GetDependenciesPaths(  const std::vector<DependencyInfo>& dependencies,
                                std::vector<std::string>& copiesPaths,
                                std::vector<std::string>& sourcesPaths,
                                std::string runcpp2ScriptDir,
                                std::string scriptDir);
    
    bool PopulateLocalDependencies( const std::vector<DependencyInfo>& dependencies,
                                    const std::vector<std::string>& dependenciesCopiesPaths,
                                    const std::vector<std::string>& dependenciessourcesPaths,
                                    const std::string runcpp2ScriptDir);

    bool RunDependenciesSetupSteps( const ProfileName& profileName,
                                    const std::vector<DependencyInfo>& dependencies,
                                    const std::vector<std::string>& dependenciesCopiesPaths);
    
    bool SetupScriptDependencies(   const ProfileName& profileName,
                                    const std::string& scriptPath, 
                                    const ScriptInfo& scriptInfo,
                                    bool resetDependencies,
                                    std::vector<std::string>& outDependenciesLocalCopiesPaths,
                                    std::vector<std::string>& outDependenciesSourcePaths);
    
    //--------------------------------------------
    //Compiling and Linking
    //--------------------------------------------
    bool CopyDependenciesBinaries(  const std::string& scriptPath, 
                                    const ScriptInfo& scriptInfo,
                                    const std::vector<std::string>& dependenciesCopiesPaths,
                                    const CompilerProfile& profile);

    bool CompileAndLinkScript(  const std::string& scriptPath, 
                                const ScriptInfo& scriptInfo,
                                const CompilerProfile& profile);

    //--------------------------------------------
    //Profile check
    //--------------------------------------------
    bool IsProfileAvailableOnSystem(const CompilerProfile& profile);

    bool IsProfileValidForScript(   const CompilerProfile& profile, 
                                    const ScriptInfo& scriptInfo, 
                                    const std::string& scriptPath);

    std::vector<ProfileName> GetAvailableProfiles(  const std::vector<CompilerProfile>& profiles,
                                                    const ScriptInfo& scriptInfo,
                                                    const std::string& scriptPath);

    int GetPreferredProfileIndex(   const std::string& scriptPath,
                                    const ScriptInfo& scriptInfo,
                                    const std::vector<CompilerProfile>& profiles, 
                                    const std::string& configPreferredProfile);

    //--------------------------------------------
    //Running
    //--------------------------------------------
    bool RunScript( const std::string& scriptPath, 
                    const std::vector<CompilerProfile>& profiles,
                    const std::string& configPreferredProfile,
                    const std::string& runArgs);



}


#endif