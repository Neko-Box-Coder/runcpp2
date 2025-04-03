#include "runcpp2/Data/LocalSource.hpp"
#include "runcpp2/Data/ParseCommon.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "ssLogger/ssLog.hpp"

namespace
{
    static_assert(  static_cast<int>(runcpp2::Data::LocalCopyMode::Count) == 4, 
                    "Update CopyModeToString when adding new LocalCopyMode values");

    std::string CopyModeToString(runcpp2::Data::LocalCopyMode mode)
    {
        switch(mode)
        {
            case runcpp2::Data::LocalCopyMode::Auto:
                return "Auto";
            case runcpp2::Data::LocalCopyMode::Symlink:
                return "Symlink";
            case runcpp2::Data::LocalCopyMode::Hardlink:
                return "Hardlink";
            case runcpp2::Data::LocalCopyMode::Copy:
                return "Copy";
            default:
                ssLOG_ERROR("Unknown LocalCopyMode value");
                return "Auto";
        }
    }

    static_assert(  static_cast<int>(runcpp2::Data::LocalCopyMode::Count) == 4, 
                    "Update StringToCopyMode when adding new LocalCopyMode values");

    runcpp2::Data::LocalCopyMode StringToCopyMode(const std::string& str, bool& outSuccess)
    {
        if(str == "Auto")
        { 
            outSuccess = true; 
            return runcpp2::Data::LocalCopyMode::Auto;
        }
        else if(str == "Symlink")
        { 
            outSuccess = true; 
            return runcpp2::Data::LocalCopyMode::Symlink;
        }
        else if(str == "Hardlink")
        { 
            outSuccess = true; 
            return runcpp2::Data::LocalCopyMode::Hardlink;
        }
        else if(str == "Copy")
        { 
            outSuccess = true; 
            return runcpp2::Data::LocalCopyMode::Copy;
        }
        
        ssLOG_ERROR("Invalid LocalCopyMode value: " << str);
        outSuccess = false;
        return runcpp2::Data::LocalCopyMode::Auto;
    }
}

bool runcpp2::Data::LocalSource::ParseYAML_Node(ryml::ConstNodeRef node)
{
    INTERNAL_RUNCPP2_SAFE_START();
    
    std::vector<NodeRequirement> requirements =
    {
        NodeRequirement("Path", ryml::NodeType_e::KEYVAL, true, false)
    };
    
    if(!CheckNodeRequirements(node, requirements))
    {
        ssLOG_ERROR("LocalSource: Failed to meet requirements");
        return false;
    }
    
    node["Path"] >> Path;

    if(ExistAndHasChild(node, "CopyMode"))
    {
        std::string modeStr;
        node["CopyMode"] >> modeStr;
        bool success = false;
        CopyMode = StringToCopyMode(modeStr, success);
        if (!success)
            return false;
    }
    
    return true;
    
    INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
}

std::string runcpp2::Data::LocalSource::ToString(std::string indentation) const
{
    std::string out;
    out += indentation + "Local:\n";
    out += indentation + "    Path: " + GetEscapedYAMLString(Path) + "\n";
    out += indentation + "    CopyMode: " + CopyModeToString(CopyMode) + "\n";
    return out;
}

bool runcpp2::Data::LocalSource::Equals(const LocalSource& other) const
{
    return Path == other.Path && CopyMode == other.CopyMode;
} 
