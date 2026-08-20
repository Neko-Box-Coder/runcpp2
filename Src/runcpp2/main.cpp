#include "runcpp2/Data/ParseCommon.hpp"
#include "runcpp2/Data/Profile.hpp"
#include "runcpp2/Data/ScriptInfo.hpp"

#include "runcpp2/ConfigParsing.hpp"
#include "runcpp2/StringUtil.hpp"
#include "runcpp2/runcpp2.hpp"

#include "ssLogger/ssLogInit.hpp"
#include "ssLogger/ssLog.hpp"
#include "ghc/filesystem.hpp"
#include "DSResult/DSResult.hpp"

//NOTE: #include "runcpp2/DefaultYAMLs.c" at the end

#include <cctype>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <system_error>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

struct OptionInfo
{
    std::string LongOption;
    bool ValueExists;
};

//TODO: Merge long and short options into a single structure
int ParseArgs(  const std::unordered_map<std::string, OptionInfo>& longOptionsMap,
                const std::unordered_map<std::string, const OptionInfo&>& shortOptionsMap,
                std::unordered_map<std::string, std::string>& outLongOptions,
                int argc, 
                char* argv[])
{
    int currentArgIndex = 0;
    std::string optionForCapturingValue = "";
    
    for(int i = 1; i < argc; ++i)
    {
        std::string currentArg = std::string(argv[i]);
        
        //Storing value for last option
        if(!optionForCapturingValue.empty())
        {
            //If the current argument matches one of the options, error out
            if(longOptionsMap.count(currentArg) || shortOptionsMap.count(currentArg))
            {
                ssLOG_ERROR("Missing value for option: " << optionForCapturingValue);
                return -1;
            }
            
            outLongOptions[optionForCapturingValue] = currentArg;
            optionForCapturingValue.clear();
            currentArgIndex = i;
            ssLOG_DEBUG("currentArgIndex: " << currentArgIndex);
            ssLOG_DEBUG("argv: " << argv[i]);
            continue;
        }
        
        //Matched long or short options
        if(longOptionsMap.count(currentArg) || shortOptionsMap.count(currentArg))
        {
            currentArgIndex = i;
            ssLOG_DEBUG("currentArgIndex: " << currentArgIndex);
            ssLOG_DEBUG("argv: " << argv[i]);
            const OptionInfo& currentInfo = longOptionsMap.count(currentArg) ?
                                            longOptionsMap.at(currentArg) :
                                            shortOptionsMap.at(currentArg);
            if(currentInfo.ValueExists)
                optionForCapturingValue = currentInfo.LongOption;
            else
                outLongOptions[currentInfo.LongOption] = "";
            
            continue;
        }
        else if(!currentArg.empty() && currentArg[0] == '-')
        {
            ssLOG_ERROR("Invalid option: " << currentArg);
            return -1;
        }
        else
            break;
    }
    
    ssLOG_DEBUG("returning currentArgIndex: " << currentArgIndex);
    return currentArgIndex;
}

DS::Result<void> GenerateScriptTemplate(const ghc::filesystem::path& outputFilePath)
{
    DS_ASSERT_FALSE(outputFilePath.empty());
    
    std::string defaultScriptInfo;
    runcpp2::GetDefaultScriptInfo(defaultScriptInfo);
    
    //Check if output filepath exists, if so check if it is a directory
    std::error_code e;
    if(ghc::filesystem::exists(outputFilePath, e))
    {
        if(ghc::filesystem::is_directory(outputFilePath, e))
        {
            return DS_ERROR_MSG(outputFilePath.string() + " is a directory. " +
                                "Cannot output script template to a directory");
        }
        
        //If exists, check if it is a cpp/cc file.
        std::ifstream readOutputFile(outputFilePath);
        std::stringstream buffer;
        
        if(!readOutputFile)
            return DS_ERROR_MSG("Failed to open file: " + outputFilePath.string());
        
        if(outputFilePath.extension() == ".cpp" || outputFilePath.extension() == ".cc")
        {
            //If so, prepend the script info template but wrapped in block comment
            buffer << "/* runcpp2" << std::endl << std::endl;
            buffer << defaultScriptInfo << std::endl;
            buffer << "*/" << std::endl << std::endl;
            buffer << readOutputFile.rdbuf();
        }
        //If not, check if it is yaml/yml. 
        else if(outputFilePath.extension() == ".yaml" || outputFilePath.extension() == ".yml")
        {
            //If so just prepend it normally
            buffer << defaultScriptInfo << std::endl << std::endl;
            buffer << readOutputFile.rdbuf();
        }
        //If not prepend it still but output a warning
        else
        {
            ssLOG_WARNING("Outputing script info template to non yaml file, is the intended?");
            buffer << defaultScriptInfo << std::endl << std::endl;
            buffer << readOutputFile.rdbuf();
        }
        
        readOutputFile.close();
        
        std::ofstream writeOutputFile(outputFilePath);
        if(!writeOutputFile)
            return DS_ERROR_MSG("Failed to open file: " + outputFilePath.string());

        writeOutputFile << buffer.rdbuf();
    }
    //Otherwise write it to the file
    else
    {
        std::ofstream writeOutputFile(outputFilePath);
        if(!writeOutputFile)
            return DS_ERROR_MSG("Failed to open file: " + outputFilePath.string());
        
        writeOutputFile << defaultScriptInfo;
    }
    
    return {};
}

