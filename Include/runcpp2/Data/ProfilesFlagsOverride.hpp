#ifndef RUNCPP2_DATA_PROFILES_FLAGS_OVERRIDE_HPP
#define RUNCPP2_DATA_PROFILES_FLAGS_OVERRIDE_HPP

#include "FlagsOverrideInfo.hpp"
#include "runcpp2/Data/ParseCommon.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "ssLogger/ssLog.hpp"

#include <unordered_map>

namespace runcpp2
{
namespace Data
{
    struct ProfilesFlagsOverride
    {
        std::unordered_map<ProfileName, FlagsOverrideInfo> FlagsOverrides;
        
        inline bool ParseYAML_Node(YAML::ConstNodePtr node)
        {
            ssLOG_FUNC_DEBUG();
            
            if(!node->IsMap())
            {
                ssLOG_ERROR("ProfilesFlagsOverrides: Not a map type");
                return false;
            }
            
            for(int i = 0; i < node->GetChildrenCount(); ++i)
            {
                ProfileName profile = node->GetMapKeyScalarAt<ProfileName>(i).DS_TRY_ACT(return false);
                if(!ParseYAML_NodeWithProfile(node->GetMapValueNodeAt(i), profile))
                    return false;
            }
            
            return true;
        }

        inline bool ParseYAML_NodeWithProfile(YAML::ConstNodePtr node, ProfileName profile)
        {
            ssLOG_FUNC_DEBUG();
            
            if(!node->IsMap())
            {
                ssLOG_ERROR("ProfilesFlagsOverrides: FlagsOverrideInfo type requires a map");
                return false;
            }
            
            FlagsOverrideInfo flags;
            YAML::ConstNodePtr flagsOverrideNode = node;
            
            if(!flags.ParseYAML_Node(flagsOverrideNode))
            {
                ssLOG_ERROR("ProfilesFlagsOverrides: Unable to parse FlagsOverride.");
                return false;
            }
            
            FlagsOverrides[profile] = flags;
            
            return true;
        }

        inline bool IsYAML_NodeParsableAsDefault(YAML::ConstNodePtr node) const
        {
            ssLOG_FUNC_DEBUG();
            FlagsOverrideInfo flags;
            return flags.IsYAML_NodeParsableAsDefault(node);
        }

        inline std::string ToString(std::string indentation) const
        {
            std::string out;
            
            if(FlagsOverrides.empty())
                return out;
            
            for(auto it = FlagsOverrides.begin(); it != FlagsOverrides.end(); ++it)
            {
                out += indentation + it->first + ":\n";
                out += it->second.ToString(indentation + "    ");
            }
            
            return out;
        }

        inline bool Equals(const ProfilesFlagsOverride& other) const
        {
            if(FlagsOverrides.size() != other.FlagsOverrides.size())
                return false;

            for(const auto& it : FlagsOverrides)
            {
                if( other.FlagsOverrides.count(it.first) == 0 || 
                    !other.FlagsOverrides.at(it.first).Equals(it.second))
                {
                    return false;
                }
            }
            
            return true;
        }
    };
}
}

#endif
