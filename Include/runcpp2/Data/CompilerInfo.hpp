#ifndef RUNCPP2_DATA_COMPILER_INFO_HPP
#define RUNCPP2_DATA_COMPILER_INFO_HPP

#include "ryml.hpp"

#include <string>

namespace runcpp2
{
    namespace Data
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
                
                bool ParseYAML_Node(ryml::ConstNodeRef& profileNode);
                std::string ToString(std::string indentation) const;
        };
    }
}

#endif