std::string PadSpaceRight(const std::string& s, int padCol)
{
    if(s.size() > padCol)
        return s;
    return s + std::string(padCol - s.size(), ' ');
}

const int CMD_COLS_BEFORE_DESC = 56;
void PrintGeneralOptions()
{
    ssLOG_BASE( PadSpaceRight("       --log-level <level>", CMD_COLS_BEFORE_DESC) + 
                "Sets the log level (Normal, Info, Debug) for runcpp2");
}

DS::Result<bool> ProcessGeneralOptions(int argc, char* argv[], int& argIndex)
{
    if(strcmp(argv[argIndex], "--log-level") == 0)
    {
        if(argIndex == argc - 1)
            return DS_ERROR_MSG("Expecting value after --log-level");
        
        std::string level = argv[++argIndex];
        if(level == "info")
            ssLOG_SET_CURRENT_THREAD_TARGET_LEVEL(ssLOG_LEVEL_INFO);
        else if(level == "debug")
            ssLOG_SET_CURRENT_THREAD_TARGET_LEVEL(ssLOG_LEVEL_DEBUG);
        else if(level == "normal")
            ssLOG_SET_CURRENT_THREAD_TARGET_LEVEL(ssLOG_LEVEL_WARNING);
        else
            return DS_ERROR_MSG("Invalid level: " + DS_STR(level));
        
        return true;
    }
    
    return false;
}

void PrintRunBuildWatchCommonOptions(bool includeSourceOnly)
{
    ssLOG_BASE(PadSpaceRight("  -h,  --[h]elp", CMD_COLS_BEFORE_DESC) + "Show this help message");
    ssLOG_BASE( PadSpaceRight("  -l,  --[l]ocal", CMD_COLS_BEFORE_DESC) + 
                "Build in the current working directory under .runcpp2 directory");
    if(includeSourceOnly)
    {
        ssLOG_BASE( PadSpaceRight("  -s,  --[s]ource-only", CMD_COLS_BEFORE_DESC) + 
                    "Builds source files only without building dependencies.\n" +
                    PadSpaceRight("", CMD_COLS_BEFORE_DESC) + 
                    "The previous built binaries will be used for dependencies.\n" +
                    PadSpaceRight("", CMD_COLS_BEFORE_DESC) + 
                    "Requires dependencies to be built already.");
    }
    ssLOG_BASE( PadSpaceRight(   "  -p,  --[p]arameters <name1:val1:name2:val2:...>", 
                                CMD_COLS_BEFORE_DESC) +
                "Colon separated parameter name value pairs that perform text replacement on the "
                "build config");
    
    ssLOG_BASE( PadSpaceRight("  -j,  --[j]obs <number>", CMD_COLS_BEFORE_DESC) +
                "Maximum number of threads running. Defaults to 8");
    ssLOG_BASE( PadSpaceRight("  -c,  --[c]onfig <file>", CMD_COLS_BEFORE_DESC) +
                "Use specified config file instead of default");
}

