#include "runcpp2/Data/Profile.hpp"
#include "runcpp2/runcpp2.hpp"
#include "runcpp2/LibYAML_Wrapper.hpp"
#include "runcpp2/DeferUtil.hpp"
#include "ssLogger/ssLog.hpp"

DS::Result<void> TestMain()
{
    std::unordered_map<std::string, std::string> tempParameters;
    
    //Valid Profile Should Be Parsed Correctly
    {
        //NOTE: This is just a test YAML for validating parsing, don't use it for actual config
        const char* yamlStr = R"(
            Name: "g++"
            NameAliases: ["mingw"]
            FileExtensions: [.cpp, .cc, .cxx]
            Languages: ["c++"]
            Parameters:
                Param1:
                    Optional: true
                    Default: "All"
                    Array: false
                    Constraint: ["A", "B", "C", "All:A,B,C"]
                Param2:
                    Optional: true
                    Default: "123"
                    Array: false
                    Constraint: "Int"
            Variables:
                VarName1: "Some string {Param1} substitution"
            Setup:
                DefaultPlatform: ["setup command 1", "setup command 2"]
            Cleanup:
                DefaultPlatform: []
            FilesTypes:
                ObjectLinkFile:
                    Prefix:
                        DefaultPlatform: ""
                    Extension:
                        Windows: ".obj"
                        Unix: ".o"
                SharedLinkFile:
                    Prefix:
                        Windows: ""
                        Linux: "lib"
                        MacOS: ""
                    Extension:
                        Windows: ".lib"
                        Linux: ".so"
                        MacOS: ".dylib"
                SharedLibraryFile:
                    Prefix:
                        Windows: ""
                        Linux: "lib"
                        MacOS: ""
                    Extension:
                        Windows: ".dll"
                        Linux: ".so"
                        MacOS: ".dylib"
                StaticLinkFile:
                    Prefix:
                        Unix: "lib"
                        Windows: ""
                    Extension:
                        Windows: ".lib"
                        Unix: ".a"
                ExecutableFile:
                    Prefix:
                        Unix: ""
                        Windows: ""
                    Extension:
                        Windows: ".exe"
                        Unix: ""
                DebugSymbolFile:
                    Prefix:
                        Windows: ""
                        Unix: ""
                    Extension:
                        Windows: .pdb
                        Unix: .debug
            Compiler:
                PreRun:
                    DefaultPlatform: ""
                CheckExistence:
                    DefaultPlatform: "g++ -v"
                CompileTypes:
                    Executable:
                        DefaultPlatform:
                            Flags: "-std=c++17 -Wall -g"
                            Executable: "g++"
                            RunParts:
                            -   Type: Once
                                CommandPart: "{Stage.Executable} -c {Stage.CompileFlags}"
                            -   Type: Repeats
                                CommandPart: " -I\"{Stage.IncludeDirectory.Path}\""
                                Separator: " "
                            -   Type: Once
                                CommandPart: " \"{Stage.Input.Path}\" -o \"{Stage.Output.Directory}{/}\
                                    {Stage.ObjectLinkFile.Prefix}{Stage.Input.Name}\
                                    {Stage.ObjectLinkFile.Extension}\""
                            ExpectedOutputFiles: ["TestOutputFile", "TestOutputFile2"]
                    ExecutableShared:
                        DefaultPlatform:
                            Flags: "-std=c++17 -Wall -g -fpic"
                            Executable: "g++"
                            RunParts:
                            -   Type: Once
                                CommandPart: "{Stage.Executable} -c {Stage.CompileFlags}"
                            -   Type: Repeats
                                CommandPart: " -I\"{Stage.IncludeDirectory.Path}\""
                            -   Type: Once
                                CommandPart: " \"{Stage.Input.Path}\" -o \"{Stage.Output.Directory}{/}\
                                    {Stage.ObjectLinkFile.Prefix}{Stage.Input.Name}\
                                    {Stage.ObjectLinkFile.Extension}\""
                            ExpectedOutputFiles: ["TestOutputFile", "TestOutputFile2"]
                    Static:
                        DefaultPlatform:
                            Flags: "-std=c++17 -Wall -g"
                            Executable: "g++"
                            RunParts:
                            -   Type: Once
                                CommandPart: "{Stage.Executable} -c {Stage.CompileFlags}"
                            ExpectedOutputFiles: ["TestOutputFile", "TestOutputFile2"]
                    Shared:
                        DefaultPlatform:
                            Flags: "-std=c++17 -Wall -g -fpic"
                            Executable: "g++"
                            RunParts:
                            -   Type: Once
                                CommandPart: "{Stage.Executable} -c {Stage.CompileFlags}"
                            ExpectedOutputFiles: ["TestOutputFile", "TestOutputFile2"]
            Linker:
                PreRun:
                    DefaultPlatform: ""
                CheckExistence:
                    DefaultPlatform: "g++ -v"
                LinkTypes:
                    Executable:
                        Unix:
                            Flags: "-Wl,-rpath,\\$ORIGIN"
                            Executable: "g++"
                            RunParts:
                            -   Type: Once
                                CommandPart: "{Stage.Executable} {Stage.LinkFlags} -o \
                                    \"{Stage.Output.Directory}{/}{Stage.Output.Name}\""
                            ExpectedOutputFiles: ["TestOutputFile", "TestOutputFile2"]
                        Windows:
                            Flags: "-Wl,-rpath,\\$ORIGIN"
                            Executable: "g++"
                            RunParts:
                            -   Type: Once
                                CommandPart: "{Stage.Executable} {Stage.LinkFlags} -o \
                                    \"{Stage.Output.Directory}{/}{Stage.Output.Name}.exe\""
                            ExpectedOutputFiles: ["TestOutputFile", "TestOutputFile2"]
                    Static:
                        DefaultPlatform:
                            Flags: ""
                            Executable: "g++"
                            RunParts:
                            -   Type: Once
                                CommandPart: "{Stage.Executable} {Stage.LinkFlags} -o \
                                    \"{Stage.Output.Directory}{/}{Stage.StaticLinkFile.Prefix}\
                                    {Stage.Output.Name}{Stage.StaticLinkFile.Extension}\""
                            ExpectedOutputFiles: ["TestOutputFile", "TestOutputFile2"]
                    Shared:
                        Unix:
                            Flags: "-shared -Wl,-rpath,\\$ORIGIN"
                            Executable: "g++"
                            RunParts:
                            -   Type: Once
                                CommandPart: "{Stage.Executable} {Stage.LinkFlags} -o \
                                    \"{Stage.Output.Directory}{/}{Stage.SharedLibraryFile.Prefix}\
                                    {Stage.Output.Name}{Stage.SharedLibraryFile.Extension}\""
                            ExpectedOutputFiles: ["TestOutputFile", "TestOutputFile2"]
                    ExecutableShared:
                        Unix:
                            Flags: "-shared -Wl,-rpath,\\$ORIGIN"
                            Executable: "g++"
                            RunParts:
                            -   Type: Once
                                CommandPart: "{Stage.Executable} {Stage.LinkFlags} -o \
                                \"{Stage.Output.Directory}{/}{Stage.SharedLibraryFile.Prefix}\
                                {Stage.Output.Name}{Stage.SharedLibraryFile.Extension}\""
                            ExpectedOutputFiles: ["TestOutputFile", "TestOutputFile2"]
        )";
        
        runcpp2::YAML::ResourceHandle resource;
        std::vector<runcpp2::YAML::NodePtr> roots = runcpp2::YAML::ParseYAML(   yamlStr, 
                                                                                resource).DS_TRY();
        DEFER { FreeYAMLResource(resource); };
        
        DS_ASSERT_EQ(roots.size(), 1);
        runcpp2::YAML::NodePtr root = roots.front();
        runcpp2::Data::Profile profile;
        
        profile.ParseYAML_Node(root, true, tempParameters).DS_TRY();
        
        //Verify basic fields
        DS_ASSERT_EQ(profile.Name, "g++");
        DS_ASSERT_EQ(profile.NameAliases.size(), 1);
        DS_ASSERT_EQ(profile.NameAliases.count("mingw"), 1);
        
        //Verify FileExtensions
        DS_ASSERT_EQ(profile.FileExtensions.size(), 3);
        DS_ASSERT_EQ(profile.FileExtensions.count(".cpp"), 1);
        DS_ASSERT_EQ(profile.FileExtensions.count(".cc"), 1);
        DS_ASSERT_EQ(profile.FileExtensions.count(".cxx"), 1);
        
        //Verify Languages
        DS_ASSERT_EQ(profile.Languages.size(), 1);
        DS_ASSERT_EQ(profile.Languages.count("c++"), 1);
        
        //Verify Parameters
        DS_ASSERT_EQ(profile.Parameters.count("Param1"), 1);
        {
            ParameterValue& paramVal = profile.Parameters["Param1"];
            DS_ASSERT_TRUE(paramVal.Optional);
            DS_ASSERT_EQ(paramVal.Default, "All");
            DS_ASSERT_FALSE(paramVal.Array);
            DS_ASSERT_TRUE( paramVal.CurrentConstraintType == 
                            runcpp2::Data::ParameterValue::ConstraintType::Choices);
            DS_ASSERT_TRUE(mpark::is<std::vector<std::string>>(paramVal.ConstraintValue));
            std::vector<std::string>& constraintVals = 
                mpark::get<std::vector<std::string>>(paramVal.ConstraintValue);
            DS_ASSERT_EQ(constraintVals.size(), 4);
            DS_ASSERT_EQ(constraintVals[2], "C");
        }
        
        DS_ASSERT_EQ(profile.Parameters.count("Param2"), 1);
        {
            ParameterValue& paramVal = profile.Parameters["Param2"];
            DS_ASSERT_TRUE(paramVal.Optional);
            DS_ASSERT_EQ(paramVal.Default, "123");
            DS_ASSERT_FALSE(paramVal.Array);
            DS_ASSERT_TRUE( paramVal.CurrentConstraintType == 
                            runcpp2::Data::ParameterValue::ConstraintType::Int);
            //DS_ASSERT_TRUE(mpark::is<std::string>(paramVal.ConstraintValue));
        }
        
        //Verify Variables
        DS_ASSERT_EQ(profile.Variables.count("VarName1"), 1);
        DS_ASSERT_EQ(profile.Variables["VarName1"], "Some string {Param1} substitution");
        
        //Verify Setup
        DS_ASSERT_EQ(profile.Setup.count("DefaultPlatform"), 1);
        const std::vector<std::string>& setupCommands = profile.Setup.at("DefaultPlatform");
        DS_ASSERT_EQ(setupCommands.size(), 2);
        DS_ASSERT_EQ(setupCommands.at(0), "setup command 1");
        DS_ASSERT_EQ(setupCommands.at(1), "setup command 2");
        
        //Verify FilesTypes
        const auto& objectLinkFile = profile.FilesTypes.ObjectLinkFile;
        DS_ASSERT_EQ(objectLinkFile.Extension.at("Windows"), ".obj");
        DS_ASSERT_EQ(objectLinkFile.Extension.at("Unix"), ".o");
        
        const auto& sharedLinkFile = profile.FilesTypes.SharedLinkFile;
        DS_ASSERT_EQ(sharedLinkFile.Prefix.at("Linux"), "lib");
        DS_ASSERT_EQ(sharedLinkFile.Extension.at("Linux"), ".so");
        
        const auto& debugSymbolFile = profile.FilesTypes.DebugSymbolFile;
        DS_ASSERT_EQ(debugSymbolFile.Extension.at("Windows"), ".pdb");
        DS_ASSERT_EQ(debugSymbolFile.Extension.at("Unix"), ".debug");
        
        //Verify Compiler
        DS_ASSERT_EQ(profile.Compiler.CheckExistence.at("DefaultPlatform"), "g++ -v");
        const auto& executableCompile = profile.Compiler.OutputTypes.Executable.at("DefaultPlatform");
        DS_ASSERT_EQ(executableCompile.Flags, "-std=c++17 -Wall -g");
        DS_ASSERT_EQ(executableCompile.Executable, "g++");
        DS_ASSERT_EQ(executableCompile.RunParts.size(), 3);
        DS_ASSERT_EQ(executableCompile.RunParts[1].Separator, " ");
        DS_ASSERT_EQ(executableCompile.ExpectedOutputFiles.size(), 2);
        
        //Verify Compiler ExecutableShared
        const auto& executableSharedCompile = 
            profile.Compiler.OutputTypes.ExecutableShared.at("DefaultPlatform");
        DS_ASSERT_EQ(executableSharedCompile.Flags, "-std=c++17 -Wall -g -fpic");
        DS_ASSERT_EQ(executableSharedCompile.Executable, "g++");
        DS_ASSERT_EQ(executableSharedCompile.RunParts.size(), 3);
        DS_ASSERT_EQ(executableSharedCompile.ExpectedOutputFiles.size(), 2);
        
        //Verify Linker
        DS_ASSERT_EQ(profile.Linker.CheckExistence.at("DefaultPlatform"), "g++ -v");
        const auto& executableLink = profile.Linker.OutputTypes.Executable.at("Unix");
        DS_ASSERT_EQ(executableLink.Flags, "-Wl,-rpath,\\$ORIGIN");
        DS_ASSERT_EQ(executableLink.Executable, "g++");
        DS_ASSERT_EQ(executableLink.RunParts.size(), 1);
        DS_ASSERT_EQ(executableLink.ExpectedOutputFiles.size(), 2);
        
        //Verify Linker ExecutableShared
        const auto& executableSharedLink = profile.Linker.OutputTypes.ExecutableShared.at("Unix");
        DS_ASSERT_EQ(executableSharedLink.Flags, "-shared -Wl,-rpath,\\$ORIGIN");
        DS_ASSERT_EQ(executableSharedLink.Executable, "g++");
        DS_ASSERT_EQ(executableSharedLink.RunParts.size(), 1);
        DS_ASSERT_EQ(executableSharedLink.ExpectedOutputFiles.size(), 2);
        
        //Test ToString() and Equals()
        std::string yamlOutput = profile.ToString("");
        roots = runcpp2::YAML::ParseYAML(yamlOutput, resource).DS_TRY();
        DS_ASSERT_EQ(roots.size(), 1);
        
        runcpp2::Data::Profile parsedOutput;
        parsedOutput.ParseYAML_Node(roots.front(), true, tempParameters).DS_TRY();
        DS_ASSERT_TRUE(profile.Equals(parsedOutput));
    }
    
    //Profile With Unsubstituted Params Should Fail Parsing
    {
        //NOTE: This is just a test YAML for validating parsing, don't use it for actual config
        const char* yamlStr = R"(
            Name: "g++"
            NameAliases: ["mingw"]
            FileExtensions: [.cpp, .cc, .cxx]
            Languages: ["c++"]
            Setup: 
                DefaultPlatform:
                    ["setup command 1", "setup command 2"]
            FilesTypes:
                ObjectLinkFile:
                    Prefix: 
                        DefaultPlatform: ""
                    Extension: 
                        DefaultPlatform: ".o"
                SharedLinkFile:
                    Prefix: 
                        DefaultPlatform: "lib"
                    Extension: 
                        DefaultPlatform: ".so"
                SharedLibraryFile:
                    Prefix: 
                        DefaultPlatform: "lib"
                    Extension: 
                        DefaultPlatform: ".so"
                StaticLinkFile:
                    Prefix: 
                        DefaultPlatform: "lib"
                    Extension: 
                        DefaultPlatform: ".a"
                ExecutableFile:
                    Prefix:
                        Unix: ""
                        Windows: ""
                    Extension:
                        Windows: ".exe"
                        Unix: ""
            Compiler:
                CheckExistence: 
                    DefaultPlatform: "g++ -v"
                CompileTypes:
                    Executable:
                        DefaultPlatform: 
                            Flags: "-std=c++17 -Wall -g"
                            Executable: "g++"
                            RunParts:
                            -   Type: Once
                                CommandPart: "{Stage.Executable} -c {Stage.CompileFlags}"
                            ExpectedOutputFiles: []
                    ExecutableShared:
                        DefaultPlatform: 
                            Flags: "-std=c++17 -Wall -g -fpic"
                            Executable: "g++"
                            RunParts:
                            -   Type: Once
                                CommandPart: "{Stage.Executable} -c {Stage.CompileFlags}"
                            ExpectedOutputFiles: []
                    Static:
                        DefaultPlatform: 
                            Flags: "-std=c++17 -Wall -g"
                            Executable: "g++"
                            RunParts:
                            -   Type: Once
                                CommandPart: "{Stage.Executable} -c {Stage.CompileFlags}"
                            ExpectedOutputFiles: []
                    Shared:
                        DefaultPlatform: 
                            Flags: "-std=c++17 -Wall -g -fpic"
                            Executable: "g++"
                            RunParts:
                            -   Type: Once
                                CommandPart: "{Stage.Executable} -c {Stage.CompileFlags}"
                            ExpectedOutputFiles: []
            Linker:
                CheckExistence: 
                    DefaultPlatform: 
                        "g++ -v"
                LinkTypes:
                    Executable:
                        DefaultPlatform: 
                            Flags: "-Wl,-rpath,\\$ORIGIN"
                            Executable: "g++"
                            RunParts:
                            -   Type: Once
                                CommandPart: "{Stage.Executable} {Stage.LinkFlags} -o \
                                    \"{Stage.Output.Directory}{/}{Stage.Output.Name}\""
                            ExpectedOutputFiles: []
                    Static:
                        DefaultPlatform: 
                            Flags: "{UnexpectedParameter}"
                            Executable: "g++"
                            RunParts:
                            -   Type: Once
                                CommandPart: "{Stage.Executable} {Stage.LinkFlags} -o \
                                    \"{Stage.Output.Directory}{/}{Stage.StaticLinkFile.Prefix}\
                                    {Stage.Output.Name}{Stage.StaticLinkFile.Extension}\""
                            ExpectedOutputFiles: []
                    Shared:
                        DefaultPlatform: 
                            Flags: "-shared -Wl,-rpath,\\$ORIGIN"
                            Executable: "g++"
                            RunParts:
                            -   Type: Once
                                CommandPart: "{Stage.Executable} {Stage.LinkFlags} -o \
                                    \"{Stage.Output.Directory}{/}{Stage.SharedLibraryFile.Prefix}\
                                    {Stage.Output.Name}{Stage.SharedLibraryFile.Extension}\""
                            ExpectedOutputFiles: []
                    ExecutableShared:
                        DefaultPlatform: 
                            Flags: "-shared -Wl,-rpath,\\$ORIGIN"
                            Executable: "g++"
                            RunParts:
                            -   Type: Once
                                CommandPart: "{Stage.Executable} {Stage.LinkFlags} -o \
                                \"{Stage.Output.Directory}{/}{Stage.SharedLibraryFile.Prefix}\
                                {Stage.Output.Name}{Stage.SharedLibraryFile.Extension}\""
                            ExpectedOutputFiles: []
        )";
    
        runcpp2::YAML::ResourceHandle resource;
        std::vector<runcpp2::YAML::NodePtr> roots = runcpp2::YAML::ParseYAML(   yamlStr, 
                                                                                resource).DS_TRY();
        DEFER { FreeYAMLResource(resource); };
        
        DS_ASSERT_EQ(roots.size(), 1);
        runcpp2::YAML::NodePtr root = roots.front();
        runcpp2::Data::Profile profile;
        
        DS::Result<void> res = profile.ParseYAML_Node(root, true, tempParameters);
        
        DS_ASSERT_FALSE(res.HasValue());
        DS_ASSERT_EQ(   res.Error().Message, 
                        "Parameter {UnexpectedParameter} is expected but nothing is supplied");
    }
    
    
    return {};
}

int main(int argc, char** argv)
{
    try
    {
        TestMain().DS_TRY_ACT(ssLOG_LINE(DS_TMP_ERROR.ToString()); return 1);
        return 0;
    }
    catch(std::exception& ex)
    {
        ssLOG_LINE(ex.what());
        return 1;
    }
    return 1;
}
