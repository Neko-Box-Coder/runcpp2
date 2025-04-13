#include "runcpp2/Data/StageInfo.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "runcpp2/PlatformUtil.hpp"

#include "ssLogger/ssLog.hpp"

namespace
{
    bool ParseOutputTypes(  ryml::ConstNodeRef outputTypesSubNode, 
                            std::vector<runcpp2::NodeRequirement> requirements, 
                            std::unordered_map< PlatformName, 
                                                runcpp2::Data::StageInfo::OutputTypeInfo>& outInfos)
    {
        ssLOG_FUNC_DEBUG();
        using namespace runcpp2;
        using namespace runcpp2::Data;
        
        std::string subNodeName = runcpp2::GetKey(outputTypesSubNode);
        
        for(int i = 0; i < outputTypesSubNode.num_children(); ++i)
        {
            ryml::ConstNodeRef currentPlatformNode = outputTypesSubNode[i];
            std::string platformName = runcpp2::GetKey(currentPlatformNode);

            if(!CheckNodeRequirements(currentPlatformNode, requirements))
            {
                ssLOG_ERROR("Failed to parse outputTypesSubNode in " << subNodeName << 
                            " for platform " << platformName);
                return false;
            }

            outInfos[platformName].Flags = runcpp2::GetValue(currentPlatformNode["Flags"]);
            outInfos[platformName].Executable = runcpp2::GetValue(currentPlatformNode["Executable"]);
            
            //RunParts
            if(!ExistAndHasChild(currentPlatformNode, "RunParts"))
            {
                ssLOG_ERROR("Failed to parse RunParts");
                return false;
            }
            
            ryml::ConstNodeRef runPartsNode = currentPlatformNode["RunParts"];
            for(int j = 0; j < runPartsNode.num_children(); ++j)
            {
                ryml::ConstNodeRef currentPartNode = runPartsNode[j];
                
                std::vector<NodeRequirement> currentRunPartRequirements =
                {
                    NodeRequirement("Type", ryml::NodeType_e::KEYVAL, true, false),
                    NodeRequirement("CommandPart", ryml::NodeType_e::KEYVAL, true, false)
                };
                
                if(!CheckNodeRequirements(currentPartNode, currentRunPartRequirements))
                {
                    ssLOG_ERROR("Failed to parse RunPart at index " << j << " for " << platformName);
                    return false;
                }
                
                outInfos[platformName].RunParts.push_back({});
                std::string currentType = runcpp2::GetValue(currentPartNode["Type"]);
                
                static_assert(  static_cast<int>(StageInfo::RunPart::RunType::COUNT) == 2, 
                                "Add new RunType");
                
                if(currentType == "Once")
                    outInfos[platformName].RunParts.back().Type = StageInfo::RunPart::RunType::ONCE;
                else if(currentType == "Repeats")
                    outInfos[platformName].RunParts.back().Type = StageInfo::RunPart::RunType::REPEATS;
                else
                {
                    ssLOG_WARNING(  "Invalid RunPart type " << currentType << " at index " << j << 
                                    " for " << platformName);
                    ssLOG_DEBUG("Defaulting to Once");
                    outInfos[platformName].RunParts.back().Type = StageInfo::RunPart::RunType::ONCE;
                }
                
                outInfos[platformName].RunParts.back().CommandPart = 
                    GetValue(currentPartNode["CommandPart"]);
            }
            
            //Setup
            if(ExistAndHasChild(currentPlatformNode, "Setup"))
            {
                ryml::ConstNodeRef setupNode = currentPlatformNode["Setup"];
                for(int j = 0; j < setupNode.num_children(); ++j)
                    outInfos[platformName].Setup.push_back(GetValue(setupNode[j]));
            }
            
            //Cleanup
            if(ExistAndHasChild(currentPlatformNode, "Cleanup"))
            {
                ryml::ConstNodeRef cleanupNode = currentPlatformNode["Cleanup"];
                for(int j = 0; j < cleanupNode.num_children(); ++j)
                    outInfos[platformName].Cleanup.push_back(GetValue(cleanupNode[j]));
            }
            
            //ExpectedOutputFiles
            ryml::ConstNodeRef expectedOutputFilesNode = currentPlatformNode["ExpectedOutputFiles"];
            for(int j = 0; j < expectedOutputFilesNode.num_children(); ++j)
            {
                outInfos[platformName]  .ExpectedOutputFiles
                                        .push_back(GetValue(expectedOutputFilesNode[j]));
            }
        }
        
        return true;
    }
    
