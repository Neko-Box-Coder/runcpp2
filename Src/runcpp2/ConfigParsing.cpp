#include "runcpp2/ConfigParsing.hpp"
#include "cfgpath.h"
#include "ghc/filesystem.hpp"

#include "ssLogger/ssLog.hpp"


extern const uint8_t DefaultCompilerProfiles[];
extern const size_t DefaultCompilerProfiles_size;
extern const uint8_t DefaultScriptDependencies[];
extern const size_t DefaultScriptDependencies_size;

namespace 
{
    bool ParseCompilerProfiles( const std::string& compilerProfilesString, 
                                std::vector<runcpp2::CompilerProfile>& outProfiles,
                                std::string& outPreferredProfile)
    {
        YAML::Node compilersProfileYAML;
        
        try
        {
            compilersProfileYAML = YAML::Load(compilerProfilesString);
        }
        catch(...)
        {
            return false;
        }

        YAML::Node compilerProfilesNode = compilersProfileYAML["CompilerProfiles"];
        if(!compilerProfilesNode || compilerProfilesNode.Type() != YAML::NodeType::Sequence)
        {
            ssLOG_ERROR("CompilerProfiles is invalid");
            return false;
        }
        
        if(compilerProfilesNode.size() == 0)
        {
            ssLOG_ERROR("No compiler profiles found");
            return false;
        }
        
        for(int i = 0; i < compilerProfilesNode.size(); ++i)
        {
            YAML::Node currentCompilerProfileNode = compilerProfilesNode[i];
            
            outProfiles.push_back({});
            if(!outProfiles.back().ParseYAML_Node(currentCompilerProfileNode))
            {
                outProfiles.erase(outProfiles.end() - 1);
                ssLOG_ERROR("Failed to parse compiler profile at index " << i);
                return false;
            }
        }
        
        if( compilersProfileYAML["PreferredProfile"] && 
            compilersProfileYAML["PreferredProfile"].Type() == YAML::NodeType::Scalar)
        {
            outPreferredProfile = compilersProfileYAML["PreferredProfile"].as<std::string>();
            if(outPreferredProfile.empty())
            {
                outPreferredProfile = outProfiles.at(0).Name;
                ssLOG_WARNING("PreferredProfile is empty. Using the first profile name");
            }
        }
        
        return true;
    }
}

bool runcpp2::ReadUserConfig( std::vector<CompilerProfile>& outProfiles, 
                                        std::string& outPreferredProfile)
{
    //Check if user config exists
    char configDirC_Str[MAX_PATH] = {0};
    
    get_user_config_folder(configDirC_Str, 512, "runcpp2");
    
    if(strlen(configDirC_Str) == 0)
    {
        ssLOG_ERROR("Failed to retrieve user config path");
        return false;
    }
    
    std::string configDir = std::string(configDirC_Str);
    
    std::string compilerConfigFilePaths[2] = 
    {
        configDir + "/CompilerProfiles.yaml", 
        configDir + "/CompilerProfiles.yml"
    };
    int foundConfigFilePathIndex = -1;
    
    bool writeDefaultProfiles = false;
    
    //config directory is created by get_user_config_folder if it doesn't exist
    {
        for(int i = 0; i < sizeof(compilerConfigFilePaths) / sizeof(std::string); ++i)
        {
            //Check if the config file exists
            if(ghc::filesystem::exists(compilerConfigFilePaths[i]))
            {
                foundConfigFilePathIndex = i;
                writeDefaultProfiles = false;
                break;
            }
        }
        
        writeDefaultProfiles = true;
    }
    
    //Create default compiler profiles
    if(writeDefaultProfiles)
    {
        //Create default compiler profiles
        std::ofstream configFile(compilerConfigFilePaths[0], std::ios::binary);
        if(!configFile)
        {
            ssLOG_ERROR("Failed to create default config file: " << compilerConfigFilePaths[0]);
            return false;
        }
        configFile.write((const char*)DefaultCompilerProfiles, DefaultCompilerProfiles_size);
        configFile.close();
        foundConfigFilePathIndex = 0;
    }
    
    //Read compiler profiles
    std::string compilerConfigContent;
    {
        std::ifstream compilerConfigFilePath(compilerConfigFilePaths[foundConfigFilePathIndex]);
        if(!compilerConfigFilePath)
        {
            ssLOG_ERROR("Failed to open config file: " << 
                        compilerConfigFilePaths[foundConfigFilePathIndex]);
            
            return false;
        }
        std::stringstream buffer;
        buffer << compilerConfigFilePath.rdbuf();
        compilerConfigContent = buffer.str();
    }
    
    if(!ParseCompilerProfiles(compilerConfigContent, outProfiles, outPreferredProfile))
    {
        ssLOG_ERROR("Failed to parse config file: " << 
                    compilerConfigFilePaths[foundConfigFilePathIndex]);
        
        return false;
    }

    return true;
}

bool runcpp2::ParseScriptInfo(  const std::string& scriptInfo, 
                                ScriptInfo& outScriptInfo)
{
    if(scriptInfo.empty())
        return true;

    YAML::Node scriptYAML;
    
    try
    {
        scriptYAML = YAML::Load(scriptInfo);
    }
    catch(...)
    {
        return false;
    }
    
    if(outScriptInfo.ParseYAML_Node(scriptYAML))
    {
        outScriptInfo.Populated = true;
        return true;
    }
    else
        return false;
}