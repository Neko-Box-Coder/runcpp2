/* runcpp2
Defines: ["ssLOG_USE_SOURCE=1"]
Dependencies:
-   Name: ssLogger
    Platforms: [DefaultPlatform]
    Source:
        Git:
            URL: "https://github.com/Neko-Box-Coder/ssLogger.git"
    LibraryType: Header
    IncludePaths: ["Include"]
    Defines:
    -   "_CRT_SECURE_NO_WARNINGS=1"

-   Name: "System2.cpp"
    Platforms: [DefaultPlatform]
    Source:
        Git:
            URL: "https://github.com/Neko-Box-Coder/System2.cpp.git"
    LibraryType: Header
    IncludePaths: [".", "./External/System2"]

-   Name: "filesystem"
    Platforms: [DefaultPlatform]
    Source:
        Git:
            URL: "https://github.com/gulrak/filesystem.git"
            Branch: "v1.5.14"
    LibraryType: Header
    IncludePaths: ["./include"]
*/

#include "ssLogger/ssLogInit.hpp"
#include "ssLogger/ssLog.hpp"
#include "System2.hpp"
#include "ghc/filesystem.hpp"
#include <fstream>
#include <thread>
#include <chrono>

std::string runcpp2ExecutablePath = "";
bool integrationTest = false;

#define DELAYED_OUTPUT(str) \
    do \
    { \
        if(!integrationTest) \
            std::this_thread::sleep_for(std::chrono::milliseconds(1000)); \
        ssLOG_BASE(str); \
    } while(0)

std::string GetInput(bool allowEmpty = false)
{
    if(integrationTest)
    {
        if(!allowEmpty)
            ssLOG_ERROR("GetInput() called with allowEmpty set to false for testing");
        
        DELAYED_OUTPUT("");
        return "";
    }
    
    std::string input;
    bool triggered = false;
    do
    {
        if(triggered && !allowEmpty)
            DELAYED_OUTPUT("Empty input is not allowed.");
        
        if(!std::getline(std::cin, input)) 
        {
            ssLOG_ERROR("IO Error when trying to get cin");
            return ""; 
        }
        
        triggered = true;
    }
    while(!allowEmpty && input.empty());
    return input;
}

bool GetYN_WithDefault(bool defaultY)
{
    if(integrationTest)
    {
        DELAYED_OUTPUT("y");
        return defaultY;
    }
    
    while(true)
    {
        std::string input = GetInput(true);
        if(input == "y")
            return true;
        else if(input == "n")
            return false;
        else if(input.empty())
            return defaultY;
        else
            DELAYED_OUTPUT("Input must either be y or n");
    }
}

bool GetYN()
{
    while(true)
    {
        std::string input = GetInput();
        if(input == "y")
            return true;
        else if(input == "n")
            return false;
        else
            DELAYED_OUTPUT("Input must either be y or n");
    }
}


bool InitializeRuncpp2ExecutablePath()
{
    if(!runcpp2ExecutablePath.empty())
        return true;
    
    while(true)
    {
        System2CommandInfo commandInfo = {0};
        SYSTEM2_RESULT result = System2CppRun("runcpp2 --version", commandInfo);
        if(result != SYSTEM2_RESULT_SUCCESS)
            return false;
        
        int returnCode = 0;
        result = System2CppGetCommandReturnValueSync(commandInfo, returnCode, false);
        if(result != SYSTEM2_RESULT_SUCCESS)
            return false;

        if(returnCode == 0)
        {
            runcpp2ExecutablePath = "runcpp2";
            DELAYED_OUTPUT("Successfully found runcpp2 executable");
            return true;
        }
        
        DELAYED_OUTPUT("It seems that runcpp2 is not added to your PATH environment variable.");
        DELAYED_OUTPUT( "You can either specify the path to runcpp2 executable "
                        "or add it to your PATH environment variable.");
        DELAYED_OUTPUT("Do you want to specify the path to runcpp2 executable? [y/n]:");
        bool ans = GetYN();
        if(ans)
        {
            DELAYED_OUTPUT("Please enter the path to the runcpp2 executable:");
            std::string path = GetInput();
            if(!ghc::filesystem::exists(path))
            {
                DELAYED_OUTPUT("The path you entered does not exist.");
                return false;
            }
            
            runcpp2ExecutablePath = path;
            return true;
        }
        else
        {
            DELAYED_OUTPUT("Please add runcpp2 to your PATH environment variable now");
            DELAYED_OUTPUT("Press enter to continue...");
            GetInput(true);
        }
    }

    return true;
}

