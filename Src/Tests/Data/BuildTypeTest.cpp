#include "runcpp2/Data/BuildType.hpp"
#include "runcpp2/Data/BuildTypeHelper.hpp"
#include "ssTest.hpp"
#include "runcpp2/runcpp2.hpp"

int main(int argc, char** argv)
{
    runcpp2::SetLogLevel("Warning");
    
    using namespace runcpp2::Data;

    ssTEST_INIT_TEST_GROUP();
    
    ssTEST("BuildType String Conversion Should Work")
    {
        //Test BuildTypeToString
        ssTEST_OUTPUT_ASSERT(   "Executable string", 
                                BuildTypeToString(BuildType::EXECUTABLE) == "Executable");
        ssTEST_OUTPUT_ASSERT(   "Static string", 
                                BuildTypeToString(BuildType::STATIC) == "Static");
        ssTEST_OUTPUT_ASSERT(   "Shared string", 
                                BuildTypeToString(BuildType::SHARED) == "Shared");
        ssTEST_OUTPUT_ASSERT(   "Objects string", 
                                BuildTypeToString(BuildType::OBJECTS) == "Objects");
        ssTEST_OUTPUT_ASSERT("COUNT string", BuildTypeToString(BuildType::COUNT) == "");

        //Test StringToBuildType
        ssTEST_OUTPUT_ASSERT(   "Executable from string", 
                                StringToBuildType("Executable") == BuildType::EXECUTABLE);
        ssTEST_OUTPUT_ASSERT(   "Static from string", 
                                StringToBuildType("Static") == BuildType::STATIC);
        ssTEST_OUTPUT_ASSERT(   "Shared from string", 
                                StringToBuildType("Shared") == BuildType::SHARED);
        ssTEST_OUTPUT_ASSERT(   "Objects from string", 
                                StringToBuildType("Objects") == BuildType::OBJECTS);

        //Test invalid strings
        ssTEST_OUTPUT_ASSERT("Empty string", StringToBuildType("") == BuildType::COUNT);
        ssTEST_OUTPUT_ASSERT("Invalid string", StringToBuildType("Invalid") == BuildType::COUNT);
        ssTEST_OUTPUT_ASSERT("Case sensitive", StringToBuildType("executable") == BuildType::COUNT);
    };
    
    ssTEST("BuildType COUNT Should Match Number of Types")
    {
        ssTEST_OUTPUT_ASSERT("COUNT value", static_cast<int>(BuildType::COUNT) == 4);
    };
    
    ssTEST("BuildType Should Generate Correct Output Paths")
    {
        ssTEST_OUTPUT_SETUP
        (
            runcpp2::Data::FilesTypesInfo filesTypes;
            
            //Set up StaticLinkFile properties
            filesTypes.StaticLinkFile.Prefix["Windows"] = "";
            filesTypes.StaticLinkFile.Prefix["Unix"] = "lib";
            filesTypes.StaticLinkFile.Extension["Windows"] = ".lib";
            filesTypes.StaticLinkFile.Extension["Unix"] = ".a";

            runcpp2::Data::Profile profile;
            profile.FilesTypes = filesTypes;
            profile.Name = "GCC";
            ghc::filesystem::path buildDir = "build";
            std::string scriptName = "test";
        );

        ssTEST_OUTPUT_EXECUTION
        (
            ghc::filesystem::path outPath;
            bool result = runcpp2::Data::BuildTypeHelper::GetOutputPath(
                buildDir, scriptName, profile, runcpp2::Data::BuildType::STATIC, false, outPath);
        );

        ssTEST_OUTPUT_ASSERT("GetOutputPath should succeed", result == true);
        #ifdef _WIN32
            ssTEST_OUTPUT_ASSERT("Static library path", outPath, "build/test.lib");
        #else
            ssTEST_OUTPUT_ASSERT("Static library path", outPath, "build/libtest.a");
        #endif
    };
    
    ssTEST("BuildType Should Map To Correct File Types")
    {
        ssTEST_OUTPUT_SETUP
        (
            runcpp2::Data::FilesTypesInfo filesTypes;
        );

        ssTEST_OUTPUT_ASSERT(   "Static build type",
                                BuildTypeHelper::GetOutputFileProperties(   filesTypes, 
                                                                            BuildType::STATIC, 
                                                                            false), 
                                &filesTypes.StaticLinkFile);
    };
    
    ssTEST_END_TEST_GROUP();
    return 0;
} 