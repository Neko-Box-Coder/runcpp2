#include "runcpp2/Data/Profile.hpp"
#include "runcpp2/Data/ParseCommon.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "ssLogger/ssLog.hpp"

void runcpp2::Data::Profile::GetNames(std::vector<std::string>& outNames) const
{
    outNames.clear();
    outNames.push_back(Name);
    for(const auto& alias : NameAliases)
        outNames.push_back(alias);
    
    //Special name all that applies to all profile
    outNames.push_back("DefaultProfile");
}

bool runcpp2::Data::Profile::ParseYAML_Node(ryml::ConstNodeRef profileNode)
{
    ssLOG_FUNC_DEBUG();
    
    INTERNAL_RUNCPP2_SAFE_START();

    std::vector<NodeRequirement> requirements =
    {
        NodeRequirement("Name", ryml::NodeType_e::KEYVAL, true, false),
        NodeRequirement("NameAliases", ryml::NodeType_e::SEQ, false, true),
        NodeRequirement("FileExtensions", ryml::NodeType_e::SEQ, true, false),
        NodeRequirement("Languages", ryml::NodeType_e::SEQ, false, true),
        NodeRequirement("Setup", ryml::NodeType_e::MAP, false, true),
        NodeRequirement("Cleanup", ryml::NodeType_e::MAP, false, true),
        NodeRequirement("FilesTypes", ryml::NodeType_e::MAP, true, false),
        NodeRequirement("Compiler", ryml::NodeType_e::MAP, true, false),
        NodeRequirement("Linker", ryml::NodeType_e::MAP, true, false)
    };
    
    if(!CheckNodeRequirements(profileNode, requirements))
    {
        ssLOG_ERROR("Compiler profile: Failed to meet requirements");
        return false;
    }
    
    profileNode["Name"] >> Name;
    
    if(ExistAndHasChild(profileNode, "NameAliases"))
    {
        for(int i = 0; i < profileNode["NameAliases"].num_children(); ++i)
            NameAliases.insert(GetValue(profileNode["NameAliases"][i]));
    }

    for(int i = 0; i < profileNode["FileExtensions"].num_children(); ++i)
        FileExtensions.insert(GetValue(profileNode["FileExtensions"][i]));
    
    if(ExistAndHasChild(profileNode, "Languages"))
    {
        for(int i = 0; i < profileNode["Languages"].num_children(); ++i)
            Languages.insert(GetValue(profileNode["Languages"][i]));
    }
    
    if(ExistAndHasChild(profileNode, "Setup"))
    {
        for(int i = 0; i < profileNode["Setup"].num_children(); ++i)
        {
            ryml::ConstNodeRef currentPlatform = profileNode["Setup"][i];
            
            std::string key = GetKey(currentPlatform);
            std::vector<std::string> setupSteps;
            
            for(int j = 0; j < currentPlatform.num_children(); ++j)
                setupSteps.push_back(GetValue(currentPlatform[j]));
            
            Setup[key] = setupSteps;
        }
    }
    
    if(ExistAndHasChild(profileNode, "Cleanup"))
    {
        for(int i = 0; i < profileNode["Cleanup"].num_children(); ++i)
        {
            ryml::ConstNodeRef currentPlatform = profileNode["Cleanup"][i];
            
            std::string key = GetKey(currentPlatform);
            std::vector<std::string> cleanupSteps;
            
            for(int j = 0; j < currentPlatform.num_children(); ++j)
                cleanupSteps.push_back(GetValue(currentPlatform[j]));
            
            Cleanup[key] = cleanupSteps;
        }
    }
    
    ryml::ConstNodeRef filesTypesNode = profileNode["FilesTypes"];
    if(!FilesTypes.ParseYAML_Node(filesTypesNode))
    {
        ssLOG_ERROR("Profile: FilesTypes is invalid");
        return false;
    }
    
    ssLOG_DEBUG("Parsing Compiler");
    ryml::ConstNodeRef compilerNode = profileNode["Compiler"];
    if(!Compiler.ParseYAML_Node(compilerNode, "CompileTypes"))
    {
        ssLOG_ERROR("Profile: Compiler is invalid");
        return false;
    }
    
    ssLOG_DEBUG("Parsing Linker");
    ryml::ConstNodeRef linkerNode = profileNode["Linker"];
    if(!Linker.ParseYAML_Node(linkerNode, "LinkTypes"))
    {
        ssLOG_ERROR("Profile: Linker is invalid");
        return false;
    }
    
    return true;
    
    INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
}

