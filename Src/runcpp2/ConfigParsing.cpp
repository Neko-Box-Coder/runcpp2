#include "runcpp2/ConfigParsing.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "runcpp2/StringUtil.hpp"
#include "runcpp2/PlatformUtil.hpp"
#include "runcpp2/YamlLib.hpp"

#include "cfgpath.h"
#include "ssLogger/ssLog.hpp"

#if INTERNAL_RUNCPP2_UNIT_TESTS
    #include "Tests/ConfigParsing/MockComponents.hpp"
#else
    #define CO_NO_OVERRIDE 1
    #include "CppOverride.hpp"
#endif

extern "C" const uint8_t DefaultUserConfig[];
extern "C" const size_t DefaultUserConfig_size;
extern "C" const uint8_t CommonFileTypes[];
extern "C" const size_t CommonFileTypes_size;
extern "C" const uint8_t G_PlusPlus[];
extern "C" const size_t G_PlusPlus_size;
extern "C" const uint8_t Vs2022_v17Plus[];
extern "C" const size_t Vs2022_v17Plus_size;


namespace 
{
    bool ResovleProfileImport(  ryml::NodeRef currentProfileNode, 
                                const ghc::filesystem::path& configPath,
                                std::vector<ryml::Tree>& importProfileTrees)
    {
        INTERNAL_RUNCPP2_SAFE_START();
        
        ssLOG_FUNC_INFO();
        
        ghc::filesystem::path currentImportFilePath = configPath;
        std::stack<ghc::filesystem::path> pathsToImport;
        while(runcpp2::ExistAndHasChild(currentProfileNode, "Import") || !pathsToImport.empty())
        {
            //If we import field, we should deal with it instead
            if(runcpp2::ExistAndHasChild(currentProfileNode, "Import"))
            {
                const ryml::NodeType_e importNodeType = currentProfileNode["Import"].type().type;
                if( !INTERNAL_RUNCPP2_BIT_CONTANTS(importNodeType, ryml::NodeType_e::KEYVAL) &&
                    !INTERNAL_RUNCPP2_BIT_CONTANTS(importNodeType, ryml::NodeType_e::SEQ))
                {
                    ssLOG_ERROR("Import must be a path or sequence of paths of YAML file(s)");
                    return false;
                }
                
                ghc::filesystem::path currentImportDir = currentImportFilePath;
                currentImportDir = currentImportDir.parent_path();
                if(currentProfileNode["Import"].is_keyval())
                {
                    pathsToImport.push( currentImportDir / 
                                        runcpp2::GetValue(currentProfileNode["Import"]));
                }
                else
                {
                    if(currentProfileNode["Import"].num_children() == 0)
                    {
                        ssLOG_ERROR("An import sequence cannot be an empty");
                        return false;
                    }
                    
                    for(int i = 0; i < currentProfileNode["Import"].num_children(); ++i)
                    {
                        if(!currentProfileNode["Import"][i].is_val())
                        {
                            ssLOG_ERROR("It must be a sequence of paths");
                            return false;
                        }
                        
                        pathsToImport.push( currentImportDir / 
                                            runcpp2::GetValue(currentProfileNode["Import"][i]));
                    }
                }
            }
            
            currentImportFilePath = pathsToImport.top();
            pathsToImport.pop();
            
            std::error_code ec;
            if(!ghc::filesystem::exists(currentImportFilePath, ec))
            {
                ssLOG_ERROR("Import path doesn't exist: " << currentImportFilePath.string());
                return false;
            }
            
            //Read compiler profiles
            std::ifstream importProfileFile(currentImportFilePath);
            if(!importProfileFile)
            {
                ssLOG_ERROR("Failed to open profile import file: " << currentImportFilePath);
                return false;
            }
            std::stringstream buffer;
            buffer << importProfileFile.rdbuf();
            
            ryml::NodeRef importProfileNode;
            importProfileTrees.emplace_back();
            importProfileTrees.back() = ryml::parse_in_arena(buffer.str().c_str());
            if(!runcpp2::ResolveYAML_Stream(importProfileTrees.back(), importProfileNode))
                return false;
            
            if(!runcpp2::MergeYAML_NodeChildren(importProfileNode, currentProfileNode))
                return false;
            
            //Replace the current import field if the import profile has an import field
            if(runcpp2::ExistAndHasChild(importProfileNode, "Import"))
            {
                currentProfileNode.remove_child("Import");
                importProfileNode["Import"].duplicate(currentProfileNode, {});
            }
            //Otherwise, remove the current import field
            else
                currentProfileNode.remove_child("Import");
        }
        
        return true;
        
        INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
    }
    
