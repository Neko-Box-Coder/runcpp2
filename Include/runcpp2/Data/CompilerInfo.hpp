#ifndef RUNCPP2_DATA_COMPILER_INFO_HPP
#define RUNCPP2_DATA_COMPILER_INFO_HPP

#include "runcpp2/Data/ParseCommon.hpp"
#include "ryml.hpp"

#include <vector>
#include <string>
#include <unordered_map>

namespace runcpp2
{
    namespace Data
    {
        class CompilerInfo
        {
            public:
                std::unordered_map<PlatformName, std::string> EnvironmentSetup;
                std::string Executable;
                
                std::unordered_map<PlatformName, std::string> DefaultCompileFlags;
                std::unordered_map<PlatformName, std::string> ExecutableCompileFlags;
                std::unordered_map<PlatformName, std::string> StaticLibCompileFlags;
                std::unordered_map<PlatformName, std::string> SharedLibCompileFlags;
                
                struct Args
                {
                    std::string CompilePart;
                    std::string IncludePart;
                    std::string InputPart;
                    std::string OutputPart;
                };
                
                Args CompileArgs;
                
                bool ParseYAML_Node(ryml::ConstNodeRef& node);
                std::string ToString(std::string indentation) const;
        };
    }
}

#endif