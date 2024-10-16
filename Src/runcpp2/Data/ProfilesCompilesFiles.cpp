#include "runcpp2/Data/ProfilesCompilesFiles.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "ssLogger/ssLog.hpp"

bool runcpp2::Data::ProfilesCompilesFiles::ParseYAML_Node(ryml::ConstNodeRef& node)
{
    ssLOG_FUNC_DEBUG();

    INTERNAL_RUNCPP2_SAFE_START();
    
    if(!node.is_map())
    {
        ssLOG_ERROR("ProfilesCompilesFiles: Not a map type");
        return false;
    }
    
    for(int i = 0; i < node.num_children(); ++i)
    {
        if(!INTERNAL_RUNCPP2_BIT_CONTANTS(node[i].type().type, ryml::NodeType_e::SEQ))
        {
            ssLOG_ERROR("ProfilesCompilesFiles: CompileFiles type requires a list");
            return false;
        }
        
        ryml::ConstNodeRef currentProfilePathsNode = node[i];
        ProfileName profile = GetKey(currentProfilePathsNode);

        for(int j = 0; j <  currentProfilePathsNode.num_children(); ++j)
            CompilesFiles[profile].push_back(GetValue(currentProfilePathsNode[j]));
    }
    
    return true;
    
    INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
}

std::string runcpp2::Data::ProfilesCompilesFiles::ToString(std::string indentation) const
{
    std::string out;
    
    for(auto it = CompilesFiles.begin(); it != CompilesFiles.end(); ++it)
    {
        out += indentation + it->first + ":\n";
        
        for(int i = 0; i < it->second.size(); ++i)
            out += indentation + "-   " + it->second.at(i).string() + "\n";
    }
    
    return out;
}
