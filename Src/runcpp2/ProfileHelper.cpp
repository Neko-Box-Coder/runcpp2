#include "runcpp2/ProfileHelper.hpp"
#include "ghc/filesystem.hpp"
#include "runcpp2/PlatformUtil.hpp"
#include "ssLogger/ssLog.hpp"
#include "System2.h"

namespace
{
    bool IsProfileAvailableOnSystem(const runcpp2::Data::Profile& profile)
    {
        //Check compiler
        {
            //Getting EnvironmentSetup command
            std::string command;
            {
                command =   runcpp2::HasValueFromPlatformMap(profile.Compiler.EnvironmentSetup) ? 
                            *runcpp2::GetValueFromPlatformMap(profile.Compiler.EnvironmentSetup) : "";
                
                if(!command.empty())
                    command += " && ";
            }
            
            command += profile.Compiler.CheckExistence;
            
            ssLOG_DEBUG("Running: " << command);
            System2CommandInfo compilerCommandInfo = {};
            compilerCommandInfo.RedirectOutput = true;
            SYSTEM2_RESULT sys2Result = System2Run(command.c_str(), &compilerCommandInfo);
            
            if(sys2Result != SYSTEM2_RESULT_SUCCESS)
            {
                ssLOG_ERROR("System2Run failed with result: " << sys2Result);
                return false;
            }
            
            std::vector<char> output;
            
            do
            {
                uint32_t byteRead = 0;
                output.resize(output.size() + 4096);
                
                sys2Result = System2ReadFromOutput( &compilerCommandInfo, 
                                                    output.data() + output.size() - 4096, 
                                                    4096 - 1, 
                                                    &byteRead);
    
                output.resize(output.size() - 4096 + byteRead + 1);
                output.back() = '\0';
            }
            while(sys2Result == SYSTEM2_RESULT_READ_NOT_FINISHED);
            
            int returnCode = 0;
            sys2Result = System2GetCommandReturnValueSync(&compilerCommandInfo, &returnCode);
            if(sys2Result != SYSTEM2_RESULT_SUCCESS)
            {
                ssLOG_ERROR("System2GetCommandReturnValueSync failed with result: " << sys2Result);
                return false;
            }
            
            if(returnCode != 0)
            {
                ssLOG_DEBUG("Failed on compiler check");
                ssLOG_DEBUG("Output: " << output.data());
                return false;
            }
        }
        
        //Check linker
        {
            //Getting EnvironmentSetup command
            std::string command;
            {
                command =   runcpp2::HasValueFromPlatformMap(profile.Linker.EnvironmentSetup) ? 
                            *runcpp2::GetValueFromPlatformMap(profile.Linker.EnvironmentSetup) : "";
                
                if(!command.empty())
                    command += " && ";
            }
            
            command += profile.Linker.CheckExistence;
            ssLOG_DEBUG("Running: " << command);
            System2CommandInfo linkerCommandInfo = {};
            linkerCommandInfo.RedirectOutput = true;
            SYSTEM2_RESULT sys2Result = System2Run(command.c_str(), &linkerCommandInfo);
            
            if(sys2Result != SYSTEM2_RESULT_SUCCESS)
            {
                ssLOG_ERROR("System2Run failed with result: " << sys2Result);
                return false;
            }
            
            std::vector<char> output;
            
            do
            {
                uint32_t byteRead = 0;
                output.resize(output.size() + 4096);
                
                sys2Result = System2ReadFromOutput( &linkerCommandInfo, 
                                                    output.data() + output.size() - 4096, 
                                                    4096 - 1, 
                                                    &byteRead);
    
                output.resize(output.size() - 4096 + byteRead + 1);
                output.back() = '\0';
            }
            while(sys2Result == SYSTEM2_RESULT_READ_NOT_FINISHED);
            
            int returnCode = 0;
            sys2Result = System2GetCommandReturnValueSync(&linkerCommandInfo, &returnCode);
            if(sys2Result != SYSTEM2_RESULT_SUCCESS)
            {
                ssLOG_ERROR("System2GetCommandReturnValueSync failed with result: " << sys2Result);
                return false;
            }
            
            if(returnCode != 0)
            {
                ssLOG_DEBUG("Failed on linker check");
                ssLOG_DEBUG("Output: " << output.data());
                return false;
            }
        }

        return true;
    }

