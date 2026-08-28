#ifndef RUNCPP2_CONFIG_PARSING_HPP
#define RUNCPP2_CONFIG_PARSING_HPP

#include "runcpp2/Data/Profile.hpp"
#include "runcpp2/Data/ScriptInfo.hpp"
#include "runcpp2/Data/ParseCommon.hpp"
#include "runcpp2/Data/StageInfo.hpp"
#include "runcpp2/StringUtil.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "runcpp2/PlatformUtil.hpp"
#include "runcpp2/DeferUtil.hpp"
#include "runcpp2/LibYAML_Wrapper.hpp"
#include "runcpp2/ParameterUtil.hpp"

#if !INTERNAL_RUNCPP2_UNIT_TESTS || \
    INTERNAL_RUNCPP2_UNIT_TESTS != INTERNAL_RUNCPP2_UNIT_TESTS_CONFIG_PARSING

    #include "runcpp2/ResolveImport.hpp"
#endif

#include "DSResult/DSResult.hpp"

#include "cfgpath.h"
#include "ssLogger/ssLog.hpp"
#include "ghc/filesystem.hpp"

#include <string>
#include <vector>
#include <stdint.h>
#include <string.h>
#include <fstream>
#include <memory>
#include <sstream>
#include <stack>
#include <system_error>
#include <unordered_map>
#include <unordered_set>

#if defined(INTERNAL_RUNCPP2_UNIT_TESTS) && \
    INTERNAL_RUNCPP2_UNIT_TESTS == INTERNAL_RUNCPP2_UNIT_TESTS_CONFIG_PARSING
    
    #include "Tests/ConfigParsing/MockComponents.hpp"
    #include "runcpp2/ResolveImport.hpp"
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
extern "C" const uint8_t ClangPlusPlus[];
extern "C" const size_t ClangPlusPlus_size;
extern "C" const uint8_t AnnotatedG_PlusPlus[];
extern "C" const size_t AnnotatedG_PlusPlus_size;
extern "C" const uint8_t Vs2022_v17Plus[];
extern "C" const size_t Vs2022_v17Plus_size;

namespace 
{
    inline DS::Result<void> GetPreferredProfile(runcpp2::YAML::NodePtr configNode, 
                                                std::string& outPreferredProfile)
    {
        using namespace runcpp2;
        
        if(!ExistAndHasChild(configNode, "PreferredProfile"))
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
            return DS_ERROR_MSG("PreferredProfile needs to be a map or string value: " +
                                DS_STR(YAML::NodeTypeToString(preferredProfilesMapNode->GetType())));
        }
        
        return {};
    }
    
    inline DS::Result<void> ParseUserConfig(const std::string& userConfigString, 
                                            const ghc::filesystem::path& configPath,
                                            const std::unordered_map<   std::string, 
                                                                        std::string>& inputParameters,
                                            std::vector<runcpp2::Data::Profile>& outProfiles,
                                            std::string& outPreferredProfile)
    {
        ssLOG_FUNC_INFO();
        using namespace runcpp2;
        
        YAML::ResourceHandle parseResource;
        std::vector<YAML::NodePtr> parsedNodes = YAML::ParseYAML(   userConfigString, 
                                                                    parseResource).DS_TRY();
        DEFER { YAML::FreeYAMLResource(parseResource); };
        
        DS_ASSERT_FALSE(parsedNodes.empty());
        
        for(int i = 0; i < parsedNodes.size(); ++i)
        {
            YAML::NodePtr configNode = parsedNodes[i];
            YAML::ResolveAnchors(configNode).DS_TRY();
            
            if(!ExistAndHasChild(configNode, "Profiles"))
                continue;
            
            if(!configNode->GetMapValueNode("Profiles")->IsSequence())
                return DS_ERROR_MSG("Profiles must be a sequence");
            
            YAML::NodePtr profilesNode = configNode->GetMapValueNode("Profiles");
            if(profilesNode->GetChildrenCount() == 0)
                return DS_ERROR_MSG("No compiler profiles found");
            
            ssLOG_INFO(profilesNode->GetChildrenCount() << " profiles found in user config");
            for(int j = 0; j < profilesNode->GetChildrenCount(); ++j)
            {
                ssLOG_INFO("Parsing profile at index " << j);
                
                YAML::NodePtr currentProfileNode = profilesNode->GetSequenceChildNode(j);
                
                if(!currentProfileNode->IsMap())
                    return DS_ERROR_MSG("Profile entry must be a map");
                
                std::unordered_map<std::string, std::vector<std::string>> subMap;
                runcpp2::Data::Profile::PopulateCompilingLinkingParams(subMap);
                
                runcpp2::Data::Profile profile = 
                    ResolveImport<runcpp2::Data::Profile>(  currentProfileNode,
                                                            configPath.parent_path(),
                                                            parseResource,
                                                            subMap,
                                                            inputParameters).DS_TRY();
                profile .ParseYAML_Node(currentProfileNode, false, inputParameters)
                        .DS_TRY_ACT
                        (
                            DS_TMP_ERROR.Message += "\nFailed to parse compiler profile at index " + 
                                                    DS_STR(j);
                            DS_APPEND_TRACE(DS_TMP_ERROR);
                            return DS::Error(DS_TMP_ERROR)
                        );
                
                outProfiles.push_back(std::move(profile));
            } //for(int j = 0; j < profilesNode->GetChildrenCount(); ++j)
            
            GetPreferredProfile(configNode, outPreferredProfile).DS_TRY();
        } //for(int i = 0; i < parsedNodes.size(); ++i)
        
        if(outProfiles.empty())
            return DS_ERROR_MSG("No profiles registered");
        
        if(outPreferredProfile.empty())
        {
            outPreferredProfile = outProfiles.front().Name;
            ssLOG_WARNING("PreferredProfile is empty. Using the first profile name");
        }
        
        return {};
    }
}

