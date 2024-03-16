#ifndef RUNCPP2_DATA_COMPILER_PROFILE_HPP
#define RUNCPP2_DATA_COMPILER_PROFILE_HPP

#include "runcpp2/Data/CompilerInfo.hpp"
#include "runcpp2/Data/LinkerInfo.hpp"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace runcpp2
{
    namespace Data
    {
        class CompilerProfile
        {
            public:
                std::string Name;
                std::unordered_set<std::string> FileExtensions;
                std::unordered_set<std::string> Languages;
                std::vector<std::string> SetupSteps;
                std::unordered_map<std::string, std::string> ObjectFileExtensions;
                std::unordered_map<std::string, std::vector<std::string>> SharedLibraryExtensions;
                std::unordered_map<std::string, std::vector<std::string>> StaticLibraryExtensions;
                std::unordered_map<std::string, std::vector<std::string>> DebugSymbolFileExtensions;
                
                CompilerInfo Compiler;
                LinkerInfo Linker;
                
                bool ParseYAML_Node(YAML::Node& profileNode);
                std::string ToString(std::string indentation) const;
        };
    }
}

#endif