#include "runcpp2/ConfigParsing.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "ryml.hpp"
#include "c4/std/string.hpp"
#include "cfgpath.h"
#include "ghc/filesystem.hpp"
#include "ssLogger/ssLog.hpp"


extern "C" const uint8_t DefaultUserConfig[];
extern "C" const size_t DefaultUserConfig_size;
extern "C" const uint8_t DefaultScriptDependencies[];
extern "C" const size_t DefaultScriptDependencies_size;

namespace 
{
    bool ParseUserConfig( const std::string& userConfigString, 
                                std::vector<runcpp2::Data::CompilerProfile>& outProfiles,
                                std::string& outPreferredProfile)
    {
        ssLOG_FUNC_DEBUG();
        
        //TODO: Use callback once ryml noexcept are dropped
        #if 0
            ryml::Callbacks cb;
            auto errorCallback = [](const char* msg, 
                                    size_t msg_len, 
                                    ryml::Location location, 
                                    void *user_data)
            {
                std::string msgStr(msg, msg_len);
                throw std::runtime_error(msgStr);
            };
            
            cb.m_error = errorCallback;
            ryml::set_callbacks(cb);
        #endif
        
        INTERNAL_RUNCPP2_SAFE_START();
        
        std::string temp = userConfigString;
        ryml::Tree rootTree = ryml::parse_in_place(c4::to_substr(temp));
        ryml::ConstNodeRef rootCompilerProfileNode;
        
        if(!runcpp2::ResolveYAML_Stream(rootTree, rootCompilerProfileNode))
            return false;
        
        if( !runcpp2::ExistAndHasChild(rootCompilerProfileNode, "CompilerProfiles") || 
            !(rootCompilerProfileNode["CompilerProfiles"].type().type & ryml::NodeType_e::SEQ))
        {
            ssLOG_ERROR("CompilerProfiles is invalid");
            return false;
        }
        
        ryml::ConstNodeRef compilerProfilesNode = rootCompilerProfileNode["CompilerProfiles"];
        
        if(compilerProfilesNode.num_children() == 0)
        {
            ssLOG_ERROR("No compiler profiles found");
            return false;
        }
        
        for(int i = 0; i < compilerProfilesNode.num_children(); ++i)
        {
            ryml::ConstNodeRef currentCompilerProfileNode = compilerProfilesNode[i];
            
            outProfiles.push_back({});
            if(!outProfiles.back().ParseYAML_Node(currentCompilerProfileNode))
            {
                outProfiles.erase(outProfiles.end() - 1);
                ssLOG_ERROR("Failed to parse compiler profile at index " << i);
                return false;
            }
        }
        
        if( runcpp2::ExistAndHasChild(rootCompilerProfileNode, "PreferredProfile") && 
            rootCompilerProfileNode["PreferredProfile"].type().type & ryml::NodeType_e::KEYVAL)
        {
            rootCompilerProfileNode["PreferredProfile"] >> outPreferredProfile;
            if(outPreferredProfile.empty())
            {
                outPreferredProfile = outProfiles.at(0).Name;
                ssLOG_WARNING("PreferredProfile is empty. Using the first profile name");
            }
        }
        
        ryml::reset_callbacks();
        
        return true;
        
        INTERNAL_RUNCPP2_SAFE_CATCH_ACTION(ryml::reset_callbacks(); return false;);
    }
}

bool runcpp2::ReadUserConfig(   std::vector<Data::CompilerProfile>& outProfiles, 
                                std::string& outPreferredProfile)
{
    ssLOG_FUNC_DEBUG();
    
    //Check if user config exists
    char configDirC_Str[MAX_PATH] = {0};
    
    get_user_config_folder(configDirC_Str, MAX_PATH, "runcpp2");
    
    if(strlen(configDirC_Str) == 0)
    {
        ssLOG_ERROR("Failed to retrieve user config path");
        return false;
    }
    
    std::string configDir = std::string(configDirC_Str);
    
    ssLOG_INFO("configDir: " << configDir);
    
    std::string compilerConfigFilePaths[2] = 
    {
        configDir + "/UserConfig.yaml", 
        configDir + "/UserConfig.yml"
    };
    int foundConfigFilePathIndex = -1;
    
    bool writeDefaultConfig = true;
    
    //config directory is created by get_user_config_folder if it doesn't exist
    {
        for(int i = 0; i < sizeof(compilerConfigFilePaths) / sizeof(std::string); ++i)
        {
            //Check if the config file exists
            if(ghc::filesystem::exists(compilerConfigFilePaths[i]))
            {
                foundConfigFilePathIndex = i;
                writeDefaultConfig = false;
                break;
            }
        }
    }
    
    //Create default compiler profiles
    if(writeDefaultConfig)
    {
        //Create default compiler profiles
        std::ofstream configFile(compilerConfigFilePaths[0], std::ios::binary);
        if(!configFile)
        {
            ssLOG_ERROR("Failed to create default config file: " << compilerConfigFilePaths[0]);
            return false;
        }
        configFile.write((const char*)DefaultUserConfig, DefaultUserConfig_size);
        configFile.close();
        foundConfigFilePathIndex = 0;
    }
    
    //Read compiler profiles
    std::string userConfigContent;
    {
        std::ifstream userConfigFilePath(compilerConfigFilePaths[foundConfigFilePathIndex]);
        if(!userConfigFilePath)
        {
            ssLOG_ERROR("Failed to open config file: " << 
                        compilerConfigFilePaths[foundConfigFilePathIndex]);
            
            return false;
        }
        std::stringstream buffer;
        buffer << userConfigFilePath.rdbuf();
        userConfigContent = buffer.str();
    }
    
    if(!ParseUserConfig(userConfigContent, outProfiles, outPreferredProfile))
    {
        ssLOG_ERROR("Failed to parse config file: " << 
                    compilerConfigFilePaths[foundConfigFilePathIndex]);
        
        return false;
    }

    return true;
}

bool runcpp2::ParseScriptInfo(  const std::string& scriptInfo, 
                                Data::ScriptInfo& outScriptInfo)
{
    if(scriptInfo.empty())
        return true;

    //TODO: Use callback once ryml noexcept are dropped
    #if 0
        ryml::Callbacks cb;
        auto errorCallback = [](const char* msg, 
                                size_t msg_len, 
                                ryml::Location location, 
                                void *user_data)
        {
            std::string msgStr(msg, msg_len);
            throw std::runtime_error(msgStr);
        };
        
        cb.m_error = errorCallback;
        ryml::set_callbacks(cb);
    #endif
    
    INTERNAL_RUNCPP2_SAFE_START();

    ryml::Tree scriptTree;
    
    std::string temp = scriptInfo;
    scriptTree = ryml::parse_in_place(c4::to_substr(temp));
    
    ryml::ConstNodeRef rootScriptNode;
    
    if(!runcpp2::ResolveYAML_Stream(scriptTree, rootScriptNode))
        return false;
    
    if(outScriptInfo.ParseYAML_Node(rootScriptNode))
    {
        outScriptInfo.Populated = true;
        return true;
    }
    else
        return false;
    
    INTERNAL_RUNCPP2_SAFE_CATCH_ACTION(ryml::reset_callbacks(); return false;);
}