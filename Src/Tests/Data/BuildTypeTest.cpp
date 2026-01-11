#include "runcpp2/Data/BuildType.hpp"
#include "runcpp2/Data/BuildTypeHelper.hpp"
#include "runcpp2/runcpp2.hpp"
#include "ssLogger/ssLog.hpp"

DS::Result<void> TestMain()
{
    //BuildType Should Generate Correct Output Paths
    {
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

        std::vector<ghc::filesystem::path> outTargets;
        std::vector<bool> outIsRunnable;
        bool result = runcpp2   ::Data
                                ::BuildTypeHelper
                                ::GetPossibleOutputPaths(   buildDir, 
                                                            scriptName, 
                                                            profile, 
                                                            runcpp2::Data::BuildType::STATIC, 
                                                            outTargets, 
                                                            outIsRunnable);

        DS_ASSERT_TRUE(result);
        DS_ASSERT_EQ(outTargets.size(), 1);
        #ifdef _WIN32
            DS_ASSERT_EQ(outTargets.at(0), "build/test.lib");
        #else
            DS_ASSERT_EQ(outTargets.at(0), "build/libtest.a");
        #endif
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
