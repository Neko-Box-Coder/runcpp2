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
            std::string CompileArgs;
            
            bool ParseYAML_Node(YAML::Node& profileNode);
            std::string ToString(std::string indentation) const;
    };
}

#endif