    using OutputTypeInfo = runcpp2::Data::StageInfo::OutputTypeInfo;
    
    void OutputTypeInfoMapToString( const std::string& indentation,  
                                    const std::unordered_map<   PlatformName, 
                                                                OutputTypeInfo>& toStringMap,
                                    std::string& outString)
    {
        using namespace runcpp2;
        using namespace runcpp2::Data;
        
        for(auto it = toStringMap.begin(); it != toStringMap.end(); ++it)
        {
            outString += indentation + "    " + it->first + ": \n";
            outString +=    indentation + "        Flags: " + 
                            GetEscapedYAMLString(it->second.Flags) + "\n";
            outString +=    indentation + "        Executable: " + 
                            GetEscapedYAMLString(it->second.Executable) + "\n";
            outString += indentation + "        RunParts: \n";
            for(int i = 0; i < it->second.RunParts.size(); ++i)
            {
                static_assert(  static_cast<int>(StageInfo::RunPart::RunType::COUNT) == 2, 
                                "Add new RunType");
                
                StageInfo::RunPart::RunType currentType = it->second.RunParts.at(i).Type;
                outString +=    indentation + 
                                "        -   Type: " + 
                                (currentType == StageInfo::RunPart::RunType::ONCE ? 
                                "Once" : 
                                "Repeats") + 
                                "\n";
                
                outString +=    indentation + "            CommandPart: " + 
                                GetEscapedYAMLString(it->second.RunParts.at(i).CommandPart) + "\n";
            }
            
            outString += indentation + "        ExpectedOutputFiles: \n";
            for(int i = 0; i < it->second.ExpectedOutputFiles.size(); ++i)
            {
                outString +=    indentation + "        -   " + 
                                GetEscapedYAMLString(it->second.ExpectedOutputFiles.at(i)) + "\n";
            }
            
            if(!it->second.Setup.empty())
            {
                outString += indentation + "        Setup: \n";
                for(int i = 0; i < it->second.Setup.size(); ++i)
                {
                    outString +=    indentation + "        -   " + 
                                    GetEscapedYAMLString(it->second.Setup.at(i)) + "\n";
                }
            }
            
            if(!it->second.Cleanup.empty())
            {
                outString += indentation + "        Cleanup: \n";
                for(int i = 0; i < it->second.Cleanup.size(); ++i)
                {
                    outString +=    indentation + "        -   " + 
                                    GetEscapedYAMLString(it->second.Cleanup.at(i)) + "\n";
                }
            }
        }
    }
    
    //NOTE: This extracts substitutions and also allow escapes to happen for substitution characters.
    //      To escape a substitution character, just repeat it. (i.e. {{text}} will be escaped as {text})
    void GetEscapedStringAndExtractSubstitutions(   const std::string& processString, 
                                                    std::string& outEscapedString,
                                                    std::vector<std::string>& outFoundSubstitutions,
                                                    std::vector<int>& outFoundLocations,
                                                    std::vector<int>& outFoundLength)
    {
        ssLOG_FUNC_DEBUG();
        
        outEscapedString.clear();
        std::string currentSubstitution;
        
        int lastOpenBracketIndex = -1;
        for(int i = 0; i < processString.size(); ++i)
        {
            if(processString[i] == '{')
            {
                if(i == processString.size() - 1)
                {
                    if(lastOpenBracketIndex == -1 || lastOpenBracketIndex != i - 1)
                        ssLOG_WARNING("Unescaped { at the end: " << processString);
                    
                    outEscapedString += '{';
                    continue;
                }
                
                //If we have opening bracket for the next character, 
                //this is an escaping character
                if(processString[i + 1] == '{')
                {
                    outEscapedString += '{';
                    if(lastOpenBracketIndex != -1)
                        currentSubstitution += '{';
                    
                    ++i;
                    continue;
                }
                
                if(lastOpenBracketIndex != -1)
                {
                    ssLOG_WARNING(  "Unescaped { at index " << lastOpenBracketIndex << 
                                    ": " << processString);
                }
                
                lastOpenBracketIndex = i;
                currentSubstitution = "{";
                outEscapedString += '{';
            }
            else if(processString[i] == '}')
            {
                //If we have closing bracket for the next character, 
                //this is an escaping character
                if(i < processString.size() - 1 && processString[i + 1] == '}')
                {
                    outEscapedString += '}';
                    if(lastOpenBracketIndex != -1)
                        currentSubstitution += '}';
                    
                    ++i;
                    continue;
                }
                
                //If there's no open bracket, give warning
                if(lastOpenBracketIndex == -1)
                {
                    ssLOG_WARNING("Unescaped } at index " << i << ": " << processString);
                    continue;
                }
                
                //Add substitution
                outEscapedString += '}';
                currentSubstitution += '}';
                ssLOG_DEBUG("Substitution " << currentSubstitution << " found");
                outFoundSubstitutions.push_back(currentSubstitution);
                outFoundLocations.push_back(outEscapedString.size() - currentSubstitution.size());
                outFoundLength.push_back(currentSubstitution.size());
                
                //Reset
                lastOpenBracketIndex = -1;
                currentSubstitution.clear();
            }
            //Normal characters
            else
            {
                outEscapedString += processString[i];
                if(lastOpenBracketIndex != -1)
                    currentSubstitution += processString[i];
            }
        }
    }
    