    bool ParseUserConfig(   const std::string& userConfigString, 
                            const ghc::filesystem::path& configPath,
                            std::vector<runcpp2::Data::Profile>& outProfiles,
                            std::string& outPreferredProfile)
    {
        INTERNAL_RUNCPP2_SAFE_START();
        
        ssLOG_FUNC_INFO();
        
        ryml::Tree rootTree = ryml::parse_in_arena(userConfigString.c_str());
        ryml::NodeRef configNode;
        
        if(!runcpp2::ResolveYAML_Stream(rootTree, configNode))
            return false;
        
        if( !runcpp2::ExistAndHasChild(configNode, "Profiles") || 
            !INTERNAL_RUNCPP2_BIT_CONTANTS( configNode["Profiles"].type().type,
                                            ryml::NodeType_e::SEQ))
        {
            ssLOG_ERROR("Profiles is invalid");
            return false;
        }
        
        ryml::NodeRef profilesNode = configNode["Profiles"];
        
        if(profilesNode.num_children() == 0)
        {
            ssLOG_ERROR("No compiler profiles found");
            return false;
        }
        
        ssLOG_INFO(profilesNode.num_children() << " profiles found in user config");
        std::vector<ryml::Tree> importProfileTrees;
        for(int i = 0; i < profilesNode.num_children(); ++i)
        {
            ssLOG_INFO("Parsing profile at index " << i);
            
            if(!INTERNAL_RUNCPP2_BIT_CONTANTS(profilesNode[i].type().type, ryml::NodeType_e::MAP))
            {
                ssLOG_ERROR("Profile entry must be a map");
                return false;
            }
            
            if(!ResovleProfileImport(profilesNode[i], configPath, importProfileTrees))
                return false;

            outProfiles.push_back({});
            if(!outProfiles.back().ParseYAML_Node(profilesNode[i]))
            {
                outProfiles.erase(outProfiles.end() - 1);
                ssLOG_ERROR("Failed to parse compiler profile at index " << i);
                return false;
            }
        } //for(int i = 0; i < profilesNode.num_children(); ++i)
        
        if(outProfiles.empty())
        {
            ssLOG_ERROR("No profiles registered");
            return false;
        }
        
        if(runcpp2::ExistAndHasChild(configNode, "PreferredProfile"))
        {
            ryml::ConstNodeRef preferredProfilesMapNode = configNode["PreferredProfile"];
            
            if(INTERNAL_RUNCPP2_BIT_CONTANTS(   preferredProfilesMapNode.type().type, 
                                                ryml::NodeType_e::MAP))
            {
                std::unordered_map<PlatformName, std::string> preferredProfiles;
                for(int i = 0; i < preferredProfilesMapNode.num_children(); ++i)
                {
                    PlatformName platform = runcpp2::GetKey(preferredProfilesMapNode[i]);
                    ryml::ConstNodeRef currentNode = preferredProfilesMapNode[i];
                    if(!INTERNAL_RUNCPP2_BIT_CONTANTS(  currentNode.type().type, 
                                                        ryml::NodeType_e::KEYVAL))
                    {
                        ssLOG_ERROR("Failed to parse PreferredProfile map. "
                                    "Keyval is expected in each platform");
                        return false;
                    }
                    currentNode >> preferredProfiles[platform];
                }
                
                const std::string* selectedProfile = 
                    runcpp2::GetValueFromPlatformMap(preferredProfiles);
                outPreferredProfile = 
                    selectedProfile != nullptr ? *selectedProfile : outPreferredProfile;
            }
            else if(INTERNAL_RUNCPP2_BIT_CONTANTS(  preferredProfilesMapNode.type().type, 
                                                    ryml::NodeType_e::KEYVAL))
            {
                configNode["PreferredProfile"] >> outPreferredProfile;
            }
            else
            {
                ssLOG_ERROR("PreferredProfile needs to be a map or string value: " << 
                            preferredProfilesMapNode.type().type_str());
                return false;
            }
            
            if(outPreferredProfile.empty())
            {
                outPreferredProfile = outProfiles.front().Name;
                ssLOG_WARNING("PreferredProfile is empty. Using the first profile name");
            }
        } //if(runcpp2::ExistAndHasChild(configNode, "PreferredProfile"))
        
        return true;
        
        INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
    }
}