    bool IsProfileValidForScript(   const runcpp2::Data::Profile& profile, 
                                    const runcpp2::Data::ScriptInfo& scriptInfo, 
                                    const std::string& scriptPath)
    {
        std::string scriptExtension = ghc::filesystem::path(scriptPath).extension().string();
        if(!scriptExtension.empty())
            scriptExtension.erase(0, 1);
        
        if(profile.FileExtensions.find(scriptExtension) == profile.FileExtensions.end())
        {
            ssLOG_DEBUG("Failed to match file extension for " << scriptExtension);
            return false;
        }
        
        if(!scriptInfo.Language.empty())
        {
            if(profile.Languages.find(scriptInfo.Language) == profile.Languages.end())
            {
                ssLOG_DEBUG("Failed to match language for " << scriptInfo.Language);
                return false;
            }
        }
        
        if(!scriptInfo.RequiredProfiles.empty())
        {
            if(!runcpp2::HasValueFromPlatformMap(scriptInfo.RequiredProfiles))
            {
                ssLOG_ERROR("Required profile is not listed for current platform");
                return false;
            }
            
            const std::vector<ProfileName>& allowedProfileNames = 
                *runcpp2::GetValueFromPlatformMap(scriptInfo.RequiredProfiles);

            for(int j = 0; j < allowedProfileNames.size(); ++j)
            {
                if( allowedProfileNames.at(j) == profile.Name || 
                    profile.NameAliases.count(allowedProfileNames.at(j)) > 0)
                {
                    ssLOG_DEBUG("Allowed profile found");
                    return true;
                }
            }
            
            //If we went through all the specified platform names for required profiles, exit
            ssLOG_DEBUG("Profile not allowed");
            return false;
        }
        
        ssLOG_DEBUG("Allowing current profile as there are no requirements found");
        return true;
    }

    std::vector<int> GetAvailableProfiles(  const std::vector<runcpp2::Data::Profile>& profiles,
                                            const runcpp2::Data::ScriptInfo& scriptInfo,
                                            const std::string& scriptPath)
    {
        //Check which profile is available
        std::vector<int> availableProfiles;
        
        for(int i = 0; i < profiles.size(); ++i)
        {
            ssLOG_DEBUG("Checking profile: " << profiles.at(i).Name);
            
            if( IsProfileAvailableOnSystem(profiles.at(i)) && 
                IsProfileValidForScript(profiles.at(i), scriptInfo, scriptPath))
            {
                availableProfiles.push_back(i);
            }
        }
        
        return availableProfiles;
    }
}


int runcpp2::GetPreferredProfileIndex(  const std::string& scriptPath, 
                                        const Data::ScriptInfo& scriptInfo,
                                        const std::vector<Data::Profile>& profiles, 
                                        const std::string& configPreferredProfile)
{
    std::vector<int> availableProfiles = GetAvailableProfiles(profiles, scriptInfo, scriptPath);
    
    if(availableProfiles.empty())
    {
        ssLOG_ERROR("No compilers/linkers found");
        return -1;
    }
    
    int firstAvailableProfileIndex = -1;
    
    if(!configPreferredProfile.empty())
    {
        for(int i = 0; i < availableProfiles.size(); ++i)
        {
            if(firstAvailableProfileIndex == -1)
                firstAvailableProfileIndex = availableProfiles.at(i);
            
            if( profiles.at(availableProfiles.at(i)).Name == configPreferredProfile || 
                profiles.at(availableProfiles.at(i)).NameAliases.count(configPreferredProfile) > 0)
            {
                return availableProfiles.at(i);
            }
        }
    }
    
    return firstAvailableProfileIndex;
}