DS::Result<bool> ExtractRunBuildWatchOptions(   int argc, 
                                                char* argv[],
                                                int& argIndex,
                                                bool includeSourceOnly,
                                                
                                                bool& outLocal,
                                                bool& outSourceOnly,
                                                std::string& outParams,
                                                std::string& outJobs,
                                                std::string& outConfigPath)
{
    if(strcmp(argv[argIndex], "-l") == 0 || strcmp(argv[argIndex], "--local") == 0)
        outLocal = true;
    else if(includeSourceOnly && 
            (strcmp(argv[argIndex], "-s") == 0 || strcmp(argv[argIndex], "--source-only") == 0))
    {
        outSourceOnly = true;
    }
    else if(strcmp(argv[argIndex], "-p") == 0 || strcmp(argv[argIndex], "--parameters") == 0)
    {
        if(argIndex == argc - 1)
            return DS_ERROR_MSG("Expecting value after -p or --parameters");
        outParams = argv[++argIndex];
    }
    else if(strcmp(argv[argIndex], "-j") == 0 || strcmp(argv[argIndex], "--jobs") == 0)
    {
        if(argIndex == argc - 1)
            return DS_ERROR_MSG("Expecting value after -j or --jobs");
        outJobs = argv[++argIndex];
    }
    else if(strcmp(argv[argIndex], "-c") == 0 || strcmp(argv[argIndex], "--config") == 0)
    {
        if(argIndex == argc - 1)
            return DS_ERROR_MSG("Expecting value after -c or --config");
        outConfigPath = argv[++argIndex];
    }
    else
        return false;
    
    return true;
}

DS::Result<int> HandleRun(int argc, char* argv[])
{
    //runcpp2 run <...>
    if(argc <= 2 || strcmp(argv[2], "--help") == 0 || strcmp(argv[2], "-h") == 0)
    {
        ssLOG_BASE("Usage: runcpp2 run [options] <input file> [run args]");
        ssLOG_BASE("Options:");
        
        PrintRunBuildWatchCommonOptions(true);
        ssLOG_BASE( PadSpaceRight("  -nw, --[n]o-[w]arning", CMD_COLS_BEFORE_DESC) + 
                    "Do not print any build warning");
        ssLOG_BASE( PadSpaceRight("  -q, --[q]uiet", CMD_COLS_BEFORE_DESC) + 
                    "Alias for -nw/--no-warning. See --no-warning");
        PrintGeneralOptions();
        
        return 0;
    }
    
    bool local = false;
    bool sourceOnly = false;
    std::string params = "";
    int argIndex;
    std::string jobs = "";
    std::string configPath = "";
    bool noWarning = false;
    for(argIndex = 2; argIndex < argc; ++argIndex)
    {
        bool parsed = ExtractRunBuildWatchOptions(  argc, 
                                                    argv, 
                                                    argIndex, 
                                                    true,
                                                    local, 
                                                    sourceOnly,
                                                    params,
                                                    jobs,
                                                    configPath).DS_TRY();
        if(!parsed)
        {
            if( strcmp(argv[argIndex], "-nw") == 0 || 
                strcmp(argv[argIndex], "--no-warning") == 0 ||
                strcmp(argv[argIndex], "-q") == 0 ||
                strcmp(argv[argIndex], "--quiet") == 0)
            {
                noWarning = true;
            }
            else
            {
                parsed = ProcessGeneralOptions(argc, argv, argIndex).DS_TRY();
                if(!parsed)
                    break;
            }
        }
    }
    
    if(argIndex >= argc)
        return DS_ERROR_MSG("Input file expected");
    ghc::filesystem::path script = argv[argIndex++];
    
    std::vector<std::string> scriptArgs;
    for(; argIndex < argc; ++argIndex)
    {
        ssLOG_DEBUG("argv[" << argIndex << "]: " << argv[argIndex]);
        scriptArgs.emplace_back(argv[argIndex]);
    }
    
    std::vector<runcpp2::Data::Profile> profiles;
    std::string preferredProfile;
    runcpp2::ReadUserConfig(profiles, preferredProfile, params, configPath).DS_TRY();
    
    ssLOG_DEBUG("\nprofiles:");
    for(int i = 0; i < profiles.size(); ++i)
        ssLOG_DEBUG("\n" << profiles.at(i).ToString("    "));
    
    runcpp2::Data::ScriptInfo parsedScriptInfo;
    ghc::filesystem::file_time_type finalSourceWriteTime;
    ghc::filesystem::file_time_type finalIncludeWriteTime;
    runcpp2::RunParams runParams =  { 
                                        { 
                                            script, 
                                            profiles, 
                                            params, 
                                            local, 
                                            preferredProfile 
                                        },
                                        false, 
                                        false, 
                                        sourceOnly, 
                                        false, 
                                        noWarning,
                                        scriptArgs, 
                                        jobs, 
                                        nullptr,
                                        ""
                                    };
    int result = runcpp2::Run(  runParams,
                                //Outputs
                                parsedScriptInfo,
                                finalSourceWriteTime,
                                finalIncludeWriteTime).DS_TRY();
    return result;
}