std::string runcpp2::GetConfigFilePath()
{
    CO_INSERT_IMPL(OverrideInstance, std::string, ());
    
    //Check if user config exists
    char configDirC_Str[MAX_PATH] = {0};
    
    get_user_config_folder(configDirC_Str, MAX_PATH, "runcpp2");
    
    if(strlen(configDirC_Str) == 0)
    {
        ssLOG_ERROR("Failed to retrieve user config path");
        return "";
    }
    
    std::string configDir = std::string(configDirC_Str);
    
    ssLOG_INFO("configDir: " << configDir);
    
    std::string compilerConfigFilePaths[2] = 
    {
        configDir + "UserConfig.yaml", 
        configDir + "UserConfig.yml"
    };
    
    //config directory is created by get_user_config_folder if it doesn't exist
    {
        std::error_code ec;
        for(int i = 0; i < sizeof(compilerConfigFilePaths) / sizeof(std::string); ++i)
        {
            //Check if the config file exists
            if(ghc::filesystem::exists(compilerConfigFilePaths[i], ec))
                return compilerConfigFilePaths[i];
        }
    }
    
    return compilerConfigFilePaths[0];
}

bool runcpp2::WriteDefaultConfigs(  const ghc::filesystem::path& userConfigPath, 
                                    const bool writeUserConfig,
                                    const bool writeDefaultConfigs)
{
    CO_INSERT_IMPL(OverrideInstance, bool, (userConfigPath, writeUserConfig, writeDefaultConfigs));
    
    //Backup existing user config
    std::error_code _;
    if(writeUserConfig && ghc::filesystem::exists(userConfigPath, _))
    {
        int backupCount = 0;
        do
        {
            if(backupCount > 10)
            {
                ssLOG_ERROR("Failed to backup existing user config: " << userConfigPath.string());
                return false;
            }
            
            std::string backupPath = userConfigPath.string();
            
            if(backupCount > 0)
                backupPath += "." + std::to_string(backupCount);
            
            backupPath += ".bak";
            
            if(ghc::filesystem::exists(backupPath, _))
            {
                ssLOG_WARNING("Backup path exists: " << backupPath);
                ++backupCount;
                continue;
            }
            
            std::error_code copyErrorCode;
            ghc::filesystem::copy(userConfigPath, backupPath, copyErrorCode);
            if(copyErrorCode)
            {
                ssLOG_ERROR("Failed to backup existing user config: " << userConfigPath.string() <<
                            " with error: " << _.message());
                
                return false;
            }
            
            ssLOG_INFO("Backed up existing user config: " << backupPath);
            if(!ghc::filesystem::remove(userConfigPath, _))
            {
                ssLOG_ERROR("Failed to delete existing user config: " << userConfigPath.string());
                return false;
            }
            
            break;
        }
        while(true);
    }
    
    //Create user config
    if(writeUserConfig)
    {
        std::ofstream configFile(userConfigPath, std::ios::binary);
        if(!configFile)
        {
            ssLOG_ERROR("Failed to create default config file: " << userConfigPath.string());
            return false;
        }
        configFile.write((const char*)DefaultUserConfig, DefaultUserConfig_size);
    }
    
    if(!writeDefaultConfigs)
        return true;
    
    ghc::filesystem::path userConfigDirectory = userConfigPath;
    userConfigDirectory = userConfigDirectory.parent_path();
    ghc::filesystem::path defaultYamlDirectory = userConfigDirectory / "Default";
    
    //Default configs
    if(!ghc::filesystem::exists(defaultYamlDirectory , _))
    {
        if(!ghc::filesystem::create_directories(defaultYamlDirectory, _))
        {
            ssLOG_ERROR("Failed to create directory: " << defaultYamlDirectory.string());
            return false;
        }
    }
    
    //Writing default profiles
    auto writeDefaultConfig = 
        [&defaultYamlDirectory]
        (ghc::filesystem::path outputPath, const uint8_t* outputContent, size_t outputSize)
        {
            const ghc::filesystem::path currentOutputPath = defaultYamlDirectory / outputPath;
            std::ofstream defaultFile(  currentOutputPath.string(), 
                                        std::ios::binary | std::ios_base::trunc);
            if(!defaultFile)
            {
                ssLOG_ERROR("Failed to create default config file: " << currentOutputPath.string());
                return false;
            }
            defaultFile.write((const char*)outputContent, outputSize);
            return true;
        };
    
    if( !writeDefaultConfig("CommonFileTypes.yaml", CommonFileTypes, CommonFileTypes_size) ||
        !writeDefaultConfig("g++.yaml", G_PlusPlus, G_PlusPlus_size) ||
        !writeDefaultConfig("vs2022_v17+.yaml", Vs2022_v17Plus, Vs2022_v17Plus_size))
    {
        return false;
    }
    
    //Writing .version to indicate everything is up-to-date
    std::ofstream configVersionFile(userConfigDirectory / ".version", 
                                    std::ios::binary | std::ios_base::trunc);
    if(!configVersionFile)
    {
        ssLOG_ERROR("Failed to open version file: " << 
                    ghc::filesystem::path(userConfigDirectory / ".version").string());
        return false;
    }
    
    configVersionFile << std::to_string(RUNCPP2_CONFIG_VERSION);
    
    return true;
}

