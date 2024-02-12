#ifndef RUNCPP2_COMPILER_PROFILE_HPP
#define RUNCPP2_COMPILER_PROFILE_HPP

#include "runcpp2/CompilerInfo.hpp"
#include "runcpp2/LinkerInfo.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace runcpp2
{
    class CompilerProfile
    {
        public:
            std::string Name;
            std::vector<std::string> FileExtensions;
            std::vector<std::string> Languages;
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

#endif