DS::Result<void> HandleBuild(int argc, char* argv[])
{
    //runcpp2 build <...>
    if(argc <= 2 || strcmp(argv[2], "--help") == 0 || strcmp(argv[2], "-h") == 0)
    {
        ssLOG_BASE("Usage: runcpp2 build [options] <input file>");
        ssLOG_BASE("Options:");
        PrintRunBuildWatchCommonOptions(true);
        ssLOG_BASE( PadSpaceRight("  -rb, --[r]e[b]uild", CMD_COLS_BEFORE_DESC) + 
                    "Deletes compiled source files cache and rebuild");
        ssLOG_BASE( PadSpaceRight("  -o,  --[o]utput-dir <output dir>", CMD_COLS_BEFORE_DESC) + 
                    "Specify a directory to output to.");
        PrintGeneralOptions();
        return {};
    }
    
    bool local = false;
    bool sourceOnly = false;
    std::string params = "";
    int argIndex;
    std::string jobs = "";
    std::string configPath = "";
    bool rebuild = false;
    ghc::filesystem::path outputDir = "";
    for(argIndex = 2; argIndex < argc; ++argIndex)
    {
        bool parsed = ExtractRunBuildWatchOptions(  argc, 
                                                    argv, 
                                                    argIndex, 
                                                    true,
                                                    local, 
                                                    sourceOnly,
                                                    params,
                                                    jobs,
                                                    configPath).DS_TRY();
        if(!parsed)
        {
            if(strcmp(argv[argIndex], "-rb") == 0 || strcmp(argv[argIndex], "--rebuild") == 0)
                rebuild = true;
            else if(strcmp(argv[argIndex], "-o") == 0 || strcmp(argv[argIndex], "--output-dir") == 0)
            {
                if(argIndex == argc - 1)
                    return DS_ERROR_MSG("Expecting value after -o or --output-dir");
                outputDir = argv[++argIndex];
            }
            else
            {
                parsed = ProcessGeneralOptions(argc, argv, argIndex).DS_TRY();
                if(!parsed)
                    break;
            }
        }
    }
    
    if(argIndex >= argc)
        return DS_ERROR_MSG("Input file expected");
    ghc::filesystem::path script = argv[argIndex++];
    
    std::vector<runcpp2::Data::Profile> profiles;
    std::string preferredProfile;
    runcpp2::ReadUserConfig(profiles, preferredProfile, params, configPath).DS_TRY();
    
    ssLOG_DEBUG("\nprofiles:");
    for(int i = 0; i < profiles.size(); ++i)
        ssLOG_DEBUG("\n" << profiles.at(i).ToString("    "));
    
    runcpp2::Data::ScriptInfo parsedScriptInfo;
    ghc::filesystem::file_time_type finalSourceWriteTime;
    ghc::filesystem::file_time_type finalIncludeWriteTime;
    runcpp2::RunParams runParams =  { 
                                        { 
                                            script, 
                                            profiles, 
                                            params, 
                                            local, 
                                            preferredProfile 
                                        },
                                        rebuild, 
                                        false, 
                                        sourceOnly, 
                                        true, 
                                        false, 
                                        {}, 
                                        jobs, 
                                        nullptr,
                                        outputDir
                                    };
    runcpp2::Run(   runParams,
                    //Outputs
                    parsedScriptInfo,
                    finalSourceWriteTime,
                    finalIncludeWriteTime).DS_TRY();
    ssLOG_BASE("Build finished");
    return {};
}

