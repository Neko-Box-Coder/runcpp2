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
