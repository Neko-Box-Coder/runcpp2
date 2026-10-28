#ifndef RUNCPP2_DATA_PROCESS_PATHS_HPP
#define RUNCPP2_DATA_PROCESS_PATHS_HPP

#include "runcpp2/Data/ParseCommon.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "runcpp2/LibYAML_Wrapper.hpp"

#include "ssLogger/ssLog.hpp"

#if !defined(NOMINMAX)
    #define NOMINMAX 1
#endif
#include "ghc/filesystem.hpp"
#include "DSResult/DSResult.hpp"

#include <string>
#include <utility>
#include <vector>

namespace runcpp2
{
namespace Data
{
    struct ProcessPaths
    {
        std::vector<ghc::filesystem::path> Paths;
        
        inline bool ParseYAML_Node(YAML::ConstNodePtr node)
        {
            ssLOG_FUNC_DEBUG();
            
            if(!node->IsSequence())
            {
                ssLOG_ERROR("ProcessPaths: Not a sequence type");
                return false;
            }
            
            for(int i = 0; i < node->GetChildrenCount(); ++i)
            {
                std::string path = node ->GetSequenceChildScalar<std::string>(i)
                                        .DS_TRY_ACT(return false);
                Paths.push_back(path);
            }
            
            return true;
        }

        inline bool IsYAML_NodeParsableAsDefault(YAML::ConstNodePtr node) const
        {
            ssLOG_FUNC_DEBUG();
            return node->IsSequence();
        }

        inline std::string ToString(std::string indentation) const
        {
            std::string out;
            
            if(Paths.empty())
            {
                out += indentation + "[]\n";
                return out;
            }
            
            for(int i = 0; i < Paths.size(); ++i)
                out +=  indentation + "-   " + GetEscapedYAMLString(Paths.at(i).string()) + "\n";
            
            return out;
        }

        inline bool Equals(const ProfilesProcessPaths& other) const
        {
            if(Paths.size() != other.Paths.size())
                return false;
            
            for(const auto& it : Paths)
            {
                if(other.Paths.count(it.first) == 0 || other.Paths.at(it.first) != it.second)
                    return false;
            }
            
            return true;
        }
    };
}
}

#endif
