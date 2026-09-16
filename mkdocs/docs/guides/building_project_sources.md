# Build Info Basics

This page describes how to specify the most common options for the build info.

## Build Type

The `BuildType` setting specifies what type of output to build. Here are the supported types:

- **Executable**: Build as an executable program (default)
- **Static**: Build as a static library
- **Shared**: Build as a shared library

```yaml
BuildType: Static # Static library
```

!!! note
    If not specified, the default build type is Executable.

---

## Required Profiles

A profile represents a compiler/linker toolchain. You can set what only profiles are allowed to 
compile/link this project. For example

```yaml
RequiredProfiles: 
    Windows: ["g++", "msvc"]
    Linux: ["g++"]
    MacOS: ["g++"]
```

---

## Compile And Link Flags

You can modify compile and link flags using `OverrideCompileFlags` and `OverrideLinkFlags`. 
Each setting supports two operations:

- `Remove`: Remove flags from the profile default flags
- `Append`: Add additional flags in addition to the profile default flags

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
    Run `runcpp2 show-config-path` to locate it.

---

## Source Files And Include Paths

You can specify source files and include paths using `SourceFiles` and `IncludePaths`. If you main 
file is a source file (i.e. .cpp or .c), then it is included as a source file implicitly.

All paths are relative to main file's location, like so

```text title="Project Structure"
project/
├── main.cpp
├── main.yaml
├── src/
│   └── utils.cpp
│   └── helper.cpp
└── include/
    └── utils.hpp
    └── helper.hpp
```

```yaml title="main.yaml"
SourceFiles: ["./src/utils.cpp", "./src/helper.cpp"]
IncludePaths: ["./include"]
```

!!! note
    You can specify different source files for different platforms/profiles:
    ```yaml
    SourceFiles:
        Windows:
            "msvc": ["./src/windows_impl.cpp"]
        Unix:
            "g++": ["./src/unix_impl.cpp"]
    IncludePaths:
        Windows:
            "msvc": ["./include/win/msvc"]
        Unix:
            "g++": ["./include/unix/gcc"]
    ```

---

## Defines

You can add preprocessor definitions using the `Defines` field. Defines can be specified with or 
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
    
    ??? info
        - Runs at the script's location when no build directory exists
        - Useful for one-time initialization

2. **PreBuild**: Run before each build
    
    ??? info 
        - Runs in the build directory before compilation starts
        - Useful for generating files or updating dependencies

3. **PostBuild**: Run after each successful build
    
    ??? info
        - Runs in the output directory where binaries are located
        - Useful for copying resources or post-processing binaries

4. **Cleanup**: Run when `runcpp2 reset ...` is used
    
    ??? info
        - Runs at the script's location before the build directory is removed
        - Useful for cleaning up generated files

```yaml
Setup:
    Windows:
        DefaultProfile: ["echo Setting up in %cd%", "mkdir assets"]
PreBuild: ["python generate_version.py"] # Generate version header
PostBuild:
    Unix:
        DefaultProfile: ["cp -r assets/* ."] # Copy assets to output
Cleanup:
    Unix:
        DefaultProfile: ["rm -rf assets"] # Clean up generated files
```

!!! warning
    All commands are passed directly to the shell. Be cautious when using variables or user-provided 
    input in your commands.

---

## Importing Fields

Common values that are shared between multiple build info files can be reused by being imported to 
the current build info.

This can be done by specifying a path to the target yaml as a string or as an array of strings.

```yaml
Import: "./OtherDefines.yaml"
```
