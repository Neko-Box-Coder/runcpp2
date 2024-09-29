#include "runcpp2/Data/ProfilesDefines.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "ssLogger/ssLog.hpp"
#include <string>

bool runcpp2::Data::ProfilesDefines::ParseYAML_Node(ryml::ConstNodeRef& node)
{
    ssLOG_FUNC_DEBUG();

    INTERNAL_RUNCPP2_SAFE_START();
    
    if(!node.is_map())
    {
        ssLOG_ERROR("ProfilesDefines: Not a map type");
        return false;
    }
    
    for(int i = 0; i < node.num_children(); ++i)
    {
        if(!INTERNAL_RUNCPP2_BIT_CONTANTS(node[i].type().type, ryml::NodeType_e::SEQ))
        {
            ssLOG_ERROR("ProfilesDefines: Defines type requires a list");
            return false;
        }
        
        ryml::ConstNodeRef currentProfileDefinesNode = node[i];
        ProfileName profile = GetKey(currentProfileDefinesNode);

        for(int j = 0; j < currentProfileDefinesNode.num_children(); ++j)
        {
            std::string defineStr = GetValue(currentProfileDefinesNode[j]);
            Define define;
            
            size_t equalPos = defineStr.find('=');
            if(equalPos != std::string::npos)
            {
                define.Name = defineStr.substr(0, equalPos);
                define.Value = defineStr.substr(equalPos + 1);
                define.HasValue = true;
            }
            else
            {
                define.Name = defineStr;
                define.Value = "";
                define.HasValue = false;
            }
            
            Defines[profile].push_back(define);
        }
    }
    
    return true;
    
    INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
}

std::string runcpp2::Data::ProfilesDefines::ToString(std::string indentation) const
{
    std::string result;
    
    for(const auto& profileDefines : Defines)
    {
        result += indentation + profileDefines.first + ":\n";
        for(const auto& define : profileDefines.second)
        {
            result += indentation + "-   ";
            if(define.Value.empty())
                result += define.Name + "\n";
            else
                result += define.Name + "=" + define.Value + "\n";
        }
    }
    
    return result;
}
