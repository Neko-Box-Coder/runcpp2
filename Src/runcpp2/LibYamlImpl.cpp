//libyaml implementations
extern "C" {
    #define YAML_STR(x) #x
    #define YAML_VERSION_MAJOR 0
    #define YAML_VERSION_MINOR 2
    #define YAML_VERSION_PATCH 5
    #define YAML_VERSION_STRING YAML_STR(YAML_VERSION_MAJOR) "." YAML_STR(YAML_VERSION_MINOR) "." YAML_STR(YAML_VERSION_PATCH)
    
    #include "../src/api.c"
    #include "../src/dumper.c"
    #include "../src/emitter.c"
    #include "../src/loader.c"
    #include "../src/parser.c"
    #include "../src/reader.c"
    #include "../src/scanner.c"
    #include "../src/writer.c"
}