    bool PerformSubstituionsWithInfo(   const runcpp2::Data::StageInfo::SubstitutionMap& substitutionMap, 
                                        const std::string& escapedString,
                                        const std::vector<std::string>& foundSubstitutions,
                                        const std::vector<int>& substitutionsLocations,
                                        const std::vector<int>& substitutionsLengths,
                                        std::string& inOutSubstitutedString,
                                        int substituteValueIndex = 0)
    {
        ssLOG_FUNC_DEBUG();

        inOutSubstitutedString = escapedString;
        
        for(int i = foundSubstitutions.size() - 1; i >= 0; --i)
        {
            const std::string& substitution = foundSubstitutions.at(i);
            if(substitutionMap.count(substitution) == 0)
            {
                ssLOG_ERROR("INTERNAL ERROR, missing substitution value for \"" << substitution << "\"");
                return false;
            }
            
            std::string currentValue = substitutionMap.at(substitution).at(substituteValueIndex);
            
            //Escape escapes character at the end if any
            {
                std::vector<char> escapeChars = {'\\'};
                #ifdef _WIN32
                    escapeChars.emplace_back('^');
                #endif
                if(!currentValue.empty())
                {
                    int currentValIndex = currentValue.size();
                    while(currentValIndex > 0)
                    {
                        --currentValIndex;
                        bool found = false;
                        for(int j = 0; j < escapeChars.size(); ++j)
                        {
                            if(currentValue[currentValIndex] == escapeChars[j])
                            {
                                found = true;
                                break;
                            }
                        }
                        
                        if(!found)
                        {
                            ++currentValIndex;
                            break;
                        }
                    }
                    
                    if(currentValIndex < currentValue.size())
                    {
                        const std::string foundEscapes = currentValue.substr(currentValIndex);
                        std::string newEndEscapes;
                        //Just repeat the escape characters
                        for(int j = 0; j < foundEscapes.size(); ++j)
                        {
                            newEndEscapes.push_back(foundEscapes[j]);
                            newEndEscapes.push_back(foundEscapes[j]);
                        }
                    
                        currentValue = currentValue.substr(0, currentValIndex) + newEndEscapes;
                    }
                }
            }
            
            ssLOG_DEBUG("Replacing \"" << substitution << "\" with \"" << currentValue << 
                        "\" in \"" << escapedString << "\"");
            
            inOutSubstitutedString.replace( substitutionsLocations.at(i), 
                                            substitutionsLengths.at(i), 
                                            currentValue);
        }
        
        return true;
    }
}

bool runcpp2::Data::StageInfo::PerformSubstituions( const SubstitutionMap& substitutionMap, 
                                                    std::string& inOutSubstitutedString) const
{
    std::string escapedString;
    std::vector<std::string> foundSubstitutions;
    std::vector<int> substitutionsLocations;
    std::vector<int> substitutionsLengths;
    
    GetEscapedStringAndExtractSubstitutions(inOutSubstitutedString, 
                                            escapedString,
                                            foundSubstitutions,
                                            substitutionsLocations,
                                            substitutionsLengths);
    
    if( foundSubstitutions.size() != substitutionsLocations.size() ||
        foundSubstitutions.size() != substitutionsLengths.size())
    {
        ssLOG_ERROR("Substitution size mismatch");
        ssLOG_ERROR("foundSubstitutions.size(): " << foundSubstitutions.size());
        ssLOG_ERROR("substitutionsLocations.size(): " << substitutionsLocations.size());
        ssLOG_ERROR("substitutionsLengths.size(): " << substitutionsLengths.size());
        return false;
    }
    
    return PerformSubstituionsWithInfo( substitutionMap, 
                                        escapedString, 
                                        foundSubstitutions, 
                                        substitutionsLocations, 
                                        substitutionsLengths,
                                        inOutSubstitutedString);
}

