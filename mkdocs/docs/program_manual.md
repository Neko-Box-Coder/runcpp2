# Program Manual

## Options
```text
Usage: runcpp2 <action> [options] [input file] [run args]
Actions:
    run                                                 Runs the input file
    build                                               Build the input file
    watch                                               Watch for any changes in source files and output any compiling errors
    template                                            Creates/prepend runcpp2 build info template to the input file
    regen-user-config                                   Replace current user config with the default one
    reset                                               Perform cleanup on both/either the source and/or the dependencies
    show-config-path                                    Show where runcpp2 is reading the config from
    version                                             Show the version of runcpp2
    tutorial                                            Start interactive tutorial
    help                                                Show this help message
```

Here are the options for the main actions:

### Run
```text
Usage: runcpp2 run [options] <input file> [run args]
Options:
  -h,  --[h]elp                                         Show this help message
  -l,  --[l]ocal                                        Build in the current working directory under .runcpp2 directory
  -s,  --[s]ource-only                                  Builds source files only without building dependencies.
                                                        The previous built binaries will be used for dependencies.
                                                        Requires dependencies to be built already.
  -p,  --[p]arameters <name1=val1;name2=val2;...>       Parameter name value pairs that perform text replacement on the build config
  -j,  --[j]obs <number>                                Maximum number of threads running. Defaults to 8
  -c,  --[c]onfig <file>                                Use specified config file instead of default
       --log-level <level>                              Sets the log level (error, normal, info, debug) for runcpp2
```

### Build
```text
Usage: runcpp2 build [options] <input file>
Options:
  -h,  --[h]elp                                         Show this help message
  -l,  --[l]ocal                                        Build in the current working directory under .runcpp2 directory
  -s,  --[s]ource-only                                  Builds source files only without building dependencies.
                                                        The previous built binaries will be used for dependencies.
                                                        Requires dependencies to be built already.
  -p,  --[p]arameters <name1=val1;name2=val2;...>       Parameter name value pairs that perform text replacement on the build config
  -j,  --[j]obs <number>                                Maximum number of threads running. Defaults to 8
  -c,  --[c]onfig <file>                                Use specified config file instead of default
  -rb, --[r]e[b]uild                                    Deletes compiled source files cache and rebuild
  -o,  --[o]utput-dir <output dir>                      Specify a directory to output to.
       --log-level <level>                              Sets the log level (error, normal, info, debug) for runcpp2
```

### Watch
```text
Usage: runcpp2 watch [options] <input file>
Options:
  -h,  --[h]elp                                         Show this help message
  -l,  --[l]ocal                                        Build in the current working directory under .runcpp2 directory
  -s,  --[s]ource-only                                  Builds source files only without building dependencies.
                                                        The previous built binaries will be used for dependencies.
                                                        Requires dependencies to be built already.
  -p,  --[p]arameters <name1=val1;name2=val2;...>       Parameter name value pairs that perform text replacement on the build config
  -j,  --[j]obs <number>                                Maximum number of threads running. Defaults to 8
  -c,  --[c]onfig <file>                                Use specified config file instead of default
       --log-level <level>                              Sets the log level (error, normal, info, debug) for runcpp2
```

### Reset
```text
Usage: runcpp2 reset [options] <input file>
Options:
  -h,  --[h]elp                                         Show this help message
  -l,  --[l]ocal                                        Build in the current working directory under .runcpp2 directory
  -p,  --[p]arameters <name1=val1;name2=val2;...>       Parameter name value pairs that perform text replacement on the build config
  -j,  --[j]obs <number>                                Maximum number of threads running. Defaults to 8
  -c,  --[c]onfig <file>                                Use specified config file instead of default
  -d,  --[d]ependencies <dependencies>                  Reset dependencies only (comma-separated names, or "all" for all dependencies)
       --log-level <level>                              Sets the log level (error, normal, info, debug) for runcpp2
```

