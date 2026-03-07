#ifndef RUNCPP2_DATA_DEPENDENCY_SOURCE_HPP
#define RUNCPP2_DATA_DEPENDENCY_SOURCE_HPP

#include "runcpp2/Data/GitSource.hpp"
#include "runcpp2/Data/LocalSource.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "runcpp2/LibYAML_Wrapper.hpp"

#include "ssLogger/ssLog.hpp"
#include "ghc/filesystem.hpp"
#include "mpark/variant.hpp"
#include "DSResult/DSResult.hpp"

#include <memory>
#include <vector>
#include <string>

namespace runcpp2
{
namespace Data
{
    struct DependencySource
    {
        mpark::variant<GitSource, LocalSource> Source;
        ghc::filesystem::path ImportPath;
        std::vector<std::shared_ptr<DependencySource>> ImportedSources;
        
        inline bool ParseYAML_Node(YAML::ConstNodePtr node)
        {
            if(ExistAndHasChild(node, "ImportPath"))
            {
                DS_UNWRAP_ASSIGN_ACT(   ImportPath, 
                                        node->GetMapValueScalar<std::string>("ImportPath"), 
                                        ssLOG_ERROR(DS_TMP_ERROR.ToString()); return false);
                
                if(ImportPath.is_absolute())
                {
                    ssLOG_ERROR("DependencySource: ImportPath must be relative: " << 
                                ImportPath.string());
                    return false;
                }
            }

            if(ExistAndHasChild(node, "Git"))
            {
                if(ExistAndHasChild(node, "Local"))
                {
                    ssLOG_ERROR("DependencySource: Both Git and Local sources found");
                    return false;
                }
                
                GitSource gitSource;
                YAML::ConstNodePtr gitNode = node->GetMapValueNode("Git");
                if(!gitSource.ParseYAML_Node(gitNode))
                    return false;
                Source = gitSource;
                return true;
            }
            else if(ExistAndHasChild(node, "Local"))
            {
                if(ExistAndHasChild(node, "Git"))
                {
                    ssLOG_ERROR("DependencySource: Both Git and Local sources found");
                    return false;
                }
                
                LocalSource localSource;
                YAML::ConstNodePtr localNode = node->GetMapValueNode("Local");
                if(!localSource.ParseYAML_Node(localNode))
                    return false;
                Source = localSource;
                return true;
            }
            //If no source is found, we need to check if it's an imported source. 
            //If so, we assume it's a local source with path "./"
            else if(!ImportPath.empty())
            {
                LocalSource localSource;
                localSource.Path = "./";
                Source = localSource;
                return true;
            }
            
            ssLOG_ERROR("DependencySource: Neither Git nor Local source found");
            return false;
        }

        inline std::string ToString(std::string indentation) const
        {
            std::string out;
            if(!ImportPath.empty())
                out += indentation + "ImportPath: " + GetEscapedYAMLString(ImportPath.string()) + "\n";
            
            if(mpark::get_if<GitSource>(&Source))
            {
                const GitSource* git = mpark::get_if<GitSource>(&Source);
                out += git->ToString(indentation);
            }
            else if(mpark::get_if<LocalSource>(&Source))
            {
                const LocalSource* local = mpark::get_if<LocalSource>(&Source);
                out += local->ToString(indentation);
            }
            else
            {
                ssLOG_ERROR("Invalid DependencySource type");
                return "";
            }
            return out;
        }

        inline bool Equals(const DependencySource& other) const
        {
            if(ImportPath != other.ImportPath)
                return false;
                
            if(mpark::get_if<GitSource>(&Source))
            {
                if(mpark::get_if<GitSource>(&other.Source))
                {
                    const GitSource* git = mpark::get_if<GitSource>(&Source);
                    const GitSource* otherGit = mpark::get_if<GitSource>(&other.Source);
                    return git->Equals(*otherGit);
                }
                else
                    return false;
            }
            else if(mpark::get_if<LocalSource>(&Source))
            {
                if(mpark::get_if<LocalSource>(&other.Source))
                {
                    const LocalSource* local = mpark::get_if<LocalSource>(&Source);
                    const LocalSource* otherLocal = mpark::get_if<LocalSource>(&other.Source);
                    return local->Equals(*otherLocal);
                }
                else
                    return false;
            }
            
            ssLOG_ERROR("Invalid DependencySource type");
            return false;
        }
    };
}
}

#endif