bool runcpp2::Data::StageInfo::ConstructCommand(const SubstitutionMap& substitutionMap, 
                                                const bool isExecutable,
                                                const BuildType buildType,
                                                std::string& outCommand) const
{
    ssLOG_FUNC_DEBUG();   
    
    
    static_assert(static_cast<int>(BuildType::COUNT) == 4, "Add new type to be processed");
    const std::unordered_map<PlatformName, OutputTypeInfo>& currentOutputTypeMap = 
        isExecutable ? 
        OutputTypes.Executable :
        (
            buildType == BuildType::STATIC ? 
            OutputTypes.Static : 
            (
                buildType == BuildType::EXECUTABLE ? 
                OutputTypes.ExecutableShared : 
                OutputTypes.Shared
            )
        );
    
    if(!runcpp2::HasValueFromPlatformMap(currentOutputTypeMap))
    {
        ssLOG_ERROR("Failed to find RunParts for current platform");
        return false;
    }
    
    const OutputTypeInfo* rawOutputTypeInfo = 
        runcpp2::GetValueFromPlatformMap(currentOutputTypeMap);
    
    if(rawOutputTypeInfo == nullptr)
    {
        ssLOG_ERROR("Failed to retrieve OutputTypeInfo");
        return false;
    }
    
    const std::vector<RunPart>& currentRunParts = (*rawOutputTypeInfo).RunParts;
    outCommand.clear();
    
    for(int i = 0; i < currentRunParts.size(); ++i)
    {
        ssLOG_DEBUG("Parsing run part at index: " << i);
        ssLOG_DEBUG("Which is: \"" << currentRunParts.at(i).CommandPart << "\"");
        
        std::string currentEscapedPart;
        std::vector<std::string> substitutionsInCurrentPart;
        std::vector<int> substitutionsLocations;
        std::vector<int> substitutionsLengths;
        
        GetEscapedStringAndExtractSubstitutions(currentRunParts.at(i).CommandPart, 
                                                currentEscapedPart,
                                                substitutionsInCurrentPart,
                                                substitutionsLocations,
                                                substitutionsLengths);
        
        if( substitutionsInCurrentPart.size() != substitutionsLocations.size() ||
            substitutionsInCurrentPart.size() != substitutionsLengths.size())
        {
            ssLOG_ERROR("Substitution size mismatch");
            ssLOG_ERROR("substitutionsInCurrentPart.size(): " << substitutionsInCurrentPart.size());
            ssLOG_ERROR("substitutionsLocations.size(): " << substitutionsLocations.size());
            ssLOG_ERROR("substitutionsLengths.size(): " << substitutionsLengths.size());
            return false;
        }
        
        static_assert(  static_cast<int>(RunPart::RunType::COUNT) == 2, 
                        "Update parsing for new runtype");
        
        //Check RunType::ONCE if all the substitutions are present
        if(currentRunParts.at(i).Type == RunPart::RunType::ONCE)
        {
            std::string substitutedPart;
            
            if(!PerformSubstituionsWithInfo(substitutionMap, 
                                            currentEscapedPart, 
                                            substitutionsInCurrentPart, 
                                            substitutionsLocations, 
                                            substitutionsLengths,
                                            substitutedPart))
            {
                return false;
            }
            
            outCommand += substitutedPart;
        }
        //Check RunType::REPEATS, all the substitutions must have the same number of values.
        //Otherwise, bail
        else
        {
            int firstCount = -1;
            
            //If there are no substitution, report this
            if(substitutionsInCurrentPart.empty())
            {
                ssLOG_ERROR("There are no substitutions found in " << currentEscapedPart << 
                            " but it is set to be of RunType::REPEATS.");
                
                ssLOG_ERROR("Substitutions are needed to determine " << 
                            "how many times to repeat this part");
                
                return false;
            }
            
            //Check all the substitution found in the run part
            for(int j = 0; j < substitutionsInCurrentPart.size(); ++j)
            {
                if(substitutionMap.count(substitutionsInCurrentPart.at(j)) == 0)
                {
                    ssLOG_DEBUG("No substitution found for " << substitutionsInCurrentPart.at(j) << 
                                " in " << currentRunParts.at(i).CommandPart);
                    
                    ssLOG_DEBUG("Current run part is type repeat, skipping to next");
                    continue;
                }
                
                int substitutionValuesCount = 
                    substitutionMap.at(substitutionsInCurrentPart.at(j)).size();
                
                if(firstCount == -1)
                    firstCount = substitutionValuesCount;
                else if(substitutionValuesCount != firstCount)
                {
                    ssLOG_ERROR("The number of substitution values found for " << 
                                substitutionsInCurrentPart.at(j) << " which is " << 
                                substitutionValuesCount << " does not match " <<
                                "the number of other substitution values which is " << firstCount);

                    return false;
                }
            }
            
            //Once we agreed on how many repeats we need to do based on 
            //the substitution values count, we can then do substitution.
            for(int j = 0; j < firstCount; ++j)
            {
                std::string substitutedPart;
                            
                if(!PerformSubstituionsWithInfo(substitutionMap, 
                                                currentEscapedPart, 
                                                substitutionsInCurrentPart, 
                                                substitutionsLocations, 
                                                substitutionsLengths,
                                                substitutedPart,
                                                j))
                {
                    return false;
                }
                
                outCommand += substitutedPart;
            }
        }
    }
    
    return true;
}