DS::Result<void> HandleWatch(int argc, char* argv[])
{
    //runcpp2 watch <input file>
    if(argc <= 2 || strcmp(argv[2], "--help") == 0 || strcmp(argv[2], "-h") == 0)
    {
        ssLOG_BASE("Usage: runcpp2 watch [options] <input file>");
        ssLOG_BASE("Options:");
        PrintRunBuildWatchCommonOptions(true);
        PrintGeneralOptions();
        return {};
    }
    
    bool local = false;
    bool sourceOnly = false;
    std::string params = "";
    int argIndex;
    std::string jobs = "";
    std::string configPath = "";
    for(argIndex = 2; argIndex < argc; ++argIndex)
    {
        bool parsed = ExtractRunBuildWatchOptions(  argc, 
                                                    argv, 
                                                    argIndex, 
                                                    true,
                                                    local, 
                                                    sourceOnly,
                                                    params,
                                                    jobs,
                                                    configPath).DS_TRY();
        parsed = ProcessGeneralOptions(argc, argv, argIndex).DS_TRY();
        if(!parsed)
            break;
    }
    
    if(argIndex >= argc)
        return DS_ERROR_MSG("Input file expected");
    ghc::filesystem::path script = argv[argIndex++];
    
    std::vector<runcpp2::Data::Profile> profiles;
    std::string preferredProfile;
    runcpp2::ReadUserConfig(profiles, preferredProfile, params, configPath).DS_TRY();
    
    ssLOG_DEBUG("\nprofiles:");
    for(int i = 0; i < profiles.size(); ++i)
        ssLOG_DEBUG("\n" << profiles.at(i).ToString("    "));
    
    runcpp2::Data::ScriptInfo* lastParsedScriptInfo = nullptr;
    runcpp2::Data::ScriptInfo parsedScriptInfo;
    ghc::filesystem::file_time_type lastFinalSourceWriteTime;
    ghc::filesystem::file_time_type lastFinalIncludeWriteTime;
    bool needsRunning = true;  //First run always needs running
    
    runcpp2::CoreParams coreParams = { script, profiles, params, local, preferredProfile };
    while(true)
    {
        //Check if sources need update
        bool needsUpdate = false;
        if(!needsRunning)   //Skip check on first run
        {
            needsRunning = runcpp2::CheckSourcesNeedUpdate( coreParams,
                                                            jobs,
                                                            lastParsedScriptInfo,
                                                            lastFinalSourceWriteTime,
                                                            lastFinalIncludeWriteTime).DS_TRY();
            if(needsUpdate)
            {
                ssLOG_INFO("Source files have changed");
                needsRunning = true;
            }
        }
        
        if(needsRunning)
        {
            //Clear the screen
            if(ssLOG_GET_CURRENT_THREAD_TARGET_LEVEL() <= ssLOG_LEVEL_WARNING)
            {
                #if defined(_WIN32)
                    system("cls");
                #else
                    //https://stackoverflow.com/a/53925508
                    std::cout << "\033c";
                #endif
            }
            
            ssLOG_LINE("Changes detected, running...");
            runcpp2::RunParams runParams 
            { 
                coreParams, 
                false, 
                true, 
                sourceOnly, 
                true, 
                false, 
                {}, 
                jobs, 
                lastParsedScriptInfo,
                ""
            };
            runcpp2::Run(   runParams,
                            //Outputs
                            parsedScriptInfo,
                            lastFinalSourceWriteTime,
                            lastFinalIncludeWriteTime)
                .DS_TRY_ACT
                (
                    //Unexpected errors
                    if( DS_TMP_ERROR.Message.find("CompileScript failed") == std::string::npos &&
                        DS_TMP_ERROR.Message.find("LinkScript failed") == std::string::npos)
                    {
                        return DS::Error(DS_APPEND_TRACE(DS_TMP_ERROR));
                    }
                );
            
            lastParsedScriptInfo = &parsedScriptInfo;
            needsRunning = false;
            ssLOG_BASE("Watching...");
        } //if(needsRunning)
        
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    
    return {};
}

DS::Result<void> HandleTemplate(int argc, char* argv[])
{
    //runcpp2 template <output path>
    if(argc <= 2 || strcmp(argv[2], "--help") == 0 || strcmp(argv[2], "-h") == 0)
    {
        ssLOG_BASE("Usage: runcpp2 template [options] <output file>");
        ssLOG_BASE("Options:");
        PrintGeneralOptions();
        return {};
    }
    
    int argIndex;
    for(argIndex = 2; argIndex < argc; ++argIndex)
    {
        bool parsed = ProcessGeneralOptions(argc, argv, argIndex).DS_TRY();
        if(!parsed)
            break;
    }
    
    if(argIndex >= argc)
        return DS_ERROR_MSG("Output file expected");
    ghc::filesystem::path templatePath = argv[argIndex++];
    
    GenerateScriptTemplate(templatePath).DS_TRY();
    ssLOG_BASE("Script template generated");
    return {};
}

DS::Result<void> HandleRegenUserConfig(int argc, char* argv[])
{
    //runcpp2 regen-user-config
    if(argc == 3 && (strcmp(argv[2], "--help") == 0 || strcmp(argv[2], "-h") == 0))
    {
        ssLOG_BASE("Usage: runcpp2 regen-user-config [options]");
        ssLOG_BASE("Options:");
        ssLOG_BASE( PadSpaceRight("  -c,  --[c]onfig-directory <directory>", CMD_COLS_BEFORE_DESC) +
                    "Use specified config directory instead of default");
        PrintGeneralOptions();
        return {};
    }
    
    int argIndex;
    for(argIndex = 2; argIndex < argc; ++argIndex)
    {
        bool parsed = ProcessGeneralOptions(argc, argv, argIndex).DS_TRY();
        if(!parsed)
            break;
    }
    
    ghc::filesystem::path configFilePath = runcpp2::GetConfigFilePath().DS_TRY();
    runcpp2::WriteDefaultConfigs(configFilePath, true, true).DS_TRY();
    ssLOG_BASE("User config regenerated");
    return {};
}

DS::Result<void> HandleReset(int argc, char* argv[])
{
    //runcpp2 reset [options] <input file>
    if(argc <= 2 || strcmp(argv[2], "--help") == 0 || strcmp(argv[2], "-h") == 0)
    {
        ssLOG_BASE("Usage: runcpp2 reset [options] <input file>");
        ssLOG_BASE("Options:");
        PrintRunBuildWatchCommonOptions(false);
        ssLOG_BASE( PadSpaceRight("  -d,  --[d]ependencies <dependencies>", 
                    CMD_COLS_BEFORE_DESC) + 
                    "Reset dependencies only (comma-separated names, or \"all\" for all dependencies)");
        PrintGeneralOptions();
        return {};
    }
    
    bool local = false;
    bool sourceOnly = false;
    std::string params = "";
    int argIndex;
    std::string jobs = "";
    std::string configPath = "";
    std::string deps = "all";
    bool depsOnly = false;
    for(argIndex = 2; argIndex < argc; ++argIndex)
    {
        bool parsed = ExtractRunBuildWatchOptions(  argc, 
                                                    argv, 
                                                    argIndex, 
                                                    false,
                                                    local, 
                                                    sourceOnly,
                                                    params,
                                                    jobs,
                                                    configPath).DS_TRY();
        if(!parsed)
        {
            if(strcmp(argv[argIndex], "-d") == 0 || strcmp(argv[argIndex], "--dependencies") == 0)
            {
                if(argIndex == argc - 1)
                    return DS_ERROR_MSG("Expecting value after -d or --dependencies");
                deps = argv[++argIndex];
                depsOnly = true;
            }
            else
            {
                parsed = ProcessGeneralOptions(argc, argv, argIndex).DS_TRY();
                if(!parsed)
                    break;
            }
        }
    }
    
    if(argIndex >= argc)
        return DS_ERROR_MSG("Input file expected");
    ghc::filesystem::path script = argv[argIndex++];
    
    std::vector<runcpp2::Data::Profile> profiles;
    std::string preferredProfile;
    runcpp2::ReadUserConfig(profiles, preferredProfile, params, configPath).DS_TRY();
    
    ssLOG_DEBUG("\nprofiles:");
    for(int i = 0; i < profiles.size(); ++i)
        ssLOG_DEBUG("\n" << profiles.at(i).ToString("    "));

    runcpp2::CoreParams coreParams = { script, profiles, params, local, preferredProfile };
    runcpp2::RunResetDependencies(coreParams, deps).DS_TRY();
    if(!depsOnly)
    {
        runcpp2::RunCleanup(coreParams).DS_TRY();
    }
    
    ssLOG_BASE("Reset finished");
    return {};
}

DS::Result<int> Main(int argc, char* argv[])
{
    INTERNAL_RUNCPP2_SAFE_START()
    
    ssLOG_SET_CURRENT_THREAD_TARGET_LEVEL(ssLOG_LEVEL_WARNING);
    
    //Help
    if( argc == 1 || 
        strcmp(argv[1], "--help") == 0 || 
        strcmp(argv[1], "-h") == 0 || 
        strcmp(argv[1], "help") == 0)
    {
        ssLOG_BASE("Usage: runcpp2 <action> [options] [input file] [run args]");
        ssLOG_BASE("Actions:");
        ssLOG_BASE(PadSpaceRight("    run", CMD_COLS_BEFORE_DESC) + "Runs the input file");
        ssLOG_BASE(PadSpaceRight("    build", CMD_COLS_BEFORE_DESC) + "Build the input file");
        ssLOG_BASE( PadSpaceRight("    watch", CMD_COLS_BEFORE_DESC) + 
                    "Watch for any changes in source files and output any compiling errors");
        ssLOG_BASE( PadSpaceRight("    template", CMD_COLS_BEFORE_DESC) + 
                    "Creates/prepend runcpp2 build info template to the input file");
        ssLOG_BASE( PadSpaceRight("    regen-user-config", CMD_COLS_BEFORE_DESC) + 
                    "Replace current user config with the default one");
        ssLOG_BASE( PadSpaceRight("    reset", CMD_COLS_BEFORE_DESC) +
                    "Perform cleanup on both/either the source and/or the dependencies");
        ssLOG_BASE( PadSpaceRight("    show-config-path", CMD_COLS_BEFORE_DESC) + 
                    "Show where runcpp2 is reading the config from");
        ssLOG_BASE(PadSpaceRight("    version", CMD_COLS_BEFORE_DESC) + "Show the version of runcpp2");
        ssLOG_BASE(PadSpaceRight("    tutorial", CMD_COLS_BEFORE_DESC) + "Start interactive tutorial");
        ssLOG_BASE(PadSpaceRight("    help", CMD_COLS_BEFORE_DESC) + "Show this help message");
        return 0;
    }
    
    if(strcmp(argv[1], "run") == 0)
    {
        int result = HandleRun(argc, argv).DS_TRY();
        return result;
    }
    else if(strcmp(argv[1], "build") == 0)
    {
        HandleBuild(argc, argv).DS_TRY();
    }
    else if(strcmp(argv[1], "watch") == 0)
    {
        HandleWatch(argc, argv).DS_TRY();
    }
    else if(strcmp(argv[1], "template") == 0)
    {
        HandleTemplate(argc, argv).DS_TRY();
    }
    else if(strcmp(argv[1], "regen-user-config") == 0)
    {
        HandleRegenUserConfig(argc, argv).DS_TRY();
    }
    else if(strcmp(argv[1], "reset") == 0)
    {
        HandleReset(argc, argv).DS_TRY();
    }
    else if(strcmp(argv[1], "show-config-path") == 0)
    {
        ghc::filesystem::path configFilePath = runcpp2::GetConfigFilePath().DS_TRY();
        ssLOG_BASE(configFilePath.string());
    }
    else if(strcmp(argv[1], "version") == 0)
        ssLOG_BASE("runcpp2 version " << RUNCPP2_VERSION);
    else if(strcmp(argv[1], "tutorial") == 0)
    {
        runcpp2::DownloadTutorial(argv[0]).DS_TRY();
    }
    else
        return DS_ERROR_MSG("Invalid action: " + DS_STR(argv[1]));
    
    return 0;
    
    INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(-1)
}

int main(int argc, char* argv[])
{
    int result = Main(argc, argv).DS_TRY_ACT(ssLOG_ERROR(DS_TMP_ERROR.ToString()); return 1);
    return result;
}

#include "runcpp2/DefaultYAMLs.c"
