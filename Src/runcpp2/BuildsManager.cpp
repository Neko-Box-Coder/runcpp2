#include "runcpp2/BuildsManager.hpp"
#include "ssLogger/ssLog.hpp"

#include <sstream>
#include <cassert>

#if INTERNAL_RUNCPP2_UNIT_TESTS
    #include "UnitTests/BuildsManager/MockComponents.hpp"
#else
    #define CO_NO_OVERRIDE 1
    #include "CppOverride.hpp"
#endif

namespace runcpp2
{
    bool BuildsManager::ParseMappings(const std::string& mappingsContent)
    {
        ssLOG_FUNC_DEBUG();
        
        std::error_code e;
        std::istringstream iss(mappingsContent);
        std::string line;
        
        //Read each line in the mappings file, and extract the mappings
        while(std::getline(iss, line))
        {
            if(line.empty())
                continue;
            
            std::size_t foundComma = line.find(",");
            
            if(foundComma == std::string::npos)
            {
                ssLOG_ERROR("Failed to parse line: " << line);
                return false;
            }
            
            std::string scriptPath = line.substr(0, foundComma);
            
            if(!ghc::filesystem::path(scriptPath).is_absolute())
            {
                ssLOG_ERROR("Script path " << scriptPath << " is not absolute");
                return false;
            }
            
            if(foundComma == line.size() - 1)
            {
                ssLOG_ERROR("End of line reached, mapped path not found");
                ssLOG_ERROR("Failed to parse line" << line);
                return false;
            }
            
            std::string mappedPath = line.substr(foundComma + 1, line.size() - foundComma - 1);
            
            if(!ghc::filesystem::path(mappedPath).is_relative())
            {
                ssLOG_ERROR("Mapped path " << mappedPath << " is not relative");
                return false;
            }
            
            ssLOG_DEBUG("BuildDirectory / mappedPath: " << 
                        ghc::filesystem::path(BuildDirectory / mappedPath).string());
            
            //We don't add the mapped path to record if it doesn't exist
            if(!ghc::filesystem::exists(BuildDirectory / mappedPath, e))
                continue;
            
            Mappings[scriptPath] = mappedPath;
            ReverseMappings[mappedPath] = scriptPath;
        }
        
        return true;
    }
    
    std::string BuildsManager::ProcessPath(const std::string& path)
    {
        std::string processedPath;
        for(int i = 0; i < path.size(); ++i)
        {
            if(path[i] == '\\')
                processedPath += '/';
            else
                processedPath += path[i];
        }
        
        return processedPath;
    };
    
    BuildsManager::BuildsManager(const ghc::filesystem::path& configDirectory) : 
        ConfigDirectory(configDirectory),
        Mappings(),
        BuildDirectory(configDirectory / "CachedBuilds"),
        MappingsFile(BuildDirectory / "Mappings.csv"),
        Initialized(false)
    {}
    
    BuildsManager::~BuildsManager()
    {}
    
    bool BuildsManager::Initialize()
    {
        ssLOG_FUNC_DEBUG();

        std::error_code e;
        
        ssLOG_INFO("MappingsFile: " << MappingsFile.string());
        ssLOG_INFO("BuildDirectory: " << BuildDirectory.string());
        
        //Read the build mappings if exist, if not create it.
        if(ghc::filesystem::exists(MappingsFile, e))
        {
            std::ifstream inputFile(MappingsFile);
            if (!inputFile.is_open())
            {
                ssLOG_ERROR("Failed to open file: " << MappingsFile.string());
                return false;
            }
            std::stringstream buffer;
            buffer << inputFile.rdbuf();
            std::string source(buffer.str());
            inputFile.close();
        
            if(!ParseMappings(source))
                return false;
        }
        else
        {
            //Create the Builds directory
            if(!ghc::filesystem::exists(BuildDirectory, e))
            {
                if(!ghc::filesystem::create_directories(BuildDirectory, e))
                {
                    ssLOG_ERROR("Failed to create directory " << BuildDirectory.string());
                    return false;
                }
            }
            
            std::ofstream newFile(MappingsFile);
            
            if(!newFile.is_open())
            {
                ssLOG_ERROR("Failed to open file: " << MappingsFile.string());
                return false;
            }
            
            newFile.close();
        }
        
        Initialized = true;
        return true;
    }
    
