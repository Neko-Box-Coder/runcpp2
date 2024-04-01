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
        std::string command = profile.Compiler.Executable + " -v";
        
        System2CommandInfo compilerCommandInfo;
        SYSTEM2_RESULT sys2Result = System2Run(command.c_str(), &compilerCommandInfo);
        
        if(sys2Result != SYSTEM2_RESULT_SUCCESS)
        {
            ssLOG_ERROR("System2Run failed with result: " << sys2Result);
            return false;
        }
        
        int returnCode = 0;
        sys2Result = System2GetCommandReturnValueSync(&compilerCommandInfo, &returnCode);
        if(sys2Result != SYSTEM2_RESULT_SUCCESS)
        {
            ssLOG_ERROR("System2GetCommandReturnValueSync failed with result: " << sys2Result);
            return false;
        }
        
        if(returnCode != 0)
            return false;
        
        //Check linker
        command = profile.Linker.Executable + " -v";
        
        System2CommandInfo linkerCommandInfo;
        sys2Result = System2Run(command.c_str(), &linkerCommandInfo);
        
        if(sys2Result != SYSTEM2_RESULT_SUCCESS)
        {
            ssLOG_ERROR("System2Run failed with result: " << sys2Result);
            return false;
        }
        
        returnCode = 0;
        sys2Result = System2GetCommandReturnValueSync(&linkerCommandInfo, &returnCode);
        if(sys2Result != SYSTEM2_RESULT_SUCCESS)
        {
            ssLOG_ERROR("System2GetCommandReturnValueSync failed with result: " << sys2Result);
            return false;
        }
        
        if(returnCode != 0)
            return false;

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
            return false;
        
        if(!scriptInfo.Language.empty())
        {
            if(profile.Languages.find(scriptInfo.Language) == profile.Languages.end())
                return false;
        }
        
        if(!scriptInfo.RequiredProfiles.empty())
        {
            if(!runcpp2::HasValueFromPlatformMap(scriptInfo.RequiredProfiles))
                return false;
            
            const std::vector<ProfileName>& allowedProfileNames = 
                *runcpp2::GetValueFromPlatformMap(scriptInfo.RequiredProfiles);

            for(int j = 0; j < allowedProfileNames.size(); ++j)
            {
                if(allowedProfileNames.at(j) == profile.Name)
                    return true;
            }
            
            //If we went through all the specified platform names for required profiles, exit
            return false;
        }
        
        return true;
    }

    std::vector<ProfileName> 
    GetAvailableProfiles(   const std::vector<runcpp2::Data::Profile>& profiles,
                            const runcpp2::Data::ScriptInfo& scriptInfo,
                            const std::string& scriptPath)
    {
        //Check which compiler is available
        std::vector<ProfileName> availableProfiles;
        
        for(int i = 0; i < profiles.size(); ++i)
        {
            if( IsProfileAvailableOnSystem(profiles.at(i)) && 
                IsProfileValidForScript(profiles.at(i), scriptInfo, scriptPath))
            {
                availableProfiles.push_back(profiles.at(i).Name);
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
    std::vector<ProfileName> availableProfiles = GetAvailableProfiles(  profiles, 
                                                                        scriptInfo, 
                                                                        scriptPath);
    
    if(availableProfiles.empty())
    {
        ssLOG_ERROR("No compilers/linkers found");
        return -1;
    }
    
    int firstAvailableProfileIndex = -1;
    
    if(!configPreferredProfile.empty())
    {
        for(int i = 0; i < profiles.size(); ++i)
        {
            bool available = false;
            for(int j = 0; j < availableProfiles.size(); ++j)
            {
                if(availableProfiles.at(j) == profiles.at(i).Name)
                {
                    available = true;
                    break;
                }
            }
            
            if(!available)
                continue;
            
            if(firstAvailableProfileIndex == -1)
                firstAvailableProfileIndex = i;
            
            if(profiles.at(i).Name == configPreferredProfile)
                return i;
        }
    }
    
    return firstAvailableProfileIndex;
}