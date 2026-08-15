#ifndef RUNCPP2_DATA_PROFILE_HPP
#define RUNCPP2_DATA_PROFILE_HPP

#include "runcpp2/Data/ParseCommon.hpp"
#include "runcpp2/Data/FilesTypesInfo.hpp"
#include "runcpp2/Data/StageInfo.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "runcpp2/LibYAML_Wrapper.hpp"
#include "runcpp2/Data/ParameterValue.hpp"
#include "runcpp2/ParameterUtil.hpp"

#include "DSResult/DSResult.hpp"

#include "ssLogger/ssLog.hpp"

#include <string>
#include <utility>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace runcpp2
{
namespace Data
{
    struct Profile
    {
        std::string Name;
        
        std::unordered_set<std::string> NameAliases;
        std::unordered_set<std::string> FileExtensions;
        std::unordered_set<std::string> Languages;
        std::unordered_map<std::string, ParameterValue> Parameters;
        std::unordered_map<std::string, std::string> Variables;
        std::unordered_map<PlatformName, std::vector<std::string>> Setup;
        std::unordered_map<PlatformName, std::vector<std::string>> Cleanup;
        FilesTypesInfo FilesTypes;
        
        StageInfo Compiler;
        StageInfo Linker;
        
        inline void GetNames(std::vector<std::string>& outNames) const
        {
            outNames.clear();
            outNames.push_back(Name);
            for(const auto& alias : NameAliases)
                outNames.push_back(alias);
            
            //Special name all that applies to all profile
            outNames.push_back("DefaultProfile");
        }

        inline static void 
        PopulateCompilingLinkingParams( std::unordered_map
                                        <
                                            std::string, 
                                            std::vector<std::string>
                                        >& outSubstitutionMap)
        {
            #define INTERN_ADD_MAP(x) outSubstitutionMap[x] = {x};
            
            INTERN_ADD_MAP("{Stage.SharedLibraryFile.Prefix}");
            INTERN_ADD_MAP("{Stage.SharedLinkFile.Prefix}");
            INTERN_ADD_MAP("{Stage.StaticLinkFile.Prefix}");
            INTERN_ADD_MAP("{Stage.ObjectLinkFile.Prefix}");
            INTERN_ADD_MAP("{Stage.DebugSymbolFile.Prefix}");
            INTERN_ADD_MAP("{Stage.SharedLibraryFile.Extension}");
            INTERN_ADD_MAP("{Stage.SharedLinkFile.Extension}");
            INTERN_ADD_MAP("{Stage.StaticLinkFile.Extension}");
            INTERN_ADD_MAP("{Stage.ObjectLinkFile.Extension}");
            INTERN_ADD_MAP("{Stage.DebugSymbolFile.Extension}");
            INTERN_ADD_MAP("{Stage.Executable}");
            INTERN_ADD_MAP("{Stage.CompileFlags}");
            INTERN_ADD_MAP("{Stage.Input.Name}");
            INTERN_ADD_MAP("{Stage.Input.Extension}");
            INTERN_ADD_MAP("{Stage.Input.Directory}");
            INTERN_ADD_MAP("{Stage.Input.Path}");
            INTERN_ADD_MAP("{Stage.Output.Directory}");
            INTERN_ADD_MAP("{Stage.DefineNameOnly}");
            INTERN_ADD_MAP("{Stage.DefineName}");
            INTERN_ADD_MAP("{Stage.DefineValue}");
            INTERN_ADD_MAP("{Stage.IncludeDirectory.Path}");
            INTERN_ADD_MAP("{Stage.IncludeDirectory.Source.Path}");
            INTERN_ADD_MAP("{Stage.IncludeDirectory.Dep.Path}");
            INTERN_ADD_MAP("{Stage.SharedLibraryFile.Prefix}");
            INTERN_ADD_MAP("{Stage.SharedLinkFile.Prefix}");
            INTERN_ADD_MAP("{Stage.StaticLinkFile.Prefix}");
            INTERN_ADD_MAP("{Stage.ObjectLinkFile.Prefix}");
            INTERN_ADD_MAP("{Stage.DebugSymbolFile.Prefix}");
            INTERN_ADD_MAP("{Stage.SharedLibraryFile.Extension}");
            INTERN_ADD_MAP("{Stage.SharedLinkFile.Extension}");
            INTERN_ADD_MAP("{Stage.StaticLinkFile.Extension}");
            INTERN_ADD_MAP("{Stage.ObjectLinkFile.Extension}");
            INTERN_ADD_MAP("{Stage.DebugSymbolFile.Extension}");
            INTERN_ADD_MAP("{Stage.Executable}");
            INTERN_ADD_MAP("{Stage.LinkFlags}");
            INTERN_ADD_MAP("{Stage.Output.Name}");
            INTERN_ADD_MAP("{Stage.Output.Directory}");
            INTERN_ADD_MAP("{Stage.Input.Name}");
            INTERN_ADD_MAP("{Stage.Input.Dep.Name}");
            INTERN_ADD_MAP("{Stage.Input.Source.Name}");
            INTERN_ADD_MAP("{Stage.Input.Object.Name}");
            INTERN_ADD_MAP("{Stage.Input.Dep.Object.Name}");
            INTERN_ADD_MAP("{Stage.Input.Source.Object.Name}");
            INTERN_ADD_MAP("{Stage.Input.Shared.Name}");
            INTERN_ADD_MAP("{Stage.Input.Static.Name}");
            INTERN_ADD_MAP("{Stage.Input.Extension}");
            INTERN_ADD_MAP("{Stage.Input.Dep.Extension}");
            INTERN_ADD_MAP("{Stage.Input.Source.Extension}");
            INTERN_ADD_MAP("{Stage.Input.Object.Extension}");
            INTERN_ADD_MAP("{Stage.Input.Dep.Object.Extension}");
            INTERN_ADD_MAP("{Stage.Input.Source.Object.Extension}");
            INTERN_ADD_MAP("{Stage.Input.Shared.Extension}");
            INTERN_ADD_MAP("{Stage.Input.Static.Extension}");
            INTERN_ADD_MAP("{Stage.Input.Directory}");
            INTERN_ADD_MAP("{Stage.Input.Dep.Directory}");
            INTERN_ADD_MAP("{Stage.Input.Source.Directory}");
            INTERN_ADD_MAP("{Stage.Input.Object.Directory}");
            INTERN_ADD_MAP("{Stage.Input.Dep.Object.Directory}");
            INTERN_ADD_MAP("{Stage.Input.Source.Object.Directory}");
            INTERN_ADD_MAP("{Stage.Input.Shared.Directory}");
            INTERN_ADD_MAP("{Stage.Input.Static.Directory}");
            INTERN_ADD_MAP("{Stage.Input.Path}");
            INTERN_ADD_MAP("{Stage.Input.Dep.Path}");
            INTERN_ADD_MAP("{Stage.Input.Source.Path}");
            INTERN_ADD_MAP("{Stage.Input.Object.Path}");
            INTERN_ADD_MAP("{Stage.Input.Dep.Object.Path}");
            INTERN_ADD_MAP("{Stage.Input.Source.Object.Path}");
            INTERN_ADD_MAP("{Stage.Input.Shared.Path}");
            INTERN_ADD_MAP("{Stage.Input.Static.Path}");
            INTERN_ADD_MAP("{/}");
            
            #undef INTERN_ADD_MAP
        }

        inline DS::Result<void> 
        ParseYAML_Node( YAML::ConstNodePtr profileNode,
                        bool parseParameters,
                        const std::unordered_map<std::string, std::string>& inputParameters)
        {
            ssLOG_FUNC_DEBUG();
            
            //NOTE: Parameters are already handled in ResolveProfileImport
            if(parseParameters)
            {
                ParseParametersAndVariables(*this, profileNode).DS_TRY();
            }
            
            //Clone and modify the yaml node
            YAML::ResourceHandle resourceHandle;
            YAML::NodePtr clonedNode = profileNode->Clone(false, resourceHandle).DS_TRY();
            DEFER { YAML::FreeYAMLResource(resourceHandle); };
            
            if(parseParameters)
            {
                std::unordered_map<std::string, std::vector<std::string>> substitutionMap;
                PopulateCompilingLinkingParams(substitutionMap);
                ApplyParametersAndVariables(*this, 
                                            clonedNode, 
                                            resourceHandle, 
                                            substitutionMap, 
                                            inputParameters, 
                                            {}).DS_TRY();
            }
            
            std::vector<NodeRequirement> requirements =
            {
                NodeRequirement("Name", YAML::NodeType::Scalar, true, false),
                NodeRequirement("NameAliases", YAML::NodeType::Sequence, false, true),
                NodeRequirement("FileExtensions", YAML::NodeType::Sequence, true, false),
                NodeRequirement("Languages", YAML::NodeType::Sequence, false, true),
                NodeRequirement("Setup", YAML::NodeType::Map, false, true),
                NodeRequirement("Cleanup", YAML::NodeType::Map, false, true),
                NodeRequirement("FilesTypes", YAML::NodeType::Map, true, false),
                NodeRequirement("Compiler", YAML::NodeType::Map, true, false),
                NodeRequirement("Linker", YAML::NodeType::Map, true, false)
            };
            
            if(!CheckNodeRequirements(clonedNode, requirements))
                return DS_ERROR_MSG("Compiler profile: Failed to meet requirements");
            
            Name = clonedNode->GetMapValueScalar<std::string>("Name").DS_TRY();
            
            if(ExistAndHasChild(clonedNode, "NameAliases"))
            {
                YAML::ConstNodePtr nameAliasesNode = clonedNode->GetMapValueNode("NameAliases");
                for(int i = 0; i < nameAliasesNode->GetChildrenCount(); ++i)
                {
                    std::string nameAlias = nameAliasesNode ->GetSequenceChildScalar<std::string>(i)
                                                            .DS_TRY();
                    NameAliases.insert(nameAlias);
                }
            }

            {
                YAML::ConstNodePtr fileExtensionsNode = clonedNode->GetMapValueNode("FileExtensions");
                for(int i = 0; i < fileExtensionsNode->GetChildrenCount(); ++i)
                {
                    std::string extension = 
                        fileExtensionsNode->GetSequenceChildScalar<std::string>(i).DS_TRY();
                    FileExtensions.insert(extension);
                }
            }
            
            if(ExistAndHasChild(clonedNode, "Languages"))
            {
                YAML::ConstNodePtr languagesNode = clonedNode->GetMapValueNode("Languages");
                for(int i = 0; i < languagesNode->GetChildrenCount(); ++i)
                {
                    std::string language = languagesNode->GetSequenceChildScalar<std::string>(i)
                                                        .DS_TRY();
                    Languages.insert(language);
                }
            }
            
            if(ExistAndHasChild(clonedNode, "Setup"))
            {
                YAML::ConstNodePtr setupNode = clonedNode->GetMapValueNode("Setup");
                for(int i = 0; i < setupNode->GetChildrenCount(); ++i)
                {
                    YAML::ConstNodePtr currentPlatformNode = setupNode->GetMapValueNodeAt(i);
                    
                    std::string key = setupNode->GetMapKeyScalarAt<std::string>(i).DS_TRY();
                    std::vector<std::string> setupSteps;
                    
                    for(int j = 0; j < currentPlatformNode->GetChildrenCount(); ++j)
                    {
                        std::string step = 
                            currentPlatformNode->GetSequenceChildScalar<std::string>(j).DS_TRY();
                        setupSteps.push_back(step);
                    }
                    
                    Setup[key] = setupSteps;
                }
            }
            
            if(ExistAndHasChild(clonedNode, "Cleanup"))
            {
                YAML::ConstNodePtr cleanupNode = clonedNode->GetMapValueNode("Cleanup");
                for(int i = 0; i < cleanupNode->GetChildrenCount(); ++i)
                {
                    YAML::ConstNodePtr currentPlatformNode = cleanupNode->GetMapValueNodeAt(i);
                    
                    std::string key = cleanupNode->GetMapKeyScalarAt<std::string>(i).DS_TRY();
                    std::vector<std::string> cleanupSteps;
                    
                    for(int j = 0; j < currentPlatformNode->GetChildrenCount(); ++j)
                    {
                        std::string step = 
                            currentPlatformNode->GetSequenceChildScalar<std::string>(j).DS_TRY();
                        cleanupSteps.push_back(step);
                    }
                    
                    Cleanup[key] = cleanupSteps;
                }
            }
            
            if(!FilesTypes.ParseYAML_Node(clonedNode->GetMapValueNode("FilesTypes")))
                return DS_ERROR_MSG("Profile: FilesTypes is invalid");
            
            ssLOG_DEBUG("Parsing Compiler");
            if(!Compiler.ParseYAML_Node(clonedNode->GetMapValueNode("Compiler"), "CompileTypes"))
                return DS_ERROR_MSG("Profile: Compiler is invalid");
            
            ssLOG_DEBUG("Parsing Linker");
            if(!Linker.ParseYAML_Node(clonedNode->GetMapValueNode("Linker"), "LinkTypes"))
                return DS_ERROR_MSG("Profile: Linker is invalid");
            
            return {};
        }

        inline std::string ToString(std::string indentation) const
        {
            std::string out;
            
            out += indentation + "Name: " + GetEscapedYAMLString(Name) + "\n";
            
            if(!NameAliases.empty())
            {
                out += indentation + "NameAliases:\n";
                for(auto it = NameAliases.begin(); it != NameAliases.end(); ++it)
                    out += indentation + "-   " + GetEscapedYAMLString(*it) + "\n";
            }
            
            if(FileExtensions.empty())
                out += indentation + "FileExtensions: []\n";
            else
            {
                out += indentation + "FileExtensions:\n";
                for(auto it = FileExtensions.begin(); it != FileExtensions.end(); ++it)
                    out += indentation + "-   " + GetEscapedYAMLString(*it) + "\n";
            }
            
            if(!Languages.empty())
            {
                out += indentation + "Languages:\n";
                for(auto it = Languages.begin(); it != Languages.end(); ++it)
                    out += indentation + "-   " + GetEscapedYAMLString(*it) + "\n";
            }
            
            if(!Setup.empty())
            {
                out += indentation + "Setup:\n";
                for(auto it = Setup.begin(); it != Setup.end(); ++it)
                {
                    out += indentation + "    " + it->first + ":\n";
                    for(int i = 0; i < it->second.size(); ++i)
                        out += indentation + "    -   " + GetEscapedYAMLString(it->second[i]) + "\n";
                }
            }
            
            if(!Cleanup.empty())
            {
                out += indentation + "Cleanup:\n";
                for(auto it = Cleanup.begin(); it != Cleanup.end(); ++it)
                {
                    out += indentation + "    " + it->first + ":\n";
                    for(int i = 0; i < it->second.size(); ++i)
                        out += indentation + "    -   " + GetEscapedYAMLString(it->second[i]) + "\n";
                }
            }
            
            out += indentation + "FilesTypes:\n";
            out += FilesTypes.ToString(indentation + "    ");
            
            out += indentation + "Compiler:\n";
            out += Compiler.ToString(indentation + "    ", "CompileTypes");
            
            out += indentation + "Linker:\n";
            out += Linker.ToString(indentation + "    ", "LinkTypes");
            
            return out;
        }

        inline bool Equals(const Profile& other) const
        {
            if( Name != other.Name || 
                NameAliases.size() != other.NameAliases.size() ||
                FileExtensions.size() != other.FileExtensions.size() ||
                Languages.size() != other.Languages.size() ||
                Setup.size() != other.Setup.size() ||
                Cleanup.size() != other.Cleanup.size() ||
                !FilesTypes.Equals(other.FilesTypes) ||
                !Compiler.Equals(other.Compiler) ||
                !Linker.Equals(other.Linker))
            {
                return false;
            }
            
            for(const std::string& it : NameAliases)
            {
                if(other.NameAliases.count(it) == 0)
                    return false;
            }
            
            for(const std::string& it : FileExtensions)
            {
                if(other.FileExtensions.count(it) == 0)
                    return false;
            }
            
            for(const std::string& it : Languages)
            {
                if(other.Languages.count(it) == 0)
                    return false;
            }
            
            for(const auto& it : Setup)
            {
                if(other.Setup.count(it.first) == 0 || other.Setup.at(it.first) != it.second)
                    return false;
            }
            
            for(const auto& it : Cleanup)
            {
                if(other.Cleanup.count(it.first) == 0 || other.Cleanup.at(it.first) != it.second)
                    return false;
            }
            
            return true;
        }
    };
}
}

namespace runcpp2
{
    template <typename T>
    inline bool HasValueFromProfileMap( const Data::Profile& profile,
                                        const std::unordered_map<ProfileName, T>& map)
    {
        std::vector<std::string> profileNames;
        profile.GetNames(profileNames);
        
        for(int i = 0; i < profileNames.size(); ++i)
        {
            if(map.find(profileNames.at(i)) != map.end())
                return true;
        }
        return false;
    }
    
    template <typename T>
    inline const T* GetValueFromProfileMap( const Data::Profile& profile,
                                            const std::unordered_map<ProfileName, T>& map)
    {
        std::vector<std::string> profileNames;
        profile.GetNames(profileNames);
        
        for(int i = 0; i < profileNames.size(); ++i)
        {
            auto it = map.find(profileNames.at(i));
            if(it != map.end())
                return &it->second;
        }
        return nullptr;
    }
}


#endif