bool RunCommand(const std::string& command, 
                bool expectingZeroReturnValue = true, 
                bool convertSlashes = true)
{
    (void)convertSlashes;
    std::string updatedCommand = command;
    #ifdef _WIN32
    if(convertSlashes)
    {
        std::string::size_type pos = 0;
        while((pos = updatedCommand.find('/', pos)) != std::string::npos)
        {
            updatedCommand.replace(pos, 1, "\\");
            pos++;
        }
    }
    #endif
    
    DELAYED_OUTPUT( "-----------------------\n"
                    "Output:\n"
                    "-----------------------");
    System2CommandInfo commandInfo = {0};
    SYSTEM2_RESULT result = System2CppRun(updatedCommand, commandInfo);
    if(result != SYSTEM2_RESULT_SUCCESS)
    {
        ssLOG_ERROR("Failed to run the command: " << updatedCommand);
        ssLOG_ERROR("SYSTEM2_RESULT: " << result);
        return false;
    }
    
    int returnCode = 0;
    result = System2CppGetCommandReturnValueSync(commandInfo, returnCode, false);
    if(result != SYSTEM2_RESULT_SUCCESS)
    {
        ssLOG_ERROR("Command failed: " << updatedCommand);
        ssLOG_ERROR("Failed to get the return value of the command: " << result);
        return false;
    }

    if(expectingZeroReturnValue && returnCode != 0)
    {
        ssLOG_ERROR("Command failed: " << updatedCommand);
        ssLOG_ERROR("The command returned a non-zero return value: " << returnCode);
        return false;
    }
    
    DELAYED_OUTPUT("");
    DELAYED_OUTPUT("Press enter to continue...");
    GetInput(true);

    return true;
}

bool RunCommandWithPrompt(  const std::string& command, 
                            bool expectingZeroReturnValue = true, 
                            bool convertSlashes = true)
{
    //DELAYED_OUTPUT("> " << command << " # Press enter to continue...");
    std::cout << "> " << command << "       # Press enter to continue...";
    GetInput(true);
    DELAYED_OUTPUT("");

    return RunCommand(command, expectingZeroReturnValue, convertSlashes);
}

std::string ReadFile(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    if(!file)
        return "";
    
    file.seekg(0, std::ios::end);
    std::string content;
    content.resize(file.tellg());
    file.seekg(0, std::ios::beg);
    file.read(&content[0], content.size());
    return content;
}

bool WriteFile(const std::string& path, const std::string& content)
{
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if(!file)
        return false;
    file << content;
    return true;
}

bool Chapter1_Runcpp2Executable()
{
    DELAYED_OUTPUT( "===========================================\n"
                    "Chapter 1/3: Using runcpp2 executable\n"
                    "===========================================\n");
    
    DELAYED_OUTPUT("Let's start by creating a \"tutorial\" directory.");
    DELAYED_OUTPUT( "If you want to be in a different directory, "
                    "stop the script now (ctrl+c) and run it in the directory you want.");
    DELAYED_OUTPUT("Press enter to continue...");
    GetInput(true);
    
    std::error_code ec;
    if(!ghc::filesystem::exists("tutorial", ec))
    {
        if(!ghc::filesystem::create_directory("tutorial", ec))
        {
            ssLOG_ERROR("Failed to create tutorial directory: " << ec.message());
            return false;
        }
        
        DELAYED_OUTPUT("Created directory \"tutorial\"");
    }
    
    DELAYED_OUTPUT( "From now on, all the files related to this interactive tutorial will be placed "
                    "in the \"tutorial\" directory");
    DELAYED_OUTPUT("You can open your IDE/Editor of choice in the \"tutorial\" directory.");
    DELAYED_OUTPUT( "It is recommended to have both this tutorial and your IDE/Editor visible at the "
                    "same time.");
    DELAYED_OUTPUT("Press enter to continue...");
    GetInput(true);

    if(!ghc::filesystem::exists("tutorial/main.cpp", ec))
    {
        DELAYED_OUTPUT("First let me create a very simple C++ file (\"tutorial/main.cpp\").");
        DELAYED_OUTPUT("Press enter to continue...");
        GetInput(true);
        
        std::string cppCode = R"(#include <iostream>

int main(int, char**)
{
    std::cout << "Hello World" << std::endl;
    return 0;
})";
        std::ofstream file("tutorial/main.cpp", std::ios::binary);
        file << cppCode;
        file.close();
    }
    
    DELAYED_OUTPUT("A simple hello world file is created (\"tutorial/main.cpp\").");
    DELAYED_OUTPUT("Now, let's run it.");
    if(!RunCommandWithPrompt(runcpp2ExecutablePath + " tutorial/main.cpp"))
        return false;

    DELAYED_OUTPUT("Seems simple enough.\n");
    
    DELAYED_OUTPUT("Let's build the script instead of running it.");
    DELAYED_OUTPUT("Unless specified, input file is treated as `Executable`\n");
    
    DELAYED_OUTPUT( "We can get the binary files of the script by "
                    "using the `--build` (or `-b`) option.");
    DELAYED_OUTPUT( "And we can specify the output directory for the binary files with `--output` "
                    "(or `-o`)");
    DELAYED_OUTPUT( "We also need to pass the `--executable` (or `-e`) option to "
                    "explicitly get an executable file. \n"
                    "See https://neko-box-coder.github.io/runcpp2/latest/guides/"
                    "building_project_sources/ for more details.\n");
    
    //TODO: Remove this when the cache bug is fixed
    DELAYED_OUTPUT("`--rebuild` is just resetting the build cache");

    DELAYED_OUTPUT("Let's try it");
    if(!RunCommandWithPrompt(   runcpp2ExecutablePath + 
                                " --build --output ./tutorial --executable --rebuild "
                                "tutorial/main.cpp"))
    {
        return false;
    }

    DELAYED_OUTPUT("Let's see what we have in the \"tutorial\" directory.");
    #ifdef _WIN32
        if(!RunCommandWithPrompt("dir tutorial"))
            return false;
    #else
        if(!RunCommandWithPrompt("ls -lah tutorial"))
            return false;
    #endif
    
    #ifdef _WIN32
        DELAYED_OUTPUT("You should be able to see `main.exe` in the directory.");
    #else
        DELAYED_OUTPUT("You should be able to see `main` in the directory.");
    #endif

    DELAYED_OUTPUT("Let's run the executable file to make sure it works.");
    if(!RunCommandWithPrompt("./tutorial/main"))
        return false;
    
    DELAYED_OUTPUT( "Sometimes you want to remove the cache of the script before running it again.\n"
                    "You can do that by using the `--rebuild` (or `-rb`) option.");
    if(!RunCommandWithPrompt(runcpp2ExecutablePath + " --rebuild ./tutorial/main.cpp"))
        return false;

    DELAYED_OUTPUT("You can also see the build/run process by changing the log level to `info`.");
    DELAYED_OUTPUT("Let's rebuild the cache again and see the build process.");
    if(!RunCommandWithPrompt(   runcpp2ExecutablePath + 
                                " --rebuild --log-level info ./tutorial/main.cpp"))
    {
        return false;
    }

    DELAYED_OUTPUT( "runcpp2 also can give realtime error feedback by using "
                    "the `--watch` (or `-w`) option.");
    DELAYED_OUTPUT("I would love to show it here but it will block this tutorial.");
    DELAYED_OUTPUT("Feel free to try it yourself. "
                   "Make a change, or an error, then run `runcpp2 --watch tutorial/main.cpp`");
    DELAYED_OUTPUT("Once you are done, you can revert the changes and continue the tutorial.");
    DELAYED_OUTPUT("Press enter to continue...");
    GetInput(true);

    DELAYED_OUTPUT( "You can find the path to your config file and all the builds "
                    "with the `--show-config-path` (or `-sc`) option.");
    if(!RunCommandWithPrompt(runcpp2ExecutablePath + " --show-config-path"))
        return false;
    DELAYED_OUTPUT("Feel free to make any changes to the config file.\n");

    DELAYED_OUTPUT( "Finally, you can specify your script to be built in the current directory "
                    "with the `--local` (or `-l`) option. \n"
                    "This will create and build in the `.runcpp2` directory");
    if(!RunCommandWithPrompt("cd tutorial && " + runcpp2ExecutablePath + " --local main.cpp"))
        return false;

    #ifdef _WIN32
        if(!RunCommandWithPrompt("dir tutorial"))
            return false;
    #else
        if(!RunCommandWithPrompt("ls -lah tutorial"))
            return false;
    #endif    

    DELAYED_OUTPUT("You can find the details of the rest of the options by running `runcpp2 --help`");
    DELAYED_OUTPUT( "Or you can read it at https://neko-box-coder.github.io/runcpp2/latest/"
                    "program_manual/\n");
    DELAYED_OUTPUT("This concludes the 1st chapter of the tutorial.");
    DELAYED_OUTPUT("Press enter to continue...");
    GetInput(true);

    return true;
}

