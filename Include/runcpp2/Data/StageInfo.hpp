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
                std::unordered_map<PlatformName, std::string> CheckExistence;
                
                struct RunPart
                {
                    enum class RunType
                    {
                        ONCE,
                        REPEATS,
                        COUNT
                    };
                    
                    RunType Type;
                    std::string CommandPart;
                };
                
                struct OutputTypeInfo
                {
                    std::string Flags;
                    std::string Executable;
                    
                    std::vector<std::string> Setup;
                    std::vector<std::string> Cleanup;
                    std::vector<RunPart> RunParts;
                };
                
                struct 
                {
                    std::unordered_map<PlatformName, OutputTypeInfo> Executable;
                    std::unordered_map<PlatformName, OutputTypeInfo> Static;
                    std::unordered_map<PlatformName, OutputTypeInfo> Shared;
                } OutputTypes;
                
                using SubstitutionMap = std::unordered_map<std::string, std::vector<std::string>>;
                
                bool PerformSubstituions(   const SubstitutionMap& substitutionMap, 
                                            std::string& inOutSubstitutedString) const;
                
                bool ConstructCommand(  const SubstitutionMap& substitutionMap, 
                                        bool isExecutable,
                                        std::string& outCommand) const;
                
                bool ParseYAML_Node(ryml::ConstNodeRef& node, std::string outputTypeKeyName);
                std::string ToString(   std::string indentation,
                                        std::string outputTypeKeyName) const;
                bool Equals(const StageInfo& other) const;
        };
    }
}

#endif
