#ifndef RUNCPP2_PIPELINE_STEPS_HPP
#define RUNCPP2_PIPELINE_STEPS_HPP

#include "runcpp2/Data/Profile.hpp"
#include "runcpp2/Data/PipelineResult.hpp"
#include "runcpp2/Data/ProfilesCommands.hpp"
#include "runcpp2/Data/ScriptInfo.hpp"
#include "runcpp2/Data/CmdOptions.hpp"

#include "runcpp2/BuildsManager.hpp"
#include "runcpp2/IncludeManager.hpp"

#include "ghc/filesystem.hpp"

#include <string>
#include <vector>
#include <unordered_map>

namespace runcpp2
{
    bool CopyFiles( const ghc::filesystem::path& destDir,
                    const std::vector<std::string>& filePaths,
                    std::vector<std::string>& outCopiedPaths);
    
    PipelineResult RunProfileCommands(  const Data::ProfilesCommands* commands,
                                        const Data::Profile& profile,
                                        const std::string& workingDir,
                                        const std::string& commandType);

    PipelineResult ValidateInputs(  const std::string& scriptPath, 
                                    const std::vector<Data::Profile>& profiles,
                                    ghc::filesystem::path& outAbsoluteScriptPath,
                                    ghc::filesystem::path& outScriptDirectory,
                                    std::string& outScriptName);

    PipelineResult 
    ParseAndValidateScriptInfo( const ghc::filesystem::path& absoluteScriptPath,
                                const ghc::filesystem::path& scriptDirectory,
                                const std::string& scriptName,
                                Data::ScriptInfo& outScriptInfo);

    PipelineResult HandleCleanup(   const Data::ScriptInfo& scriptInfo,
                                    const Data::Profile& profile,
                                    const ghc::filesystem::path& scriptDirectory,
                                    const ghc::filesystem::path& buildDir,
                                    const ghc::filesystem::path& absoluteScriptPath,
                                    BuildsManager& buildsManager);

    PipelineResult 
    InitializeBuildDirectory(   const ghc::filesystem::path& configDir,
                                const ghc::filesystem::path& absoluteScriptPath,
                                bool useLocalBuildDir,
                                BuildsManager& outBuildsManager,
                                ghc::filesystem::path& outBuildDir,
                                IncludeManager& outIncludeManager);

    PipelineResult ResolveScriptImports(Data::ScriptInfo& scriptInfo,
                                        const ghc::filesystem::path& scriptPath,
                                        const ghc::filesystem::path& buildDir);
    
    PipelineResult CheckScriptInfoChanges(  const ghc::filesystem::path& buildDir,
                                            const Data::ScriptInfo& scriptInfo,
                                            const Data::Profile& profile,
                                            const ghc::filesystem::path& absoluteScriptPath,
                                            const Data::ScriptInfo* lastScriptInfo,
                                            bool& outRecompileNeeded,
                                            bool& outRelinkNeeded,
                                            std::vector<std::string>& outChangedDependencies);
    
    PipelineResult 
    ProcessDependencies(Data::ScriptInfo& scriptInfo,
                        const Data::Profile& profile,
                        const ghc::filesystem::path& absoluteScriptPath,
                        const ghc::filesystem::path& buildDir,
                        const std::unordered_map<CmdOptions, std::string>& currentOptions,
                        const std::vector<std::string>& changedDependencies,
                        std::vector<Data::DependencyInfo*>& outAvailableDependencies,
                        std::vector<std::string>& outGatheredBinariesPaths);

    void SeparateDependencyFiles(   const Data::FilesTypesInfo& filesTypes,
                                    const std::vector<std::string>& gatheredBinariesPaths,
                                    std::vector<std::string>& outLinkFilesPaths,
                                    std::vector<std::string>& outFilesToCopyPaths);

    PipelineResult HandlePreBuild(  const Data::ScriptInfo& scriptInfo,
                                    const Data::Profile& profile,
                                    const ghc::filesystem::path& buildDir);

    PipelineResult HandlePostBuild( const Data::ScriptInfo& scriptInfo,
                                    const Data::Profile& profile,
                                    const ghc::filesystem::path& buildDir);

    PipelineResult 
    RunCompiledOutput(  const ghc::filesystem::path& target,
                        const ghc::filesystem::path& absoluteScriptPath,
                        const Data::ScriptInfo& scriptInfo,
                        const std::vector<std::string>& runArgs,
                        const std::unordered_map<CmdOptions, std::string>& currentOptions,
                        int& returnStatus);

    PipelineResult 
    HandleBuildOutput(  const ghc::filesystem::path& target,
                        const std::vector<std::string>& filesToCopyPaths,
                        const Data::ScriptInfo& scriptInfo,
                        const Data::Profile& profile,
                        const std::string& buildOutputDir,
                        const std::unordered_map<CmdOptions, std::string>& currentOptions);

    PipelineResult GetTargetPath(   const ghc::filesystem::path& buildDir,
                                    const std::string& scriptName,
                                    const Data::Profile& profile,
                                    const std::unordered_map<CmdOptions, std::string>& currentOptions,
                                    ghc::filesystem::path& outTarget);
    
    bool GatherSourceFiles( const ghc::filesystem::path& absoluteScriptPath, 
                            const Data::ScriptInfo& scriptInfo,
                            const Data::Profile& currentProfile,
                            std::vector<ghc::filesystem::path>& outSourcePaths);
    
    bool GatherIncludePaths(const ghc::filesystem::path& scriptDirectory, 
                            const Data::ScriptInfo& scriptInfo,
                            const Data::Profile& currentProfile,
                            const std::vector<Data::DependencyInfo*>& dependencies,
                            std::vector<ghc::filesystem::path>& outIncludePaths);

    using SourceIncludeMap = std::unordered_map<std::string, std::vector<ghc::filesystem::path>>;
    
    bool GatherFilesIncludes(   const std::vector<ghc::filesystem::path>& sourceFiles,
                                const std::vector<ghc::filesystem::path>& includePaths,
                                SourceIncludeMap& outSourceIncludes);
}


#endif