bool runcpp2::ReadUserConfig(   std::vector<Data::Profile>& outProfiles, 
                                std::string& outPreferredProfile,
                                const std::string& customConfigPath)
{
    INTERNAL_RUNCPP2_SAFE_START();

    ssLOG_FUNC_INFO();
    
    ghc::filesystem::path configPath =  !customConfigPath.empty() ? 
                                        customConfigPath : 
                                        GetConfigFilePath();
    ghc::filesystem::path configVersionPath = configPath.parent_path() / ".version";
    
    if(configPath.empty())
        return false;
    
    std::error_code e;
    
    bool writeUserConfig = false;
    bool writeDefaultConfigs = false;
    
    //Create default config files if it doesn't exist
    if(!ghc::filesystem::exists(configPath, e))
    {
        if(!customConfigPath.empty())
        {
            ssLOG_ERROR("Config file doesn't exist: " << configPath.string());
            return false;
        }
        
        ssLOG_INFO("Config file doesn't exist. Creating one at: " << configPath.string());
        writeUserConfig = true;
        writeDefaultConfigs = true;
    }
    //Overwrite default config files if it is using old version
    else if(ghc::filesystem::exists(configVersionPath, e))
    {
        std::ifstream configVersionFile(configVersionPath);
        if(!configVersionFile)
        {
            ssLOG_ERROR("Failed to open version file: " << configVersionPath.string());
            return false;
        }
        std::string configVersionStr;
        std::stringstream buffer;
        buffer << configVersionFile.rdbuf();
        configVersionStr = buffer.str();
        
        Trim(configVersionStr);
        int configVersion = std::stoi(configVersionStr);
        if(configVersion < RUNCPP2_CONFIG_VERSION)
            writeDefaultConfigs = true;
    }
    //Overwrite default config files if missing version file
    else
        writeDefaultConfigs = true;
    
    if( (writeUserConfig || writeDefaultConfigs) && 
        !WriteDefaultConfigs(configPath, writeUserConfig, writeDefaultConfigs))
    {
        return false;
    }
    
    if(ghc::filesystem::is_directory(configPath, e))
    {
        ssLOG_ERROR("Config file path is a directory: " << configPath.string());
        return false;
    }
    
    //Read compiler profiles
    std::string userConfigContent;
    {
        std::ifstream userConfigFile(configPath);
        if(!userConfigFile)
        {
            ssLOG_ERROR("Failed to open config file: " << configPath.string());
            return false;
        }
        std::stringstream buffer;
        buffer << userConfigFile.rdbuf();
        userConfigContent = buffer.str();
    }
    
    if(!ParseUserConfig(userConfigContent, configPath, outProfiles, outPreferredProfile))
    {
        ssLOG_ERROR("Failed to parse config file: " << configPath.string());
        return false;
    }

    return true;
    
    INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
}

bool runcpp2::ParseScriptInfo(  const std::string& scriptInfo, 
                                Data::ScriptInfo& outScriptInfo)
{
    INTERNAL_RUNCPP2_SAFE_START();

    if(scriptInfo.empty())
        return true;

    ryml::Tree scriptTree;
    scriptTree = ryml::parse_in_arena(scriptInfo.c_str());
    
    ryml::NodeRef rootScriptNode;
    
    if(!runcpp2::ResolveYAML_Stream(scriptTree, rootScriptNode))
        return false;
    
    if(outScriptInfo.ParseYAML_Node(rootScriptNode))
    {
        outScriptInfo.Populated = true;
        return true;
    }
    else
        return false;
    
    INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
}

