# Program Manual

## Usage
`#!shell runcpp2 [options] [input_file]`

## Options
```text
Options:
  Run/Build:
    -b,  --[b]uild                          Build the script but don't run it
    -o,  --[o]utput <Output Dir>            Output files to the directory specified, must be used with --build
    -w,  --[w]atch                          Watch script changes and output any compiling errors
    -l,  --[l]ocal                          Build in the current working directory under .runcpp2 directory
    -e,  --[e]xecutable                     Runs as executable instead of shared library
    -c,  --[c]onfig <file>                  Use specified config file instead of default
    -t,  --create-script-[t]emplate <file>  Creates/prepend runcpp2 script info template
    -s,  --[s]ource-only                    (Re)Builds source files only without building dependencies.
                                                The previous built binaries will be used for dependencies.
                                                Requires dependencies to be built already.
    -j,  --[j]obs                           Maximum number of threads running. Defaults to 8
  Reset/Cleanup:
    -rb, --[r]e[b]uild                      Deletes compiled source files cache and rebuild
    -ru, --[r]eset-[u]ser-config            Replace current user config with the default one
    -rd, --[r]eset-[d]ependencies <names>   Reset dependencies (comma-separated names, or "all" for all)
    -cu, --[c]lean[u]p                      Run cleanup commands and remove build directory
  Settings:
    -sc, --[s]how-[c]onfig-path             Show where runcpp2 is reading the config from
    -v,  --[v]ersion                        Show the version of runcpp2
    -h,  --[h]elp                           Show this help message
         --log-level <level>                Sets the log level (Normal, Info, Debug) for runcpp2
  Others:
         --tutorial                         Start interactive tutorial
```


