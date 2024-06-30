#ifndef RUNCPP2_DATA_STAGE_INFO_HPP
#define RUNCPP2_DATA_STAGE_INFO_HPP

#include "runcpp2/Data/ParseCommon.hpp"
#include "ryml.hpp"

#include <vector>
#include <string>
#include <unordered_map>

namespace runcpp2
{
    namespace Data
    {
        class StageInfo
        {
            public:
                std::unordered_map<PlatformName, std::string> PreRun;
                std::string CheckExistence;
                
                struct OutputTypeInfo
                {
                    std::string Flags;
                    std::string Executable;
                };
                
                struct 
                {
                    std::unordered_map<PlatformName, OutputTypeInfo> Executable;
                    std::unordered_map<PlatformName, OutputTypeInfo> Static;
                    std::unordered_map<PlatformName, OutputTypeInfo> Shared;
                } OutputTypes;
                
                struct RunPart
                {
                    enum class RunType
                    {
                        ONCE,
                        REPEATS
                    };
                    
                    RunType Type;
                    std::string CommandPart;
                };
                
                std::unordered_map<PlatformName, std::vector<RunPart>> RunParts;
                
                bool ParseYAML_Node(ryml::ConstNodeRef& node, std::string outputTypeKeyName);
                std::string ToString(std::string indentation) const;
        };
    }
}

#endif
