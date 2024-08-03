#include "runcpp2/ProfileHelper.hpp"
#include "ghc/filesystem.hpp"
#include "runcpp2/PlatformUtil.hpp"
#include "ssLogger/ssLog.hpp"

namespace
{
    bool IsProfileAvailableOnSystem(const runcpp2::Data::Profile& profile)
    {
        //Check compiler
        {
            //Getting PreRun command
            std::string command;
            {
                command =   runcpp2::HasValueFromPlatformMap(profile.Compiler.PreRun) ? 
                            *runcpp2::GetValueFromPlatformMap(profile.Compiler.PreRun) : "";
                
                if(!command.empty())
                    command += " && ";
            }
            
            if(!runcpp2::HasValueFromPlatformMap(profile.Compiler.CheckExistence))
            {
                ssLOG_INFO( "Compiler for profile " << profile.Name << 
                            " does not have CheckExistence for current platform");
                return false;
            }
            command += *runcpp2::GetValueFromPlatformMap(profile.Compiler.CheckExistence);
            
            std::string output;
            if(!runcpp2::RunCommandAndGetOutput(command, output))
            {
                ssLOG_INFO("Failed to find compiler for profile " << profile.Name);
                return false;
            }
        }
        
        //Check linker
        {
            //Getting PreRun command
            std::string command;
            {
                command =   runcpp2::HasValueFromPlatformMap(profile.Linker.PreRun) ? 
                            *runcpp2::GetValueFromPlatformMap(profile.Linker.PreRun) : "";
                
                if(!command.empty())
                    command += " && ";
            }
            
            if(!runcpp2::HasValueFromPlatformMap(profile.Linker.CheckExistence))
            {
                ssLOG_INFO( "Linker for profile " << profile.Name << 
                            " does not have CheckExistence for current platform");
                return false;
            }
            command += *runcpp2::GetValueFromPlatformMap(profile.Linker.CheckExistence);
            
            std::string output;
            if(!runcpp2::RunCommandAndGetOutput(command, output))
            {
                ssLOG_INFO("Failed to find linker for profile " << profile.Name);
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
