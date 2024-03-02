#ifndef RUNCPP2_DATA_COMPILER_INFO_HPP
#define RUNCPP2_DATA_COMPILER_INFO_HPP

#include "yaml-cpp/yaml.h"

#include <string>

namespace runcpp2
{
    class CompilerInfo
    {
        public:
            std::string Executable;
            std::string DefaultCompileFlags;
            
            struct Args
            {
                std::string CompilePart;
                std::string IncludePart;
                std::string InputPart;
                std::string OutputPart;
            };
            
            Args CompileArgs;
            
            bool ParseYAML_Node(YAML::Node& profileNode);
            std::string ToString(std::string indentation) const;
    };
}

#endif