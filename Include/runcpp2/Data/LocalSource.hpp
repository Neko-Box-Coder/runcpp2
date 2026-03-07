#ifndef RUNCPP2_DATA_LOCAL_SOURCE_HPP
#define RUNCPP2_DATA_LOCAL_SOURCE_HPP

#include "runcpp2/YamlLib.hpp"
#include "runcpp2/Data/ParseCommon.hpp"
#include "runcpp2/ParseUtil.hpp"

#include "ssLogger/ssLog.hpp"
#include <string>

namespace runcpp2
{
namespace Data
{
    enum class LocalCopyMode;
}
}

namespace
{
    std::string CopyModeToString(runcpp2::Data::LocalCopyMode mode);
    runcpp2::Data::LocalCopyMode StringToCopyMode(const std::string& str, bool& outSuccess);
}

namespace runcpp2
{
namespace Data
{
    enum class LocalCopyMode
    {
        Auto,
        Symlink,
        Hardlink,
        Copy,
        Count
    };

    struct LocalSource
    {
        std::string Path;
        LocalCopyMode CopyMode = LocalCopyMode::Auto;
        
        inline bool ParseYAML_Node(YAML::ConstNodePtr node)
        {
            std::vector<NodeRequirement> requirements =
            {
                NodeRequirement("Path", YAML::NodeType::Scalar, true, false)
            };
            
            if(!CheckNodeRequirements(node, requirements))
            {
                ssLOG_ERROR("LocalSource: Failed to meet requirements");
                return false;
            }
            
            Path = node->GetMapValueScalar<std::string>("Path").DS_TRY_ACT(return false);
            if(ExistAndHasChild(node, "CopyMode"))
            {
                bool success = false;
                CopyMode = StringToCopyMode(node->GetMapValueScalar<std::string>("CopyMode")
                                                .DefaultOr(),
                                            success);
                if(!success)
                    return false;
            }
            
            return true;
        }

        inline std::string ToString(std::string indentation) const
        {
            std::string out;
            out += indentation + "Local:\n";
            out += indentation + "    Path: " + GetEscapedYAMLString(Path) + "\n";
            out += indentation + "    CopyMode: " + CopyModeToString(CopyMode) + "\n";
            return out;
        }

        inline bool Equals(const LocalSource& other) const
        {
            return Path == other.Path && CopyMode == other.CopyMode;
        }
    };
}
}

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

#endif 