bool runcpp2::Data::StageInfo::ParseYAML_Node(  ryml::ConstNodeRef node, 
                                                std::string outputTypeKeyName)
{
    ssLOG_FUNC_DEBUG();
    
    INTERNAL_RUNCPP2_SAFE_START();
    std::vector<NodeRequirement> requirements =
    {
        NodeRequirement("PreRun", ryml::NodeType_e::MAP, false, true),
        NodeRequirement("CheckExistence", ryml::NodeType_e::KEYMAP, true, false),
        NodeRequirement(outputTypeKeyName, ryml::NodeType_e::MAP, true, false)
    };

    if(!CheckNodeRequirements(node, requirements))
    {
        ssLOG_ERROR("StageInfo: Failed to meet requirements");
        return false;
    }

    if(ExistAndHasChild(node, "PreRun"))
    {
        ryml::ConstNodeRef preRunNode = node["PreRun"];
        for(int i = 0; i < preRunNode.num_children(); ++i)
        {
            std::string key = GetKey(preRunNode[i]);
            std::string value = GetValue(preRunNode[i]);
            PreRun[key] = value;
        }
    }
    
    //CheckExistence
    {
        ryml::ConstNodeRef checkExistenceNode = node["CheckExistence"];
        for(int i = 0; i < checkExistenceNode.num_children(); ++i)
        {
            std::string key = GetKey(checkExistenceNode[i]);
            std::string value = GetValue(checkExistenceNode[i]);
            CheckExistence[key] = value;
        }
    }
    
    //OutputTypes
    {
        if(!ExistAndHasChild(node, outputTypeKeyName))
        {
            ssLOG_ERROR("Failed to parse " << outputTypeKeyName);
            return false;
        }
        
        ryml::ConstNodeRef outputTypeNode = node[outputTypeKeyName.c_str()];
        std::vector<NodeRequirement> outputTypeRequirements =
        {
            NodeRequirement("Executable", ryml::NodeType_e::MAP, true, false),
            NodeRequirement("ExecutableShared", ryml::NodeType_e::MAP, true, false),
            NodeRequirement("Static", ryml::NodeType_e::MAP, true, false),
            NodeRequirement("Shared", ryml::NodeType_e::MAP, true, false)
        };
        
        if(!CheckNodeRequirements(outputTypeNode, outputTypeRequirements))
        {
            ssLOG_ERROR("Failed to parse " << outputTypeKeyName);
            return false;
        }

        std::vector<NodeRequirement> outputTypeInfoRequirements =
        {
            NodeRequirement("Flags", ryml::NodeType_e::KEYVAL, true, true),
            NodeRequirement("Executable", ryml::NodeType_e::KEYVAL, true, false),
            NodeRequirement("RunParts", ryml::NodeType_e::SEQ, true, false),
            NodeRequirement("Setup", ryml::NodeType_e::SEQ, false, false),
            NodeRequirement("Cleanup", ryml::NodeType_e::SEQ, false, false),
            NodeRequirement("ExpectedOutputFiles", ryml::NodeType_e::SEQ, true, false),

        };
        ryml::ConstNodeRef executableNode = outputTypeNode["Executable"];
        if(!ParseOutputTypes(executableNode, outputTypeInfoRequirements, OutputTypes.Executable))
            return false;
        
        ryml::ConstNodeRef executableSharedNode = outputTypeNode["ExecutableShared"];
        if(!ParseOutputTypes(   executableSharedNode, 
                                outputTypeInfoRequirements, 
                                OutputTypes.ExecutableShared))
        {
            return false;
        }
        
        ryml::ConstNodeRef staticNode = outputTypeNode["Static"];
        if(!ParseOutputTypes(staticNode, outputTypeInfoRequirements, OutputTypes.Static))
            return false;
        
        ryml::ConstNodeRef sharedNode = outputTypeNode["Shared"];
        if(!ParseOutputTypes(sharedNode, outputTypeInfoRequirements, OutputTypes.Shared))
            return false;
    }
    
    return true;

    INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
}

