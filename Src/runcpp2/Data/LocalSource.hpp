#ifndef RUNCPP2_DATA_LOCAL_SOURCE_HPP
#define RUNCPP2_DATA_LOCAL_SOURCE_HPP

#include "runcpp2/LibYAML_Wrapper.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "runcpp2/Data/PlatformProfileContainer.hpp"

#include "DSResult/DSResult.hpp"
#include "ssLogger/ssLog.hpp"

#include <string>
#include <vector>

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
    
    struct LocalSourcePath
    {
        std::string Path;
        
        inline bool IsYAML_NodeParsableAsDefault(YAML::ConstNodePtr node) const
        {
            return node->IsScalar();
        }
        
        inline bool ParseYAML_Node(YAML::ConstNodePtr node)
        {
            if(!node->IsScalar())
            {
                ssLOG_ERROR("Failed to parse LocalSourcePath, unexpected type: " << 
                            YAML::NodeTypeToString(node->GetType()));
                return false;
            }
            Path = node->GetScalar<std::string>().DS_TRY_ACT(return false);
            return true;
        }
        
        inline std::string ToString(std::string indentation) const
        {
            std::string out;
            out += indentation + GetEscapedYAMLString(Path) + "\n";
            return out;
        }
        
        inline bool Equals(const LocalSourcePath& other) const
        {
            return Path == other.Path;
        }
    };
    
    struct LocalSource
    {
        PlatformProfileContainer<LocalSourcePath> Path;
        LocalCopyMode CopyMode = LocalCopyMode::Auto;
        
        inline bool ParseYAML_Node(YAML::ConstNodePtr node)
        {
            if(!ExistAndHasChild(node, "Path"))
            {
                ssLOG_ERROR("LocalSource must container \"Path\"");
                return false;
            }
            
            YAML::ConstNodePtr pathNode = node->GetMapValueNode("Path");
            if(!Path.ParseYAML_Node(pathNode))
                return false;
            
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
            out += indentation + "    Path: \n" + Path.ToString(indentation + "        ");
            out += indentation + "    CopyMode: " + CopyModeToString(CopyMode) + "\n";
            return out;
        }

        inline bool Equals(const LocalSource& other) const
        {
            return Path.Equals(other.Path) && CopyMode == other.CopyMode;
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
