#include "runcpp2/ConfigParsing.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "runcpp2/StringUtil.hpp"
#include "runcpp2/PlatformUtil.hpp"
#include "runcpp2/YamlLib.hpp"

#include "runcpp2/LibYAML_Wrapper.hpp"
#include "runcpp2/DeferUtil.hpp"

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
    bool ResovleProfileImport_LibYaml(  runcpp2::YAML::NodePtr currentProfileNode, 
                                        const ghc::filesystem::path& configPath,
                                        runcpp2::YAML::ResourceHandle& currentYamlResources)
    {
        using namespace runcpp2;
        
        ssLOG_FUNC_INFO();
        
        ghc::filesystem::path currentImportFilePath = configPath;
        std::stack<ghc::filesystem::path> pathsToImport;
        while(  runcpp2::ExistAndHasChild_LibYaml(currentProfileNode, "Import") || 
                !pathsToImport.empty())
        {
            //If we import field, we should deal with it instead
            if(runcpp2::ExistAndHasChild_LibYaml(currentProfileNode, "Import"))
            {
                const YAML::ConstNodePtr importNode = currentProfileNode->GetMapValueNode("Import");
                if( importNode->GetType() != YAML::NodeType::Scalar && 
                    importNode->GetType() != YAML::NodeType::Sequence)
                {
                    ssLOG_ERROR("Import must be a path or sequence of paths of YAML file(s)");
                    return false;
                }
                
                ghc::filesystem::path currentImportDir = currentImportFilePath;
                currentImportDir = currentImportDir.parent_path();
                if(importNode->GetType() == YAML::NodeType::Scalar)
                {
                    std::string importPath = importNode ->GetScalar<std::string>()
                                                        .DS_TRY_ACT(return false);
                    pathsToImport.push(currentImportDir / importPath);
                }
                else
                {
                    if(importNode->GetChildrenCount() == 0)
                    {
                        ssLOG_ERROR("An import sequence cannot be an empty");
                        return false;
                    }
                    
                    for(int i = 0; i < importNode->GetChildrenCount(); ++i)
                    {
                        if(importNode->GetSequenceChildNode(i)->GetType() != YAML::NodeType::Scalar)
                        {
                            ssLOG_ERROR("It must be a sequence of paths");
                            return false;
                        }
                        
                        std::string importPath = importNode ->GetSequenceChildScalar<std::string>(i)
                                                            .DS_TRY_ACT(return false);
                        pathsToImport.push(currentImportDir / importPath);
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
            std::stringstream buffer;
            {
                std::ifstream importProfileFile(currentImportFilePath);
                if(!importProfileFile)
                {
                    ssLOG_ERROR("Failed to open profile import file: " << currentImportFilePath);
                    return false;
                }
                buffer << importProfileFile.rdbuf();
            }
            
            YAML::ResourceHandle yamlResources;
            DEFER { YAML::FreeYAMLResource(yamlResources); };
            
            std::vector<YAML::NodePtr> yamlRootNodes = 
                YAML::ParseYAML(buffer.str(), yamlResources).DS_TRY_ACT(return false);
            
            if(yamlRootNodes.empty())
                return false;
            
            for(int i = 0; i < yamlRootNodes.size(); ++i)
            {
                YAML::ResolveAnchors(yamlRootNodes[i]).DS_TRY_ACT(return false);
                YAML::NodePtr importProfileNode = yamlRootNodes[i];
                
                if(!MergeYAML_NodeChildren_LibYaml( importProfileNode, 
                                                    currentProfileNode, 
                                                    currentYamlResources))
                {
                    return false;
                }
                
                if(ExistAndHasChild_LibYaml(importProfileNode, "Import"))
                {
                    currentProfileNode->RemoveMapChild("Import").DS_TRY_ACT(return false);
                    importProfileNode   ->GetMapValueNode("Import")
                                        ->CloneToMapChild(  "Import", 
                                                            currentProfileNode,
                                                            currentYamlResources)
                                        .DS_TRY_ACT(return false);
                }
                else
                {
                    currentProfileNode->RemoveMapChild("Import").DS_TRY_ACT(return false);
                }
            }
        }   //while(  runcpp2::ExistAndHasChild_LibYaml(currentProfileNode, "Import") || 
            //        !pathsToImport.empty())
        
        return true;
    }

    DS::Result<void> GetPreferredProfile(   runcpp2::YAML::NodePtr configNode, 
                                            std::string& outPreferredProfile)
    {
        using namespace runcpp2;
        
        if(!ExistAndHasChild_LibYaml(configNode, "PreferredProfile"))
            return {};
        
        YAML::NodePtr preferredProfilesMapNode = configNode->GetMapValueNode("PreferredProfile");
        
        if(preferredProfilesMapNode->IsMap())
        {
            std::unordered_map<PlatformName, std::string> preferredProfiles;
            for(int j = 0; j < preferredProfilesMapNode->GetChildrenCount(); ++j)
            {
                PlatformName platform = preferredProfilesMapNode->GetMapKeyScalarAt<PlatformName>(j)
                                                                .DS_TRY();
                
                YAML::NodePtr valueNode = preferredProfilesMapNode->GetMapValueNodeAt(j);
                if(!valueNode->IsScalar())
                {
                    return DS_ERROR_MSG("Failed to parse PreferredProfile map. "
                                        "Keyval is expected in each platform");
                }
                preferredProfiles[platform] = valueNode ->GetScalar<std::string>().DS_TRY();
            }
            
            const std::string* selectedProfile = GetValueFromPlatformMap(preferredProfiles);
            if(!selectedProfile && !outPreferredProfile.empty())
                ssLOG_WARNING("Multiple preferred profile is found...");

            outPreferredProfile = selectedProfile != nullptr ? *selectedProfile : outPreferredProfile;
        }
        else if(preferredProfilesMapNode->IsScalar())
        {
            if(!outPreferredProfile.empty())
                ssLOG_WARNING("Multiple preferred profile is found...");
            
            outPreferredProfile = preferredProfilesMapNode->GetScalar<std::string>().DS_TRY();
        }
        else
        {
            return DS_ERROR_MSG(DS_STR("PreferredProfile needs to be a map or string value: ") +
                                DS_STR(YAML::NodeTypeToString(preferredProfilesMapNode->GetType())));
        }
        
        return {};
    }
    
    bool ParseUserConfig_LibYaml(   const std::string& userConfigString, 
                                    const ghc::filesystem::path& configPath,
                                    std::vector<runcpp2::Data::Profile>& outProfiles,
                                    std::string& outPreferredProfile)
    {
        ssLOG_FUNC_INFO();
        using namespace runcpp2;
        
        YAML::ResourceHandle parseResource;
        std::vector<YAML::NodePtr> parsedNodes = 
            YAML::ParseYAML(userConfigString, parseResource).DS_TRY_ACT(return false);
        
        DEFER { YAML::FreeYAMLResource(parseResource); };
        
        if(parsedNodes.empty())
            return false;
        
        for(int i = 0; i < parsedNodes.size(); ++i)
        {
            YAML::NodePtr configNode = parsedNodes[i];
            YAML::ResolveAnchors(configNode).DS_TRY_ACT(return false);
            
            if(!ExistAndHasChild_LibYaml(configNode, "Profiles"))
                continue;
            
            if(!configNode->GetMapValueNode("Profiles")->IsSequence())
            {
                ssLOG_ERROR("Profiles must be a sequence");
                return false;
            }
            
            YAML::NodePtr profilesNode = configNode->GetMapValueNode("Profiles");
            
            if(profilesNode->GetChildrenCount() == 0)
            {
                ssLOG_ERROR("No compiler profiles found");
                return false;
            }
            
            ssLOG_INFO(profilesNode->GetChildrenCount() << " profiles found in user config");
            
            for(int j = 0; j < profilesNode->GetChildrenCount(); ++j)
            {
                ssLOG_INFO("Parsing profile at index " << j);
                
                YAML::NodePtr currentProfileNode = profilesNode->GetSequenceChildNode(j);
                
                if(!currentProfileNode->IsMap())
                {
                    ssLOG_ERROR("Profile entry must be a map");
                    return false;
                }
                
                if(!ResovleProfileImport_LibYaml(currentProfileNode, configPath, parseResource))
                    return false;

                outProfiles.push_back({});
                if(!outProfiles.back().ParseYAML_Node(currentProfileNode))
                {
                    outProfiles.erase(outProfiles.end() - 1);
                    ssLOG_ERROR("Failed to parse compiler profile at index " << j);
                    return false;
                }
            } //for(int j = 0; j < profilesNode->GetChildrenCount(); ++j)
            
            GetPreferredProfile(configNode, outPreferredProfile).DS_TRY_ACT(return false);
        } //for(int i = 0; i < parsedNodes.size(); ++i)
        
        if(outPreferredProfile.empty())
        {
            outPreferredProfile = outProfiles.front().Name;
            ssLOG_WARNING("PreferredProfile is empty. Using the first profile name");
        }
        
        if(outProfiles.empty())
        {
            ssLOG_ERROR("No profiles registered");
            return false;
        }
        
        return true;
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
    
    if(!ParseUserConfig_LibYaml(userConfigContent, configPath, outProfiles, outPreferredProfile))
    {
        ssLOG_ERROR("Failed to parse config file: " << configPath.string());
        return false;
    }

    return true;
    
    INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
}

bool runcpp2::ParseScriptInfo_LibYaml(  const std::string& scriptInfo, 
                                        Data::ScriptInfo& outScriptInfo)
{
    INTERNAL_RUNCPP2_SAFE_START();

    if(scriptInfo.empty())
        return true;

    YAML::ResourceHandle resourceHandle;
    std::vector<YAML::NodePtr> scriptNodes = 
        YAML::ParseYAML(scriptInfo, resourceHandle).DS_TRY_ACT(return false);
    
    DEFER { YAML::FreeYAMLResource(resourceHandle); };
    if(scriptNodes.empty())
        return false;
    
    //NOTE: Use the first one
    YAML::ResolveAnchors(scriptNodes.front()).DS_TRY_ACT(return false);
    YAML::NodePtr rootScriptNode = scriptNodes.front();
    
    if(outScriptInfo.ParseYAML_Node(rootScriptNode))
    {
        outScriptInfo.Populated = true;
        return true;
    }
    else
        return false;
    
    INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
}

