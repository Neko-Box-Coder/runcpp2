#ifndef RUNCPP2_COMPILING_LINKING_HPP
#define RUNCPP2_COMPILING_LINKING_HPP

#include "runcpp2/Data/Profile.hpp"
#include "runcpp2/Data/ScriptInfo.hpp"

#include "ghc/filesystem.hpp"

#include <string>

namespace runcpp2 
{
    bool CompileScriptOnly( const ghc::filesystem::path& buildDir,
                            const ghc::filesystem::path& scriptPath,
                            const std::vector<ghc::filesystem::path>& sourceFiles,
                            const std::vector<bool>& sourceHasCache,
                            const std::vector<ghc::filesystem::path>& includePaths,
                            const Data::ScriptInfo& scriptInfo,
                            const std::vector<Data::DependencyInfo*>& availableDependencies,
                            const Data::Profile& profile,
                            bool buildExecutable);
    
    //TODO: Convert string paths to filesystem paths
    bool CompileAndLinkScript(  const ghc::filesystem::path& buildDir,
                                const ghc::filesystem::path& scriptPath,
                                const std::string& outputName,
                                const std::vector<ghc::filesystem::path>& sourceFiles,
                                const std::vector<bool>& sourceHasCache,
                                const std::vector<ghc::filesystem::path>& includePaths,
                                const Data::ScriptInfo& scriptInfo,
                                const std::vector<Data::DependencyInfo*>& availableDependencies,
                                const Data::Profile& profile,
                                const std::vector<std::string>& compiledObjectsPaths,
                                bool buildExecutable,
                                const std::string exeExt);
}

#endif