bool Chapter2_BuildConfig()
{
    DELAYED_OUTPUT( "===========================================\n"
                    "Chapter 2/3: Build Config\n"
                    "===========================================\n");

    std::error_code ec;
    if(!ghc::filesystem::exists("tutorial", ec))
    {
        ssLOG_ERROR("Missing tutorial directory");
        return false;
    }

    if(!ghc::filesystem::exists("tutorial/main.cpp", ec))
    {
        ssLOG_ERROR("Missing tutorial/main.cpp");
        return false;
    }

    DELAYED_OUTPUT( "You can specify your build config either as an inline YAML comment or as a "
                    "separate YAML file.");
    
    DELAYED_OUTPUT( "To set build config with an inline comment, you need to first start your "
                    "comment with either `/*runcpp2` or `//runcpp2`");
    DELAYED_OUTPUT("Space between `//` / `/*` and `runcpp2` is allowed as well.");
    DELAYED_OUTPUT("Press enter to continue...");
    GetInput(true);

    DELAYED_OUTPUT("First, let me add a define (WORLD=42) as build config to tutorial/main.cpp");
    DELAYED_OUTPUT("Press enter to continue...");
    GetInput(true);
    
    {
        std::string fileContent = ReadFile("tutorial/main.cpp");
        if(fileContent.empty())
        {
            ssLOG_ERROR("Failed to read tutorial/main.cpp");
            return false;
        }

        const std::string definesString = R"(/*runcpp2
Defines:
    DefaultPlatform:
        DefaultProfile: ["WORLD=42"]
*/)";
        
        if(!WriteFile("tutorial/main.cpp", definesString + "\n" + fileContent))
        {
            ssLOG_ERROR("Failed to write tutorial/main.cpp");
            return false;
        }

        DELAYED_OUTPUT("I have just added the define to the script as an inline YAML comment.");
        DELAYED_OUTPUT("Press enter to continue...");
        GetInput(true);
        
        DELAYED_OUTPUT( "Now let me add `std::cout << WORLD << std::endl;` to output the value "
                        "of WORLD.");
        DELAYED_OUTPUT("Press enter to continue...");
        GetInput(true);

        size_t returnPos = fileContent.find("return 0;");
        if(returnPos == std::string::npos)
        {
            ssLOG_ERROR("Failed to find return 0; in tutorial/main.cpp");
            return false;
        }

        fileContent.replace(returnPos, 
                            std::string("return 0;").size(),
                            "std::cout << WORLD << std::endl;\n    return 0;");
        if(!WriteFile("tutorial/main.cpp", definesString + "\n" + fileContent))
        {
            ssLOG_ERROR("Failed to write tutorial/main.cpp");
            return false;
        }
    }

    DELAYED_OUTPUT("I have just added the output line");
    DELAYED_OUTPUT("Now let's run the script");
    if(!RunCommandWithPrompt("cd tutorial && " + runcpp2ExecutablePath + " main.cpp"))
        return false;

    DELAYED_OUTPUT( "You should see the output `42`. \n"
                    "You can specify different settings for different platforms and profiles.");
    DELAYED_OUTPUT( "A platform is basically the Host OS (**NOT** the target OS), "
                    "so the OS you are running this script on.");
    DELAYED_OUTPUT("A profile is a single configuration of compiler/linker toolchain.");
    DELAYED_OUTPUT("Press enter to continue...");
    GetInput(true);
    
    DELAYED_OUTPUT( "`DefaultPlatform` is the default host platform if no explicit platform "
                    "is specified.");
    DELAYED_OUTPUT("`DefaultProfile` is the default profile if no explicit profile is specified.");
    DELAYED_OUTPUT( "So the define I just added will apply to any platform and profile, "
                    "unless there's a explicit platform/profile specified.");
    DELAYED_OUTPUT( "You can have settings that apply to specific platforms (e.g. Linux) and "
                    "profiles (e.g. msvc).");
    DELAYED_OUTPUT("Press enter to continue...");
    GetInput(true);
    
    DELAYED_OUTPUT( "If you remember what I said earlier, you can also specify the build config "
                    "as a separate YAML file.");
    DELAYED_OUTPUT( "I will continue the rest of the chapter with that since it is quite "
                    "cumbersome to edit tutorial/main.cpp everytime.");
    DELAYED_OUTPUT("Let me first remove the inline comment in tutorial/main.cpp.");
    DELAYED_OUTPUT("Press enter to continue...");
    GetInput(true);
    
    {
        std::string fileContent = ReadFile("tutorial/main.cpp");
        if(fileContent.empty())
        {
            ssLOG_ERROR("Failed to read tutorial/main.cpp");
            return false;
        }
        
        size_t startPos = fileContent.find("/*runcpp2");
        size_t endPos = fileContent.find("*/", startPos);
        
        if(startPos != std::string::npos && endPos != std::string::npos)
        {
            fileContent.erase(startPos, (endPos + 2 + 1 /* newline */) - startPos);
        }
        
        if(!WriteFile("tutorial/main.cpp", fileContent))
        {
            ssLOG_ERROR("Failed to write tutorial/main.cpp");
            return false;
        }
    }

    DELAYED_OUTPUT("I have just removed the inline comment in tutorial/main.cpp");
    DELAYED_OUTPUT("Press enter to continue...");
    GetInput(true);

    DELAYED_OUTPUT( "To have a dedicated build config file, you just need to create a YAML file "
                    "with the same name as your script file but with a `.yaml` extension.");
    DELAYED_OUTPUT("Let me create tutorial/main.yaml");
    DELAYED_OUTPUT("Press enter to continue...");
    GetInput(true);

    {
        const std::string yamlContent = R"(Defines:
    DefaultPlatform:
        DefaultProfile: ["WORLD=42"])";
        
        if(!WriteFile("tutorial/main.yaml", yamlContent))
        {
            ssLOG_ERROR("Failed to write tutorial/main.yaml");
            return false;
        }
    }

    DELAYED_OUTPUT("Now let me run the script again, nothing should have changed.");
    if(!RunCommandWithPrompt("cd tutorial && " + runcpp2ExecutablePath + " main.cpp"))
        return false;

    DELAYED_OUTPUT("");
    DELAYED_OUTPUT("A common build config is to specify other source files.");
    DELAYED_OUTPUT("This can be done by adding the paths to the `OtherFilesToBeCompiled` field.");
    DELAYED_OUTPUT("Let me add a second C++ file (\"tutorial/Hello.cpp\").");
    DELAYED_OUTPUT("Press enter to continue...");
    GetInput(true);
    
    {
        std::string fileContent = R"(#include "Hello.hpp"
#include <iostream>

void CallHello()
{
    std::cout << "Hello from another C++ file" << std::endl;
}
)";
        WriteFile("tutorial/Hello.cpp", fileContent);
    }

    DELAYED_OUTPUT("Let me also add a header file for the second C++ file (\"tutorial/Hello.hpp\").");
    DELAYED_OUTPUT("Press enter to continue...");
    GetInput(true);

    {
        std::string fileContent = R"(#ifndef HELLO_HPP
#define HELLO_HPP

void CallHello();

#endif // HELLO_HPP)";
        WriteFile("tutorial/Hello.hpp", fileContent);
    }

    DELAYED_OUTPUT("Turns out I still need to edit tutorial/main.cpp to call the function.");
    DELAYED_OUTPUT("Let me do that now.");
    DELAYED_OUTPUT("Press enter to continue...");
    GetInput(true);
    
    {
        std::string fileContent = ReadFile("tutorial/main.cpp");
        if(fileContent.empty())
        {
            ssLOG_ERROR("Failed to read tutorial/main.cpp");
            return false;
        }

        size_t returnPos = fileContent.find("return 0;");
        if(returnPos == std::string::npos)
        {
            ssLOG_ERROR("Failed to find return 0; in tutorial/main.cpp");
            return false;
        }
        fileContent.replace(returnPos, 
                            std::string("return 0;").size(),
                            "CallHello();\n    return 0;");

        WriteFile("tutorial/main.cpp", std::string("#include \"Hello.hpp\"\n") + fileContent);
    }

    DELAYED_OUTPUT("Now let me add the second C++ file to `OtherFilesToBeCompiled` in the YAML file.");
    DELAYED_OUTPUT("Press enter to continue...");
    GetInput(true);
    
    {
        const std::string yamlContent = R"(Defines:
    DefaultPlatform:
        DefaultProfile: ["WORLD=42"]

OtherFilesToBeCompiled:
    DefaultPlatform:
        DefaultProfile: ["./Hello.cpp"]
)";
        
        WriteFile("tutorial/main.yaml", yamlContent);
    }

    DELAYED_OUTPUT("The path to the other file is always **relative to the script file**.");
    DELAYED_OUTPUT("You can also add include paths for the script to the `IncludePaths` field");
    DELAYED_OUTPUT("Although unnecessary in this case, let me add \"./\" to the include paths.");
    DELAYED_OUTPUT("Press enter to continue...");
    GetInput(true);
    
    {
        const std::string yamlContent = R"(Defines:
    DefaultPlatform:
        DefaultProfile: ["WORLD=42"]

OtherFilesToBeCompiled:
    DefaultPlatform:
        DefaultProfile: ["./Hello.cpp"]

IncludePaths: 
    DefaultPlatform:
        DefaultProfile: ["./"]
)";
        
        WriteFile("tutorial/main.yaml", yamlContent);
    }

    DELAYED_OUTPUT("Let's run the script again.");
    if(!RunCommandWithPrompt("cd tutorial && " + runcpp2ExecutablePath + " main.cpp"))
        return false;
    
    DELAYED_OUTPUT("You should see the output `Hello from another C++ file`.");
    DELAYED_OUTPUT("");
    
    DELAYED_OUTPUT( "If you **only** have a setting that applies to all platforms and profiles like "
                    "what we have,");
    DELAYED_OUTPUT("you can specify the settings directly.");
    DELAYED_OUTPUT("Let me update the YAML file and show you what I mean.");
    DELAYED_OUTPUT("Press enter to continue...");
    GetInput(true);

    {
        const std::string yamlContent = R"(Defines: ["WORLD=42"]
OtherFilesToBeCompiled: ["./Hello.cpp"]
IncludePaths: ["./"]
)";
        WriteFile("tutorial/main.yaml", yamlContent);
    }
    
    DELAYED_OUTPUT("I have just updated the YAML file to omit `DefaultPlatform` and `DefaultProfile`.");
    DELAYED_OUTPUT("It's much more concise now. Let's run the script again.");
    if(!RunCommandWithPrompt("cd tutorial && " + runcpp2ExecutablePath + " main.cpp"))
        return false;
    
    DELAYED_OUTPUT("You should see the same output as before.");
    DELAYED_OUTPUT("");
    
    DELAYED_OUTPUT( "Finally, let me show you how to specify different settings for "
                    "different platforms and profiles.");
    DELAYED_OUTPUT("Let's update the compile config to have it generate a preprocessed output.");
    DELAYED_OUTPUT("Press enter to continue...");
    GetInput(true);
    
    {
        const std::string yamlContent = R"(Defines: ["WORLD=42"]
OtherFilesToBeCompiled: ["./Hello.cpp"]
IncludePaths: ["./"]

OverrideCompileFlags:
    DefaultPlatform:
        "g++": 
            Append: "-E"
    Windows:
        "msvc": 
            Append: "/P"
)";
        WriteFile("tutorial/main.yaml", yamlContent);
    }

    DELAYED_OUTPUT( "Since output preprocessed file flag is dependent on the compiler, "
                    "I have specified it per platform and profile.");
    DELAYED_OUTPUT( "You can see \"g++\" is under `DefaultPlatform`, meaning the flags will be "
                    "applied to all platforms. ");
    DELAYED_OUTPUT( "So if you specify your profile to be \"g++\" and you are on Windows, "
                    "it will use the \"-E\" flag.");
    DELAYED_OUTPUT("If you are using msvc and you are on Windows, it will use the \"/P\" flag.");
    DELAYED_OUTPUT("Let's run the script now.");
    RunCommandWithPrompt("cd tutorial && " + runcpp2ExecutablePath + " main.cpp");
    
    DELAYED_OUTPUT("");
    DELAYED_OUTPUT( "You probably see some errors for linking, this is expected since the "
                    "preprocessed flag doesn't generate any object files.");
    DELAYED_OUTPUT("This is just a demonstration on specifying platform/profile specific settings.");
    DELAYED_OUTPUT("But the default profile and settings should be good enough for the most part.");
    DELAYED_OUTPUT("Let me remove the flags I have just added.");
    DELAYED_OUTPUT("Press enter to continue...");
    GetInput(true);

    {
        const std::string yamlContent = R"(Defines: ["WORLD=42"]
OtherFilesToBeCompiled: ["./Hello.cpp"]
IncludePaths: ["./"]
)";
        WriteFile("tutorial/main.yaml", yamlContent);
    }

    DELAYED_OUTPUT("Finally, you can generate a build config template which documents all the fields.");
    DELAYED_OUTPUT("You can do that by using the `--create-script-template`/ `-t` option.");
    DELAYED_OUTPUT("This will either create a new file or prepend the template to the existing file.");
    
    if(!RunCommandWithPrompt(runcpp2ExecutablePath + " -t tutorial/template.yaml"))
        return false;
    
    DELAYED_OUTPUT("You can see the template in tutorial/template.yaml");
    DELAYED_OUTPUT("");

    DELAYED_OUTPUT("This concludes the 2nd chapter of the tutorial.");
    DELAYED_OUTPUT("Press enter to continue...");
    GetInput(true);
    
    return true;
}

