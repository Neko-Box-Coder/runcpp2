# Building Project Sources

## Specifying Build Type

The `BuildType` setting specifies what type of output to build. There are four supported types:

- **Executable**: Build as an executable program (default)
- **Static**: Build as a static library
- **Shared**: Build as a shared library
- **Objects**: Only compile to object files without linking

???+ example
    ```yaml
    # Build as a static library
    BuildType: Static
    ```

!!! note
    If not specified, the default build type is Executable.

!!! warning
    When you specify `BuildType` as `Executable`, it will still produce a **shared library** for running.
    Under the hood, runcpp2 simply loads the shared library and call the `main()` function.
    
    The reason of this behavior is because this makes it possible to "catch" if there's any
    missing external (shared) libraries that failed to be resolved, either because of missing
    `.dll`/`.so` or misconfigured search path. 
    
    This allows runcpp2 to differentiate a failure on resolving shared library and if the program
    just returns a non-zero exit code.
    
    Therefore, when calling with the build flag `--build`, the output of the binary is **shared 
    library instead of an executable**. This behavior can be **overridden** by passing the 
    `--executable`/`-e` flag to force runcpp2 to produce the executable.

---

## Editing Compile And Link Flags

You can modify compile and link flags using `OverrideCompileFlags` and `OverrideLinkFlags`. 
Each setting supports two operations:

- `Remove`: Remove flags from the default flags
- `Append`: Add additional flags after the default flags

???+ example
    ```yaml
    OverrideCompileFlags:
        Windows:
            "msvc":
                Remove: "/W3"        # Remove default warning level
                Append: "/W4 /WX"    # Use W4 and treat warnings as errors
        DefaultPlatform:
            "g++":
                Append: "-Wall -Wextra -Werror"

    OverrideLinkFlags:
        Linux:
            "g++":
                Append: "-Wl,-rpath,\\$ORIGIN"    # Add rpath for shared libraries
    ```

!!! warning
    Flag modifications are passed directly to the shell. Be cautious when using variables or 
    user-provided input in your build commands.

!!! note
    The default flags for each profile can be found in your user config file. 
    Run `runcpp2 --show-config-path` to locate it.

---

## Adding Source Files And Include Paths

You can add additional source files and include paths using `OtherFilesToBeCompiled` and `IncludePaths`.
All paths are relative to the script file's location.

???+ example
    ```text title="Project Structure"
    project/
    ├── main.cpp
    ├── src/
    │   └── utils.cpp
    │   └── helper.cpp
    └── include/
        └── utils.hpp
        └── helper.hpp
    ```
    
    ```yaml title="Build Settings"
    OtherFilesToBeCompiled:
    -   "./src/utils.cpp"
    -   "./src/helper.cpp"
    IncludePaths:
    -   "./include"
    ```

!!! note
    You can specify different source files for different platforms/profiles, same for IncludePaths:
    ```yaml
    OtherFilesToBeCompiled:
        Windows:
            "msvc":
            -   "./src/windows_impl.cpp"
        Unix:
            "g++":
            -   "./src/unix_impl.cpp"
    IncludePaths:
        Windows:
            "msvc":
            -   "./include/win/msvc"
        Unix:
            "g++":
            -   "./include/unix/gcc"
    ```

### Globbing Source Files

**Coming Soon**. See [Roadmap](../TODO.md)

### Mixing C And C++ Files

When building a project with a mixture of c and c++ files, the same profile will be used for all files. 

!!! note
    This is different from other build systems like CMake where it will use the c compiler for c 
    files and the c++ compiler for c++ files. 
    
    ??? example
        If you need the same behavior, you will need to create a script file for building the c files as if it is a standalone library first
        
        ```text title="Project Structure"
        project/
        ├── main.cpp
        ├── main.yaml
        ├── src/
        │   └── utils.c
        │   └── math.c
        │   └── helper.cpp
        └── include/
            └── utils.h
            └── math.h
            └── helper.hpp
        ```

        ```c title="src/utils.c"
        /*runcpp2
        Language: "c"
        RequiredProfiles:
            DefaultPlatform: ["gcc"]
        OtherFilesToBeCompiled:
        -   "./math.c"
        IncludePaths:
        -   "../include"
        */
        #include "utils.h"
        int add(int a, int b) { return a + b; }
        ```
        
        ```yaml title="main.yaml"
        Dependencies:
        -   Name: "utils"
            Source:
                Local:
                    Path: "./src"
            LibraryType: "Static"
            IncludePaths:
                -   "./include"
            Build:
            -   "runcpp2 -b ./utils.c"
            LinkProperties:
                SearchLibraryNames: ["utils"]
                SearchDirectories: ["./"]
        ```

---

## Adding Defines

You can add preprocessor definitions using the `Defines` setting. Defines can be specified with or 
without values, for different platforms/profiles:

???+ example
    ```yaml
    # For DefaultPlatform & DefaultProfile
    Defines:
    -   "DEBUG"                    # Define without value (#define DEBUG)
    -   "VERSION_MAJOR=1"          # Define with value (#define VERSION_MAJOR 1)
    -   "APP_NAME=\"MyApp\""       # Define with string value (#define APP_NAME "MyApp")
    ```

---

## Adding Command Hooks

runcpp2 provides four types of command hooks that run at different stages of the build, all of which
can be configured per platform/profile:

1. **Setup**: Run once before the script is first built
    - Runs at the script's location when no build directory exists
    - Useful for one-time initialization

2. **PreBuild**: Run before each build
    - Runs in the build directory before compilation starts
    - Useful for generating files or updating dependencies

3. **PostBuild**: Run after each successful build
    - Runs in the output directory where binaries are located
    - Useful for copying resources or post-processing binaries

4. **Cleanup**: Run when using the `--cleanup` option
    - Runs at the script's location before the build directory is removed
    - Useful for cleaning up generated files

???+ example
    ```yaml
    Setup:
        Windows:
            DefaultProfile:
            -   "echo Setting up in %cd%"
            -   "mkdir assets"
    
    PreBuild:
    -   "python generate_version.py"    # Generate version header
    
    PostBuild:
        Unix:
            DefaultProfile:
            -   "cp -r assets/* ."              # Copy assets to output
    
    Cleanup:
        Unix:
            DefaultProfile:
            -   "rm -rf assets"                 # Clean up generated files
    ```

!!! warning
    All commands are passed directly to the shell. Be cautious when using variables or 
    user-provided input in your commands.

---

## Intellisense and language server support

**Coming Soon**. See [Roadmap](../TODO.md)