std::string runcpp2::Data::StageInfo::ToString( std::string indentation, 
                                                std::string outputTypeKeyName) const
{
    ssLOG_FUNC_DEBUG();

    std::string out;
    
    if(!PreRun.empty())
    {
        out += indentation + "PreRun:\n";
        for(auto it = PreRun.begin(); it != PreRun.end(); ++it)
            out += indentation + "    " + it->first + ": " + GetEscapedYAMLString(it->second) + "\n";
    }
    
    out += indentation + "CheckExistence:\n";
    for(auto it = CheckExistence.begin(); it != CheckExistence.end(); ++it)
        out += indentation + "    " + it->first + ": " + GetEscapedYAMLString(it->second) + "\n";
    
    out += indentation + outputTypeKeyName + ":\n";
    
    out += indentation + "    Executable: \n";
    OutputTypeInfoMapToString(indentation + "    ", OutputTypes.Executable, out);
    
    out += indentation + "    ExecutableShared: \n";
    OutputTypeInfoMapToString(indentation + "    ", OutputTypes.ExecutableShared, out);
    
    out += indentation + "    Static: \n";
    OutputTypeInfoMapToString(indentation + "    ", OutputTypes.Static, out);
    
    out += indentation + "    Shared: \n";
    OutputTypeInfoMapToString(indentation + "    ", OutputTypes.Shared, out);

    return out;
}

bool runcpp2::Data::StageInfo::Equals(const StageInfo& other) const
{
    if( PreRun.size() != other.PreRun.size() ||
        CheckExistence.size() != other.CheckExistence.size())
    {
        return false;   
    }
    
    for(const auto& it : PreRun)
    {
        if(other.PreRun.count(it.first) == 0 || other.PreRun.at(it.first) != it.second)
            return false;
    }

    for(const auto& it : CheckExistence)
    {
        if( other.CheckExistence.count(it.first) == 0 || 
            other.CheckExistence.at(it.first) != it.second)
        {
            return false;
        }
    }

    auto compareOutputTypeInfoMaps = 
        []( const std::unordered_map<PlatformName, OutputTypeInfo>& a,
            const std::unordered_map<PlatformName, OutputTypeInfo>& b) -> bool
        {
            if(a.size() != b.size())
                return false;

            for(const auto& it : a)
            {
                if(b.count(it.first) == 0)
                    return false;

                const OutputTypeInfo& otherInfo = b.at(it.first);
                const OutputTypeInfo& info = it.second;

                if( info.Flags != otherInfo.Flags ||
                    info.Executable != otherInfo.Executable ||
                    info.Setup != otherInfo.Setup ||
                    info.Cleanup != otherInfo.Cleanup)
                {
                    return false;
                }

                if(info.RunParts.size() != otherInfo.RunParts.size())
                    return false;

                for(size_t i = 0; i < info.RunParts.size(); ++i)
                {
                    if( info.RunParts[i].Type != otherInfo.RunParts[i].Type ||
                        info.RunParts[i].CommandPart != otherInfo.RunParts[i].CommandPart)
                    {
                        return false;
                    }
                }
            }
            return true;
        };

    if( !compareOutputTypeInfoMaps(OutputTypes.Executable, other.OutputTypes.Executable) ||
        !compareOutputTypeInfoMaps(OutputTypes.ExecutableShared, other.OutputTypes.ExecutableShared) ||
        !compareOutputTypeInfoMaps(OutputTypes.Static, other.OutputTypes.Static) ||
        !compareOutputTypeInfoMaps(OutputTypes.Shared, other.OutputTypes.Shared))
    {
        return false;
    }

    return true;
}