bool Chapter3_ExternalDependencies()
{
    DELAYED_OUTPUT( "===========================================\n"
                    "Chapter 3/3: External Dependencies\n"
                    "===========================================\n");

    std::error_code ec;
    if(!ghc::filesystem::exists("tutorial", ec))
    {
        ssLOG_ERROR("Missing tutorial directory");
        return false;
    }
    
    DELAYED_OUTPUT( "It's good to be able to run a C++ file by itself, "
                    "but there's a limit to what you can do without any external dependencies.");
    DELAYED_OUTPUT("This can be done by adding your dependencies to the `Dependencies` field.");
    DELAYED_OUTPUT("The most common and simple one is a header library.");
    DELAYED_OUTPUT("Let me query and download an example for you.");

    #ifdef _WIN32
        if(!RunCommandWithPrompt(   "powershell -Command \""
                                    "Invoke-WebRequest https://github.com/Neko-Box-Coder/runcpp2/raw/"
                                    "refs/heads/master/Examples/Logging.cpp "
                                    "-OutFile tutorial/Logging.cpp\"", true, false))
        {
            return false;
        }    
    #else
        if(!RunCommandWithPrompt(   "curl -L -o tutorial/Logging.cpp "
                                    "https://github.com/Neko-Box-Coder/runcpp2/raw/refs/heads/"
                                    "master/Examples/Logging.cpp"))
        {
            return false;
        }
    #endif

    DELAYED_OUTPUT("I have downloaded the example to tutorial/Logging.cpp");
    DELAYED_OUTPUT("Press enter to continue...");
    GetInput(true);
    
    DELAYED_OUTPUT("A dependency must contain a \"Source\" to determine where it is.");
    DELAYED_OUTPUT("In this case, it is coming from a \"Git\" repository.");
    DELAYED_OUTPUT( "If you have a git submodule, you can point it to a local directory with "
                    "\"Local\" instead of \"Git\"");
    DELAYED_OUTPUT("Press enter to continue...");
    GetInput(true);

    DELAYED_OUTPUT( "Next, you have \"LibraryType\" which tells runcpp2 what type of dependency "
                    "this is. Other options include \"Static\", \"Object\" and \"Shared\"");
    DELAYED_OUTPUT("That's pretty much it for a header only library. Let's run it shell we?");
    if(!RunCommandWithPrompt("cd tutorial && " + runcpp2ExecutablePath + " Logging.cpp"))
        return false;
    
    DELAYED_OUTPUT("Let's try an external shared library");
    DELAYED_OUTPUT("Let me query and download an example for you.");
    #ifdef _WIN32
        if(!RunCommandWithPrompt(   "powershell -Command \""
                                    "Invoke-WebRequest https://github.com/Neko-Box-Coder/runcpp2/raw/"
                                    "refs/heads/master/Examples/SDLWindow.cpp "
                                    "-OutFile tutorial/SDLWindow.cpp\"", true, false))
        {
            return false;
        }
            
    #else
        if(!RunCommandWithPrompt(   "curl -L -o tutorial/SDLWindow.cpp "
                                    "https://github.com/Neko-Box-Coder/runcpp2/raw/refs/heads/"
                                    "master/Examples/SDLWindow.cpp"))
        {
            return false;
        }
    #endif
    DELAYED_OUTPUT("I have downloaded the example to tutorial/SDLWindow.cpp");
    DELAYED_OUTPUT("Press enter to continue...");
    GetInput(true);

    DELAYED_OUTPUT("Non header dependency has a few more fields. First we have \"LinkProperties\".");
    DELAYED_OUTPUT("This can contain 4 inner fields.");
    DELAYED_OUTPUT( "First is \"SearchLibraryNames\". "
                    "Any binary files that is linkable and contains the search term in the filename "
                    "will be linked against.");
    DELAYED_OUTPUT("For example \"Library\" will match against \"MyLibrary.dll\"");
    DELAYED_OUTPUT("Press enter to continue...");
    GetInput(true);
    
    DELAYED_OUTPUT( "Next, we have \"SearchDirectories\". This tells runcpp2 where to search for "
                    "the binaries.");
    DELAYED_OUTPUT( "This is **NOT** recursive search. You need to supply the directories that "
                    "may contain the binaries we are linking against.");
    DELAYED_OUTPUT("Press enter to continue...");
    GetInput(true);
    
    DELAYED_OUTPUT("Finally, we have \"ExcludeLibraryNames\" and \"AdditionalLinkOptions\".");
    DELAYED_OUTPUT("\"ExcludeLibraryNames\" will not link the binaries that match the filename.");
    DELAYED_OUTPUT( "\"AdditionalLinkOptions\" is a list of link flags needed when linking against "
                    "the dependency binaries we found.");
    DELAYED_OUTPUT("Press enter to continue...");
    GetInput(true);
    
    DELAYED_OUTPUT( "The example I have just downloaded will build SDL2 from source, "
                    "then link against the built binary and display a Window.");
    DELAYED_OUTPUT( "It might take a bit of time to build SDL2 but only happens when you run "
                    "the script for the first time. Let's run it now.");
    if(!integrationTest)
    {
        if(!RunCommandWithPrompt("cd tutorial && " + runcpp2ExecutablePath + " SDLWindow.cpp"))
            return false;
    }
    else
    {
        DELAYED_OUTPUT("running SDLWindow.cpp skipped for integration testing");
        if(!RunCommandWithPrompt("cd tutorial && " + runcpp2ExecutablePath + " --build SDLWindow.cpp"))
            return false;
    }

    DELAYED_OUTPUT("You can also have a dependency as a standalone YAML file which you can import.");
    DELAYED_OUTPUT("Let me move the SDL2 dependency to a standalone YAML file.");
    DELAYED_OUTPUT("Press enter to continue...");
    GetInput(true);

    //Try to extract SDL dependency and write it to standalone YAML
    {
        std::string fileContent = ReadFile("tutorial/SDLWindow.cpp");
        if(fileContent.empty())
        {
            ssLOG_ERROR("Failed to read tutorial/SDLWindow.cpp");
            return false;
        }

        size_t commentStart = fileContent.find("/*");
        size_t commentEnd = fileContent.find("*/");

        if(commentStart == std::string::npos || commentEnd == std::string::npos)
        {
            ssLOG_ERROR("Failed to find comment block in tutorial/SDLWindow.cpp");
            return false;
        }

        //Store the comment block (+2 to include the */)
        std::string commentBlock = fileContent.substr(commentStart, commentEnd - commentStart + 2);

        //Replace the comment block with PLACEHOLDER
        std::string newInlineConfig = R"(/* runcpp2

Dependencies:
-   Name: SDL2
    Source:
        ImportPath: "SDL2.yaml"
*/)";
        fileContent.replace(commentStart, commentEnd - commentStart + 2, newInlineConfig);

        //Find the SDL dependency block
        size_t sdlStart = commentBlock.find("Name: SDL2");
        if(sdlStart == std::string::npos)
        {
            ssLOG_ERROR("Failed to find Name: SDL2 in tutorial/SDLWindow.cpp");
            return false;
        }

        //Extract the SDL dependency block
        int expectedIndent = 0;
        std::string sdlContent;
        {
            for(int i = (int)sdlStart - 1; 
                i >= 0 && (fileContent[i] == ' ' || fileContent[i] == '-'); 
                --i)
            {
                expectedIndent++;
            }
            
            size_t sdlEnd = sdlStart;
            for(int i = (int)sdlStart + 1; i < (int)commentBlock.length(); ++i)
            {
                if(commentBlock[i] == '\n') 
                {
                    size_t lineStart = i + 1;
                    if(lineStart >= commentBlock.length())
                    {
                        sdlEnd = i;
                        break;
                    }
                    
                    //Count spaces at start of line
                    int spaces = 0;
                    while(lineStart + spaces < commentBlock.length())
                    {
                        if(commentBlock[lineStart + spaces] == ' ')
                            spaces++;
                        //Short empty line, remove it
                        else if(commentBlock[lineStart + spaces] == '\n' && spaces < expectedIndent)
                            commentBlock.erase(lineStart, spaces + 1);
                        else if(commentBlock[lineStart + spaces] == '\r')
                            continue;
                        else
                            break;
                    }

                    //If line has fewer spaces than expected indent, we've found the end
                    if(spaces < expectedIndent) 
                    {
                        sdlEnd = i;
                        break;
                    }
                    
                    //Update where we last saw valid sdl content
                    sdlEnd = i;
                }
            }

            sdlContent = commentBlock.substr(sdlStart, sdlEnd - sdlStart);
        }

        //Trim each line of the SDL dependency block
        {
            size_t firstNewline = sdlContent.find('\n');
            if(firstNewline == std::string::npos)
            {
                ssLOG_ERROR("Failed to find first newline in tutorial/SDLWindow.cpp");
                return false;
            }

            std::string indentSpaces(expectedIndent, ' ');
            indentSpaces = "\n" + indentSpaces;
            
            size_t currentIndentPos = sdlContent.find(indentSpaces, firstNewline);
            while(currentIndentPos != std::string::npos)
            {
                sdlContent.replace(currentIndentPos, indentSpaces.length(), "\n");
                currentIndentPos = sdlContent.find(indentSpaces, ++currentIndentPos);
            }
        }
        
        //Write the SDL dependency block to a standalone YAML file
        if(!WriteFile("tutorial/SDL2.yaml", sdlContent))
        {
            ssLOG_ERROR("Failed to write tutorial/SDL2.yaml");
            return false;
        }
        
        DELAYED_OUTPUT("I have extracted the SDL2 dependency block to a standalone YAML file.");
        DELAYED_OUTPUT("Press enter to continue...");
        GetInput(true);

        //Write the modified file content to a new file
        if(!WriteFile("tutorial/SDLWindow.cpp", fileContent))
        {
            ssLOG_ERROR("Failed to write tutorial/SDLWindow.cpp");
            return false;
        }
    }

    DELAYED_OUTPUT("I have modified tutorial/SDLWindow.cpp to import the SDL2 dependency YAML file.");
    DELAYED_OUTPUT("As you can see, we are now importing the SDL2 dependency YAML file instead");
    DELAYED_OUTPUT("by using the `ImportPath` field.");
    DELAYED_OUTPUT( "The import is not limited to local files, you can also import from a git "
                    "repository. For more details, please refer to the documentation.");
    DELAYED_OUTPUT("Press enter to continue...");
    GetInput(true);

    DELAYED_OUTPUT("Let's run it now.");
    if(!integrationTest)
    {
        if(!RunCommandWithPrompt("cd tutorial && " + runcpp2ExecutablePath + " SDLWindow.cpp"))
            return false;
    }
    else
    {
        DELAYED_OUTPUT("running SDLWindow.cpp skipped for integration testing");
        if(!RunCommandWithPrompt("cd tutorial && " + runcpp2ExecutablePath + " --build SDLWindow.cpp"))
            return false;
    }
    
    DELAYED_OUTPUT("This concludes the 3rd chapter of the tutorial.");
    DELAYED_OUTPUT("Press enter to continue...");
    GetInput(true);

    return true;
}