namespace runcpp2
{
    inline DS::Result<ghc::filesystem::path> GetConfigFilePath()
    {
        CO_INSERT_IMPL(OverrideInstance, DS::Result<std::string>, ());
        
        //Check if user config exists
        char configDirC_Str[MAX_PATH] = {0};
        
        get_user_config_folder(configDirC_Str, MAX_PATH, "runcpp2");
        
        DS_ASSERT_GT(strlen(configDirC_Str), 0);
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
    
    inline DS::Result<void> WriteDefaultConfigs(const ghc::filesystem::path& userConfigPath, 
                                                const bool writeUserConfig,
                                                const bool writeDefaultConfigs)
    {
        CO_INSERT_IMPL( OverrideInstance, 
                        DS::Result<void>, 
                        (userConfigPath, writeUserConfig, writeDefaultConfigs));
        
        //Backup existing user config
        std::error_code _;
        if(writeUserConfig && ghc::filesystem::exists(userConfigPath, _))
        {
            int backupCount = 0;
            do
            {
                if(backupCount > 10)
                {
                    return DS_ERROR_MSG("Failed to backup existing user config: " + 
                                        userConfigPath.string());
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
                    return DS_ERROR_MSG("Failed to backup existing user config: " + 
                                        userConfigPath.string() + " with error: " + _.message());
                }
                
                ssLOG_INFO("Backed up existing user config: " << backupPath);
                if(!ghc::filesystem::remove(userConfigPath, _))
                {
                    return DS_ERROR_MSG("Failed to delete existing user config: " + 
                                        userConfigPath.string());
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
                return DS_ERROR_MSG("Failed to create default config file: " + userConfigPath.string());
            
            configFile.write((const char*)DefaultUserConfig, DefaultUserConfig_size);
        }
        
        if(!writeDefaultConfigs)
            return {};
        
        ghc::filesystem::path userConfigDirectory = userConfigPath;
        userConfigDirectory = userConfigDirectory.parent_path();
        ghc::filesystem::path defaultYamlDirectory = userConfigDirectory / "Default";
        
        //Default configs
        if(!ghc::filesystem::exists(defaultYamlDirectory , _))
            DS_ASSERT_TRUE(ghc::filesystem::create_directories(defaultYamlDirectory, _));
        
        //Writing default profiles
        auto writeDefaultConfig = 
            [&defaultYamlDirectory](ghc::filesystem::path outputPath, 
                                    const uint8_t* outputContent, 
                                    size_t outputSize) -> DS::Result<void>
            {
                const ghc::filesystem::path currentOutputPath = defaultYamlDirectory / outputPath;
                std::ofstream defaultFile(  currentOutputPath.string(), 
                                            std::ios::binary | std::ios_base::trunc);
                if(!defaultFile)
                {
                    return DS_ERROR_MSG("Failed to create default config file: " + 
                                        currentOutputPath.string());
                }
                defaultFile.write((const char*)outputContent, outputSize);
                return {};
            };
        
        writeDefaultConfig("CommonFileTypes.yaml", CommonFileTypes, CommonFileTypes_size).DS_TRY();
        writeDefaultConfig("g++.yaml", G_PlusPlus, G_PlusPlus_size).DS_TRY();
        writeDefaultConfig("clang++.yaml", ClangPlusPlus, ClangPlusPlus_size).DS_TRY();
        writeDefaultConfig("AnnotatedG++.yaml", AnnotatedG_PlusPlus, AnnotatedG_PlusPlus_size).DS_TRY();
        writeDefaultConfig("vs2022_v17+.yaml", Vs2022_v17Plus, Vs2022_v17Plus_size).DS_TRY();
        
        //Writing .version to indicate everything is up-to-date
        std::ofstream configVersionFile(userConfigDirectory / ".version", 
                                        std::ios::binary | std::ios_base::trunc);
        if(!configVersionFile)
        {
            return DS_ERROR_MSG("Failed to open version file: " + 
                                ghc::filesystem::path(userConfigDirectory / ".version").string());
        }
        
        configVersionFile << std::to_string(RUNCPP2_CONFIG_VERSION);
        
        return {};
    }
    
    inline DS::Result<void> ReadUserConfig( std::vector<Data::Profile>& outProfiles, 
                                            std::string& outPreferredProfile,
                                            const std::string rawParameters,
                                            const std::string& customConfigPath = "")
    {
        ssLOG_FUNC_INFO();
        
        ghc::filesystem::path configPath =  !customConfigPath.empty() ? 
                                            ghc::filesystem::path(customConfigPath) : 
                                            GetConfigFilePath().DS_VALUE_OR();
        DS_CHECK_PREV();
        ghc::filesystem::path configVersionPath = configPath.parent_path() / ".version";
        
        DS_ASSERT_FALSE(configPath.empty());
        
        std::error_code e;
        
        bool writeUserConfig = false;
        bool writeDefaultConfigs = false;
        
        //Create default config files if it doesn't exist
        if(!ghc::filesystem::exists(configPath, e))
        {
            if(!customConfigPath.empty())
                return DS_ERROR_MSG("Config file doesn't exist: " + configPath.string());
            
            ssLOG_INFO("Config file doesn't exist. Creating one at: " << configPath.string());
            writeUserConfig = true;
            writeDefaultConfigs = true;
        }
        //Overwrite default config files if it is using old version
        else if(ghc::filesystem::exists(configVersionPath, e))
        {
            std::ifstream configVersionFile(configVersionPath);
            if(!configVersionFile)
                return DS_ERROR_MSG("Failed to open version file: " + configVersionPath.string());
            
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
        
        if(writeUserConfig || writeDefaultConfigs)
        {
            WriteDefaultConfigs(configPath, writeUserConfig, writeDefaultConfigs).DS_TRY();
        }
        
        if(ghc::filesystem::is_directory(configPath, e))
            return DS_ERROR_MSG("Config file path is a directory: " + configPath.string());
        
        //Read compiler profiles
        std::string userConfigContent;
        {
            std::ifstream userConfigFile(configPath);
            if(!userConfigFile)
                return DS_ERROR_MSG("Failed to open config file: " + configPath.string());
            std::stringstream buffer;
            buffer << userConfigFile.rdbuf();
            userConfigContent = buffer.str();
        }
        
        std::unordered_map<std::string, std::string> parameterValues;
        CreateParameterValues(rawParameters, parameterValues);
        
        ParseUserConfig(userConfigContent, 
                        configPath, 
                        parameterValues, 
                        outProfiles, 
                        outPreferredProfile).DS_TRY();
        return {};
    }
    
    inline DS::Result<void> 
    ParseScriptInfo(const std::string& scriptInfo, 
                    const std::unordered_map<std::string, std::string> inputParameters,
                    Data::ScriptInfo& outScriptInfo)
    {
        if(scriptInfo.empty())
            return {};

        YAML::ResourceHandle resourceHandle;
        std::vector<YAML::NodePtr> scriptNodes = YAML::ParseYAML(scriptInfo, resourceHandle).DS_TRY();
        DEFER { YAML::FreeYAMLResource(resourceHandle); };
        DS_ASSERT_FALSE(scriptNodes.empty());
        
        //NOTE: Use the first one
        YAML::ResolveAnchors(scriptNodes.front()).DS_TRY();
        YAML::NodePtr rootScriptNode = scriptNodes.front();
        outScriptInfo.ParseYAML_Node(rootScriptNode, inputParameters).DS_TRY();
        outScriptInfo.Populated = true;
        return {};
    }
}

#if defined(INTERNAL_RUNCPP2_UNIT_TESTS) && \
    INTERNAL_RUNCPP2_UNIT_TESTS == INTERNAL_RUNCPP2_UNIT_TESTS_CONFIG_PARSING
    
    #include "Tests/ConfigParsing/UndefMocks.hpp"
#endif

#endif
