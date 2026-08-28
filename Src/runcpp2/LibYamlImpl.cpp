//libyaml implementations
extern "C" {
    #define YAML_STR(x) #x
    #define YAML_VERSION_MAJOR 0
    #define YAML_VERSION_MINOR 2
    #define YAML_VERSION_PATCH 5
    #define YAML_VERSION_STRING YAML_STR(YAML_VERSION_MAJOR) "." YAML_STR(YAML_VERSION_MINOR) "." YAML_STR(YAML_VERSION_PATCH)
    
    #if defined(_MSC_VER)
        #pragma warning(push)
        #pragma warning(disable: 4706 4701 4702)
    #endif
    
    #include "libyaml/src/api.c"
    #include "libyaml/src/dumper.c"
    #include "libyaml/src/emitter.c"
    #include "libyaml/src/loader.c"
    #include "libyaml/src/parser.c"
    #include "libyaml/src/reader.c"
    #include "libyaml/src/scanner.c"
    #include "libyaml/src/writer.c"
    
    #if defined(_MSC_VER)
        #pragma warning(pop)
    #endif
}