int main(int argc, char* argv[])
{
    ssLOG_SET_CURRENT_THREAD_TARGET_LEVEL(ssLOG_LEVEL_WARNING);
    
    //--test <path to runcpp2> <path to config file>
    integrationTest = argc > 2 && std::string(argv[1]) == "--test";
    
    DELAYED_OUTPUT( "===========================================\n"
                    "Runcpp2 Interactive Tutorial\n"
                    "===========================================\n");

    if(!integrationTest)
    {
        if(!InitializeRuncpp2ExecutablePath())
            return 1;
    }
    else
    {
        if(argc != 4)
        {
            ssLOG_ERROR("Missing arguments for integrationTest");
            return 1;
        }
        
        std::error_code ec;
        auto exePath = ghc::filesystem::absolute(ghc::filesystem::path(argv[2]), ec);
        if(ec)
        {
            ssLOG_ERROR("Failed to get absolute path for " << argv[2] << ". \n" << ec.message());
            return 1;
        }
        
        auto configPath = ghc::filesystem::absolute(ghc::filesystem::path(argv[3]), ec);
        if(ec)
        {
            ssLOG_ERROR("Failed to get absolute path for " << argv[3] << ". \n" << ec.message());
            return 1;
        }
        
        runcpp2ExecutablePath = exePath.string() + " -l -c " + configPath.string();
    }

    DELAYED_OUTPUT("The whole tutorial is about 15 minutes long.");

    DELAYED_OUTPUT("Do you want to start from the beginning? [Y/n]:");
    bool ans = GetYN_WithDefault(true);
    
    if(!ans)
    {
        while(true)
        {
            DELAYED_OUTPUT("Which chapter do you want to start from? [1-3]");
            DELAYED_OUTPUT("1. Using runcpp2 executable");
            DELAYED_OUTPUT("2. Build Config");
            DELAYED_OUTPUT("3. External Dependencies");

            std::string input = GetInput();
            if(input == "1")
                goto chapter1;
            else if(input == "2")
                goto chapter2;
            else if(input == "3")
                goto chapter3;
            else
                DELAYED_OUTPUT("Invalid answer, try again.");
        }
    }
    
    #define HANDLE_FAILURE(statement) \
        if(!(statement)) \
        { \
            DELAYED_OUTPUT( "Seems like something went wrong. Please report the issue on " \
                            "https://github.com/Neko-Box-Coder/runcpp2/issues"); \
            return 1; \
        }
    
    chapter1:;
    HANDLE_FAILURE(Chapter1_Runcpp2Executable());

    chapter2:;
    HANDLE_FAILURE(Chapter2_BuildConfig());

    chapter3:;
    HANDLE_FAILURE(Chapter3_ExternalDependencies());


    DELAYED_OUTPUT("This concludes the tutorial.");
    DELAYED_OUTPUT("Feel free to check out the documentation for more details.");
    DELAYED_OUTPUT("https://neko-box-coder.github.io/runcpp2/latest/guides/basic_concepts/");
    DELAYED_OUTPUT("");
    DELAYED_OUTPUT("As well as the examples in the repository.");
    DELAYED_OUTPUT("https://github.com/Neko-Box-Coder/runcpp2/tree/master/Examples");
    DELAYED_OUTPUT("Press enter to continue...");
    GetInput(true);

    return 0;
}