bool runcpp2::Data::Profile::ParseYAML_Node(YAML::ConstNodePtr profileNode)
{
    ssLOG_FUNC_DEBUG();
    
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
    
    if(!CheckNodeRequirements_LibYaml(profileNode, requirements))
    {
        ssLOG_ERROR("Compiler profile: Failed to meet requirements");
        return false;
    }
    
    Name = profileNode->GetMapValueScalar<std::string>("Name").DS_TRY_ACT(return false);
    
    if(ExistAndHasChild_LibYaml(profileNode, "NameAliases"))
    {
        YAML::ConstNodePtr nameAliasesNode = profileNode->GetMapValueNode("NameAliases");
        for(int i = 0; i < nameAliasesNode->GetChildrenCount(); ++i)
        {
            std::string nameAlias = nameAliasesNode ->GetSequenceChildScalar<std::string>(i)
                                                    .DS_TRY_ACT(return false);
            NameAliases.insert(nameAlias);
        }
    }

    {
        YAML::ConstNodePtr fileExtensionsNode = profileNode->GetMapValueNode("FileExtensions");
        for(int i = 0; i < fileExtensionsNode->GetChildrenCount(); ++i)
        {
            std::string extension = fileExtensionsNode  ->GetSequenceChildScalar<std::string>(i)
                                                        .DS_TRY_ACT(return false);
            FileExtensions.insert(extension);
        }
    }
    
    if(ExistAndHasChild_LibYaml(profileNode, "Languages"))
    {
        YAML::ConstNodePtr languagesNode = profileNode->GetMapValueNode("Languages");
        for(int i = 0; i < languagesNode->GetChildrenCount(); ++i)
        {
            std::string language = languagesNode->GetSequenceChildScalar<std::string>(i)
                                                .DS_TRY_ACT(return false);
            Languages.insert(language);
        }
    }
    
    if(ExistAndHasChild_LibYaml(profileNode, "Setup"))
    {
        YAML::ConstNodePtr setupNode = profileNode->GetMapValueNode("Setup");
        for(int i = 0; i < setupNode->GetChildrenCount(); ++i)
        {
            YAML::ConstNodePtr currentPlatformNode = setupNode->GetMapValueNodeAt(i);
            
            std::string key = setupNode->GetMapKeyScalarAt<std::string>(i).DS_TRY_ACT(return false);
            std::vector<std::string> setupSteps;
            
            for(int j = 0; j < currentPlatformNode->GetChildrenCount(); ++j)
            {
                std::string step = currentPlatformNode  ->GetSequenceChildScalar<std::string>(j)
                                                        .DS_TRY_ACT(return false);
                setupSteps.push_back(step);
            }
            
            Setup[key] = setupSteps;
        }
    }
    
    if(ExistAndHasChild_LibYaml(profileNode, "Cleanup"))
    {
        YAML::ConstNodePtr cleanupNode = profileNode->GetMapValueNode("Cleanup");
        for(int i = 0; i < cleanupNode->GetChildrenCount(); ++i)
        {
            YAML::ConstNodePtr currentPlatformNode = cleanupNode->GetMapValueNodeAt(i);
            
            std::string key = cleanupNode->GetMapKeyScalarAt<std::string>(i).DS_TRY_ACT(return false);
            std::vector<std::string> cleanupSteps;
            
            for(int j = 0; j < currentPlatformNode->GetChildrenCount(); ++j)
            {
                std::string step = currentPlatformNode  ->GetSequenceChildScalar<std::string>(j)
                                                        .DS_TRY_ACT(return false);
                cleanupSteps.push_back(step);
            }
            
            Cleanup[key] = cleanupSteps;
        }
    }
    
    if(!FilesTypes.ParseYAML_Node(profileNode->GetMapValueNode("FilesTypes")))
    {
        ssLOG_ERROR("Profile: FilesTypes is invalid");
        return false;
    }
    
    ssLOG_DEBUG("Parsing Compiler");
    if(!Compiler.ParseYAML_Node(profileNode->GetMapValueNode("Compiler"), "CompileTypes"))
    {
        ssLOG_ERROR("Profile: Compiler is invalid");
        return false;
    }
    
    ssLOG_DEBUG("Parsing Linker");
    if(!Linker.ParseYAML_Node(profileNode->GetMapValueNode("Linker"), "LinkTypes"))
    {
        ssLOG_ERROR("Profile: Linker is invalid");
        return false;
    }
    
    return true;
}

std::string runcpp2::Data::Profile::ToString(std::string indentation) const
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

bool runcpp2::Data::Profile::Equals(const Profile& other) const
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