    bool BuildsManager::CreateBuildMapping(const ghc::filesystem::path& scriptPath)
    {
        CO_OVERRIDE_MEMBER_IMPL(OverrideInstance, bool, (scriptPath));
        
        ssLOG_FUNC_DEBUG();
        
        if(!Initialized)
            return false;
        
        if(scriptPath.is_relative())
        {
            ssLOG_ERROR("Cannot create build mapping with relative path");
            return false;
        }
        
        std::string processScriptPathStr = ProcessPath(scriptPath.string());
        
        if(Mappings.count(processScriptPathStr) > 0)
            return true;
        
        //Create build folder for script
        constexpr int MAX_TRIES = 16;
        std::size_t scriptPathHash;
        std::string scriptBuildPathStr;
        int i = 0;
        do
        {
            scriptPathHash = std::hash<std::string>{}(processScriptPathStr);
            scriptBuildPathStr = "./" + std::to_string(scriptPathHash);
            
            if(ReverseMappings.count(scriptBuildPathStr) == 0)
                break;
            else if(i == MAX_TRIES - 1)
            {
                ssLOG_ERROR("Failed to get unique hash for " << scriptPath.string());
                return false;
            }
        }
        while(i++ < MAX_TRIES);
        
        ghc::filesystem::path scriptBuildPath = BuildDirectory / scriptBuildPathStr;
        std::error_code e;
        
        //Removing existing build folder if exists
        if(ghc::filesystem::exists(scriptBuildPath, e))
        {
            if(!ghc::filesystem::remove_all(scriptBuildPath, e))
            {
                ssLOG_ERROR("Failed to remove " << scriptBuildPath);
                return false;
            }
        }
        
        //Create the build folder we need
        if(!ghc::filesystem::create_directories(scriptBuildPath, e))
        {
            ssLOG_ERROR("Failed to create directory " << scriptBuildPath.string());
            return false;
        }
        
        ssLOG_INFO("Build path " << scriptBuildPath << " for " << scriptPath);
        Mappings[processScriptPathStr] = scriptBuildPathStr;
        ReverseMappings[scriptBuildPathStr] = processScriptPathStr;
        return true;
    }

    bool BuildsManager::RemoveBuildMapping(const ghc::filesystem::path& scriptPath)
    {
        if(!Initialized)
            return false;
        
        if(scriptPath.is_relative())
        {
            ssLOG_ERROR("Cannot have build mapping with relative path");
            return false;
        }
        
        std::string processScriptPathStr = ProcessPath(scriptPath.string());
        if(Mappings.count(processScriptPathStr) == 0)
            return true;
        
        ReverseMappings.erase(Mappings.at(processScriptPathStr));
        Mappings.erase(processScriptPathStr);
        assert(ReverseMappings.size() == Mappings.size());
        return true;
    }

    bool BuildsManager::HasBuildMapping(const ghc::filesystem::path& scriptPath)
    {
        if(!Initialized)
            return false;
        
        if(scriptPath.is_relative())
        {
            ssLOG_ERROR("Cannot have build mapping with relative path");
            return false;
        }
        
        return Mappings.count(ProcessPath(scriptPath.string()));
    }
    
    bool BuildsManager::GetBuildMapping(const ghc::filesystem::path& scriptPath, 
                                        ghc::filesystem::path& outPath)
    {
        if(!Initialized)
            return false;
        
        if(scriptPath.is_relative())
        {
            ssLOG_ERROR("Cannot have build mapping with relative path");
            return false;
        }
        
        //If it doesn't exist, create the mapping
        if(!Mappings.count(ProcessPath(scriptPath.string())))
        {
            if(!CreateBuildMapping(scriptPath))
                return false;
        }
    
        outPath = BuildDirectory.string() + "/" + Mappings.at(ProcessPath(scriptPath.string()));
        return true;
    }
    
    bool BuildsManager::RemoveAllBuildsMappings()
    {
        if(!Initialized)
            return false;
        
        Mappings.clear();
        ReverseMappings.clear();
        return true;
    }
    
    bool BuildsManager::SaveBuildsMappings()
    {
        if(!Initialized)
            return false;
        
        std::ofstream buildsMappings(MappingsFile);
        
        if(!buildsMappings.is_open())
        {
            ssLOG_ERROR("Failed to open file: " << MappingsFile.string());
            return false;
        }
        
        for(std::unordered_map<std::string, std::string>::iterator it = Mappings.begin(); 
            it != Mappings.end(); ++it)
        {
            ssLOG_DEBUG("Writing mapping: " << it->first << "," << it->second);
            buildsMappings << it->first << "," << it->second << std::endl;
        }
        
        buildsMappings.close();
        return true;
    }
}



