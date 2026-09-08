# User Config

### `DefaultPlatform`
- Description: Evaluates to the host platform.

## Config

!!! warning
    All command substitutions in this file are passed directly to the shell. Exercise caution when using variables or user-provided input in your build commands to prevent potential security vulnerabilities.

### `PreferredProfile`
- Type: `Platform Map` with `string`
- Optional: `false`
- Default: None
- Description: A profile to be used if not specified while running the build script
??? example
    ```yaml
    PreferredProfile: 
        DefaultPlatform: "gcc"
        Windows: "msvc"
    ```

### `Profiles`
- Type: `Profile[]`
- Optional: `false`
- Default: None
- Description: List of compiler/linker profiles that instruct how to compile/link

### `Parameters`
- Type: `ParametersInfo`
- Optional: `true`
- Default: None
- Description: See description of `ParametersInfo` type under [Build Settings](./build_settings.md)
??? example
    ```yaml
    Parameters:
        Param1:
            Optional: true
            Default: ""
            Array: false
            Constraint: "None"
    ```

### `Variables`
- Type: `VariablesInfo`
- Optional: `true`
- Default: None
- Description: See description of `VariablesInfo` type under [Build Settings](./build_settings.md)
??? example
    ```yaml
    Variables:
        VarName1: "Some string {Param1} substitution"
    ```

### `Import`
- Type: `string` or `string[]`
- Optioanl: `true`
- Default: None
- Description: Import other yaml files to merge to this file. Import can either be a single path or a list of paths. If there's any parameter/variables in the import file, it will applied to that file first before merging
??? example
    ```yaml
    Import: "./OtherProfiles.yaml"
    ```

## Special Types

### `Platform Map`
- Description: A map of platforms

???+ Example
    ```yaml
    ExampleSettings:
        Windows:
            ...
        Linux:
            ...
        MacOS:
            ...
    ```

### `File Info`
- Type: `map`
- Description: Information of different file types
??? example
    ```yaml
    Prefix:
        DefaultPlatform: ""
        Linux: "lib"
        MacOS: "lib"
    Extension:
        Windows: ".lib"
        Linux: ".so"
        MacOS: ".dylib"
    ```

#### `Prefix`
- Type: `Platform Map` with `string`
- Optional: `false
- Default: None
- Description: Prefix text of the file

#### `Extension`
- Type: `Platform Map` with `string`
- Optional: `false
- Default: None
- Description: Extension text of the file (including .)

### `Command Info`
- Type: `map`
- Description: Information for assembling a command
- Child Fields:
    - `Flags`
        - Type: `string`
        - Optional: `false`
        - Default: None
        - Description: Default flags to be substituted as `{Stage.CompileFlags}`/`{Stage.LinkFlags}`. This can be overridden by `OverrideCompileFlags`/`OverrideLinkFlags`
    - `Executable`
        - Type: `string`
        - Optional: `false`
        - Default: None
        - Description: The executable to be substituted as `{Stage.Executable}`
    - `RunParts`
        - Type: `RunPartInfo[]`
        - Optional: `false`
        - Default: None
        - Description: The components for the command to be run

### `Profile`

#### `Name`
- Type: `string`
- Optional: `false`
- Default: None
- Description: Name (case sensitive) of the profile that can be queried from a script
??? example
    ```yaml
    Name: "g++"
    ```

#### `NameAliases`
- Type: `string[]`
- Optional: `true`
- Default: None
- Description: Name aliases (case sensitive) of the current profile
??? example
    ```yaml
    NameAliases: ["mingw"]
    ```

#### `FileExtensions`
- Type: `string[]`
- Optional: `false`
- Default: None
- Description: The file extensions associated with the profile

#### `Languages`
- Type: `string[]`
- Optional: `false`
- Default: None
- Description: The languages supported by the profile

#### `Setup`
- Type: `Platform Map` with `string[]`
- Optional: `true`
- Default: None
- Description: The commands to run in **shell** before calling the compiler/linker for each platform. This is run inside the root build directory.

#### `Cleanup`
- Type: `Platform Map` with `string[]`
- Optional: `true`
- Default: None
- Description: The commands to run in **shell** after calling the compiler/linker for each platform. This is run inside the root build directory.

#### `FileTypes`
- Type: `map`
- Optional: `false`
- Default: None
- Description: Info for different file types
- Child Fields:

    - `ObjectLinkFile`
        - Type: `File Info`
        - Optional: `false`
        - Default: None
        - Description: The file properties for the files to be **linked** as object file for each platform
    
    - `SharedLinkFile`
        - Type: `File Info`
        - Optional: `false`
        - Default: None
        - Description: The file properties for the files to be **linked** as shared libraries for each platform
    
    - `SharedLibraryFile`
        - Type: `File Info`
        - Optional: `false`
        - Default: None
        - Description: The file properties for the files to be **copied** as shared libraries for each platform

    - `StaticLinkFile`
        - Type: `File Info`
        - Optional: `false`
        - Default: None
        - Description: The file properties for the files to be linked as static libraries for each platform

    - `ExecutableFile`
        - Type: `File Info`
        - Optional: `false`
        - Default: None
        - Description: The file properties for the files to be **copied** as executable for each platform

    - `DebugSymbolFile`
        - Type: `File Info`
        - Optional: `true`
        - Default: None
        - Description: The file properties for debug symbols to be copied alongside the binary for each platform

#### `Import`
- Type: `string` or `string[]`
- Optioanl: `true`
- Default: None
- Description: See description of `Import` under [Build Settings](./build_settings.md)

#### `Parameters`
- Type: `ParametersInfo`
- Optional: `true`
- Default: None
- Description: See description of `ParametersInfo` type under [Build Settings](./build_settings.md)
??? example
    ```yaml
    Parameters:
        Param1:
            Optional: true
            Default: ""
            Array: false
            Constraint: "None"
    ```

#### `Variables`
- Type: `VariablesInfo`
- Optional: `true`
- Default: None
- Description: See description of `VariablesInfo` type under [Build Settings](./build_settings.md)
??? example
    ```yaml
    Variables:
        VarName1: "Some string {Param1} substitution"
    ```

#### `Compiler`
- Type: `map`
- Optional: `false`
- Default: none
- Description: Compiler settings, run once per input file
- Child Fields:
    
    ##### `CheckExistence`
    - Type: `Platform Map` with `string`
    - Optional: `false`
    - Default: None
    - Description: Shell command to use for checking if the executable exists or not
    
    ##### `CompileTypes`
    - Type: `map`
    - Optional: `false`
    - Default: None
    - Description: Compilation commands for different file types. Below are all the built-in variables that can be used.
    ??? info
        Here are a list of substitution strings for RunParts, Setup and Cleanup. To escape '{' and '}' to avoid substitutioon, simply repeat the '{' or '}' character again.
        
        So to escape `"${MyBashVariable}"`, it will become `"${{MyBashVariable}}"` 

        ### Constants
        - `{Stage.SharedLibraryFile.Prefix}`
        - `{Stage.SharedLinkFile.Prefix}`
        - `{Stage.StaticLinkFile.Prefix}`
        - `{Stage.ObjectLinkFile.Prefix}`
        - `{Stage.ExecutableFile.Prefix}`
        - `{Stage.DebugSymbolFile.Prefix}`
        - `{Stage.SharedLibraryFile.Extension}`
        - `{Stage.SharedLinkFile.Extension}`
        - `{Stage.StaticLinkFile.Extension}`
        - `{Stage.ObjectLinkFile.Extension}`
        - `{Stage.ExecutableFile.Extension}`
        - `{Stage.DebugSymbolFile.Extension}`
        - `{/}`: Filesystem separator for the host platform
        
        
        ### Stage Info
        - `{Stage.Executable}`: Compiler executable
        - `{Stage.CompileFlags}`: Compile flags from config and override
        
        
        ### Input/Output Info
        - `{Stage.Input.Name}`: Name of the current input source file (without directory path and extension)
        - `{Stage.Input.Extension}`: Extension of the current input source file
        - `{Stage.Input.Directory}`: Directory of the current input source file
        - `{Stage.Input.Path}`: Full path to the current input source file
        - `{Stage.Output.Directory}`: Directory of all the output files
        
        
        ### Iterable variables, must be inside "Repeats" run type
        - `{Stage.DefineNameOnly}`: All the defines without a value specified (equivalent to #define X)
        - `{Stage.DefineName}`: Name of all the defines that has a value specified
        - `{Stage.DefineValue}`: Value of all the defines that has a value specified (use together with {Stage.DefineName})
        - `{Stage.IncludeDirectory.Path}`: Path to all the include directories
            - `{Stage.IncludeDirectory.Source.Path}`: Path to source include directories, sub array
            - `{Stage.IncludeDirectory.Dep.Path}`: Path to dependencies include directories, sub array

    - Child Fields:
        - `Executable`
            - Type: `Platform Map` with `Command Info`
            - Optional: `false`
            - Default: None
            - Description: Compilation commands for executable
        - `Static`
            - Type: `Platform Map` with `Command Info`
            - Optional: `false`
            - Default: None
            - Description: Compilation commands for static library
        - `Shared`
            - Type: `Platform Map` with `Command Info`
            - Optional: `false`
            - Default: None
            - Description: Compilation commands for shared library



**TODO**

## `UserConfig.yaml`
```yaml
# WARNING: All command substitutions in this file are passed directly to the shell.
#          Exercise caution when using variables or user-provided input in your build commands
#          to prevent potential security vulnerabilities.

# A profile to be used if not specified while running the build script
PreferredProfile: 
    DefaultPlatform: "gcc"
    Windows: "msvc"

# List of compiler/linker profiles that instruct how to compile/link
# See "./Default/AnnotatedG++.yaml" for the documentation of each field in a profile entry
# Profiles: []

Import: "./Default/DefaultProfiles.yaml"
```

## `Default/AnnotatedG++.yaml`
```yaml
# DO NOT modify this file. Changes will be overwritten when there's a reset or update

# Name (case sensitive) of the profile that can be queried from a script
Name: "g++"

# (Optional) Name aliases (case sensitive) of the current profile
NameAliases: ["mingw"]

# The file extensions associated with the profile
FileExtensions: [.cpp, .cc, .cxx, .c]

# The languages supported by the profile
Languages: ["c", "c++"]

# (Optional) The commands to run in **shell** before calling the compiler/linker for each platform.
#            This is run inside the root build directory.
# Setup: 
#     DefaultPlatform: []

# (Optional) The commands to run in **shell** after calling the compiler/linker for each platform.
#            This is run inside the root build directory.
# Cleanup: 
#     DefaultPlatform: []

# The file properties for the object files for each platform. 
# See "./CommonFileTypes.yaml" for FilesTypes
# FilesTypes: ...

# (Optional) We can use the "Import" field to import other yaml files to merge to this file.  We are importing "FilesTypes" here.
#            Import can either be a single path or a list of paths. 
#            If there's any parameter/variables in the import file, it will applied to that file 
#            first before merging
Import: "./CommonFileTypes.yaml"

# (Optional) Parameters are user supplied input that can be substituted into any keys or values, with the syntax of `{<parameter name>}`
#            This in only applied to this file.
Parameters:
    # Name of the parameter
    Profile.CompileMode:
        # (Optional) If this parameter is mandatory. Defaults to `true`
        # Optional: true
        
        # (Optional) Default value of the parameter. Defaults to empty
        Default: "Debug"
        
        # (Optional) Parameter for array. If this is true, comma separated values is expected. Defaults to false.
        #            If this is true, this parameter can only be used in config values that expect an array.
        # Array: false
        
        # (Optional) Constraint of this parameter value. An array means a multiple choice constraint.
        # Can be one of the following
        #     "None": No constraint
        #     "Bool": `true`, `false`, `1` or `0`
        #     "Float[:<Min Float>,<Max Float>]": Floating point number with optional inclusive min and max
        #     "Int[:<Min Int>,<Max Int>]": Integer number  with optional inclusive min and max
        #     ["<Choice 1>[:Mapped Value 1]", "<Choice 2>[:Mapped Value 2]", ...]: List of choices with optional corresponding mapped values
        Constraint: 
        -   "Debug:-std=c++17 -Wall -g -Og"
        -   "Release:-std=c++17 -Wall -O2"
    
    Profile.OutputPreprocess:
        Default: "false"
        Constraint:
        -   "true: -E"
        -   "1: -E"
        -   "false:"
        -   "0:"

# TODO: Conditional variable
# (Optional) Variables can be substituted into any keys or values (excluding "Parameters"), with the syntax of `{<variable name>}`
#            This in only applied to this file.
Variables:
    # A variable can be created by substituting a parameter into a string, using syntax of `{<parameter name>}`
    # If this contains an array parameter value, the substition is performed for each parameter array value and 
    # this variable will become an array variable, meaning this can only be used in config values that expect an array.
    Profile.Intern_CompileFlags: "{Profile.CompileMode}{Profile.OutputPreprocess}"

# Compiler settings, run once per input file
Compiler:
    # (Optional) The command to be prepend for each compile command in **shell** for each platform
    # PreRun: 
    #     DefaultPlatform: ""
    
    # Shell command to use for checking if the executable exists or not
    CheckExistence: 
        DefaultPlatform: "g++ -v"
    
    # Here are a list of substitution strings for RunParts, Setup and Cleanup. 
    # To escape '{' and '}' to avoid substitutioon, simply repeat the '{' or '}' character again.
    # So "${MyBashVariable}" will become "${{MyBashVariable}}"

    # Constants: --------------------------------------------------------------------------------------
    # {Stage.SharedLibraryFile.Prefix}
    # {Stage.SharedLinkFile.Prefix}
    # {Stage.StaticLinkFile.Prefix}
    # {Stage.ObjectLinkFile.Prefix}
    # {Stage.ExecutableFile.Prefix}
    # {Stage.DebugSymbolFile.Prefix}
    # {Stage.SharedLibraryFile.Extension}
    # {Stage.SharedLinkFile.Extension}
    # {Stage.StaticLinkFile.Extension}
    # {Stage.ObjectLinkFile.Extension}
    # {Stage.ExecutableFile.Extension}
    # {Stage.DebugSymbolFile.Extension}
    # {/}:                                  Filesystem separator for the host platform
    
    
    # Stage Info: --------------------------------------------------------------------------------------
    # {Stage.Executable}:                   Compiler executable
    # {Stage.CompileFlags}:                 Compile flags from config and override
    
    
    # Input/Output Info: --------------------------------------------------------------------------------------
    # {Stage.Input.Name}:                   Name of the current input source file (without directory path and extension)
    # {Stage.Input.Extension}:              Extension of the current input source file
    # {Stage.Input.Directory}:              Directory of the current input source file
    # {Stage.Input.Path}:                   Full path to the current input source file
    # {Stage.Output.Directory}:             Directory of all the output files
    
    
    # Iterable variables, must be inside "Repeats" run type: --------------------------------------------------------------------------------------
    # {Stage.DefineNameOnly}:                   All the defines without a value specified (equivalent to #define X)
    # {Stage.DefineName}:                       Name of all the defines that has a value specified
    # {Stage.DefineValue}:                      Value of all the defines that has a value specified (use together with {Stage.DefineName})
    
    # {Stage.IncludeDirectory.Path}:            Path to all the include directories
    # +-> {Stage.IncludeDirectory.Source.Path}: Path to source include directories, sub array
    # L-> {Stage.IncludeDirectory.Dep.Path}:    Path to dependencies include directories, sub array
    CompileTypes:
        Executable:
            DefaultPlatform:
                # Default flags to be substituted as {Stage.CompileFlags}
                Flags: "{Profile.Intern_CompileFlags}"
                
                # The executable to be substituted as {Stage.Executable}
                Executable: "g++"
                
                # The components for the command to be run
                RunParts: &g++_CompileRunParts
                -   Type: Once
                    CommandPart: "{Stage.Executable} -c {Stage.CompileFlags}"
                -   Type: Repeats
                    CommandPart: " -D{Stage.DefineNameOnly}="
                    # (Optional) A separator (such as ",") which will be inserted between each repeating parts
                    # Separator: ""
                -   Type: Repeats
                    CommandPart: " \"-D{Stage.DefineName}={Stage.DefineValue}\""
                -   Type: Repeats
                    CommandPart: " -isystem \"{Stage.IncludeDirectory.Dep.Path}\""
                -   Type: Repeats
                    CommandPart: " -I\"{Stage.IncludeDirectory.Source.Path}\""
                -   Type: Once
                    CommandPart: " \"{Stage.Input.Path}\" -o \"{Stage.Output.Directory}{/}\
                        {Stage.ObjectLinkFile.Prefix}{Stage.Input.Name}\
                        {Stage.ObjectLinkFile.Extension}\""
                
                # What files to be expected as output for the command
                ExpectedOutputFiles: &g++_CompileExpectedOutputFiles
                -   "{Stage.Output.Directory}{/}{Stage.ObjectLinkFile.Prefix}{Stage.Input.Name}\
                    {Stage.ObjectLinkFile.Extension}"
                
                # (Optional) The commands to run in **shell** BEFORE compiling
                #            This is run inside the .runcpp2 directory where the build happens.
                # Setup: []
                
                # (Optional) The commands to run in **shell** AFTER compiling
                #            This is run inside the .runcpp2 directory where the build happens.
                # Cleanup: []
        Static:
            DefaultPlatform:
                Flags: "{Profile.Intern_CompileFlags}"
                Executable: "g++"
                RunParts: *g++_CompileRunParts
                ExpectedOutputFiles: *g++_CompileExpectedOutputFiles
                # Setup: []
                # Cleanup: []
        Shared:
            DefaultPlatform:
                Flags: "{Profile.Intern_CompileFlags} -fpic"
                Executable: "g++"
                RunParts: *g++_CompileRunParts
                ExpectedOutputFiles: *g++_CompileExpectedOutputFiles
                # Setup: []
                # Cleanup: []

# Linker settings, run once
Linker:
    CheckExistence:
        DefaultPlatform: "g++ -v"
    
    # Here are a list of substitution strings for RunParts, Setup and Cleanup
    
    # Constants: --------------------------------------------------------------------------------------
    # {Stage.SharedLibraryFile.Prefix}
    # {Stage.SharedLinkFile.Prefix}
    # {Stage.StaticLinkFile.Prefix}
    # {Stage.ObjectLinkFile.Prefix}
    # {Stage.ExecutableFile.Prefix}
    # {Stage.DebugSymbolFile.Prefix}
    # {Stage.SharedLibraryFile.Extension}
    # {Stage.SharedLinkFile.Extension}
    # {Stage.StaticLinkFile.Extension}
    # {Stage.ObjectLinkFile.Extension}
    # {Stage.ExecutableFile.Extension}
    # {Stage.DebugSymbolFile.Extension}
    # {/}:                                  Filesystem separator for the host platform
    
    
    # Stage Info: --------------------------------------------------------------------------------------
    # {Stage.Executable}:                   Linker executable
    # {Stage.LinkFlags}:                    Link flags from config and override
    
    
    # Output Info: --------------------------------------------------------------------------------------
    # {Stage.Output.Name}:                  Name of the output file (without directory path and extension)
    # {Stage.Output.Directory}:             Directory of all the output files
    
    
    # Iterable variables, must be inside "Repeats" run type: --------------------------------------------------------------------------------------
    # {Stage.Input.Name}:                               Name of the files to be linked, regardless of the build type
    # +-> {Stage.Input.Dep.Name}:                       Name of the dependencies files to be linked, regardless of the build type, sub array
    # +-> {Stage.Input.Source.Name}:                    Name of the source files to be linked, regardless of the build type, sub array
    # +-> {Stage.Input.Object.Name}:                    Name of the object files to be linked, sub array
    # |   +-> {Stage.Input.Dep.Object.Name}:            Name of the dependencies object files to be linked, Sub array
    # |   L-> {Stage.Input.Source.Object.Name}:         Name of the source object files to be linked, Sub array
    # +-> {Stage.Input.Shared.Name}:                    Name of the shared dependencies files to be linked, sub array
    # L-> {Stage.Input.Static.Name}:                    Name of the static dependencies files to be linked, sub array
    
    # {Stage.Input.Extension}:                          File Extensions of the files to be linked, regardless of the build type
    # +-> {Stage.Input.Dep.Extension}:                  File Extensions of the dependencies files to be linked, regardless of the build type
    # +-> {Stage.Input.Source.Extension}:               File Extensions of the source files to be linked, regardless of the build type
    # +-> {Stage.Input.Object.Extension}:               File Extensions of the object files to be linked, sub array
    # |   +-> {Stage.Input.Dep.Object.Extension}:       File Extensions of the dependencies object files to be linked, sub array
    # |   L-> {Stage.Input.Source.Object.Extension}:    File Extensions of the source object files to be linked, sub array
    # +-> {Stage.Input.Shared.Extension}:               File Extensions of the shared dependencies files to be linked, sub array
    # L-> {Stage.Input.Static.Extension}:               File Extensions of the static dependencies files to be linked, sub array
    
    # {Stage.Input.Directory}:                          Directories of the files to be linked, regardless of the build type
    # +-> {Stage.Input.Dep.Directory}:                  Directories of the dependencies files to be linked, regardless of the build type
    # +-> {Stage.Input.Source.Directory}:               Directories of the source files to be linked, regardless of the build type
    # +-> {Stage.Input.Object.Directory}:               Directories of the object files to be linked, sub array
    # |   +-> {Stage.Input.Dep.Object.Directory}:       Directories of the dependencies object files to be linked, sub array
    # |   L-> {Stage.Input.Source.Object.Directory}:    Directories of the source object files to be linked, sub array
    # +-> {Stage.Input.Shared.Directory}:               Directories of the shared dependencies files to be linked, sub array
    # L-> {Stage.Input.Static.Directory}:               Directories of the static dependencies files to be linked, sub array
    
    # {Stage.Input.Path}:                               Full paths to the files to be linked, regardless of the build type
    # +-> {Stage.Input.Dep.Path}:                       Full paths to the dependencies files to be linked, regardless of the build type
    # +-> {Stage.Input.Source.Path}:                    Full paths to the source files to be linked, regardless of the build type
    # +-> {Stage.Input.Object.Path}:                    Full paths to the object files to be linked, sub array
    # |   +-> {Stage.Input.Dep.Object.Path}:            Full paths to the dependencies object files to be linked, sub array
    # |   L-> {Stage.Input.Source.Object.Path}:         Full paths to the source object files to be linked, sub array
    # +-> {Stage.Input.Shared.Path}:                    Full paths to the shared dependencies files to be linked, sub array
    # L-> {Stage.Input.Static.Path}:                    Full paths to the static dependencies files to be linked, sub array
    LinkTypes:
        Executable:
            Unix:
                Flags: "-Wl,-rpath,\\$ORIGIN"
                Executable: "g++"
                RunParts:
                -   Type: Once
                    CommandPart: "{Stage.Executable} {Stage.LinkFlags} -o \"{Stage.Output.Directory}\
                        {/}{Stage.Output.Name}\""
                -   Type: Repeats
                    CommandPart: " \"{Stage.Input.Path}\""
                ExpectedOutputFiles: ["{Stage.Output.Directory}{/}{Stage.Output.Name}"]
                # Setup: []
                # Cleanup: []
            Windows:
                Flags: "-Wl,-rpath,\\$ORIGIN"
                Executable: "g++"
                RunParts:
                -   Type: Once
                    CommandPart: "{Stage.Executable} {Stage.LinkFlags} -o \"{Stage.Output.Directory}\
                        {/}{Stage.Output.Name}{Stage.ExecutableFile.Extension}\""
                -   Type: Repeats
                    CommandPart: " \"{Stage.Input.Path}\""
                ExpectedOutputFiles: 
                -   "{Stage.Output.Directory}{/}{Stage.Output.Name}{Stage.ExecutableFile.Extension}"
                # Setup: []
                # Cleanup: []
        Static:
            DefaultPlatform:
                Flags: ""
                Executable: "g++"
                RunParts:
                -   Type: Once
                    CommandPart: "{Stage.Executable} {Stage.LinkFlags} -o \"{Stage.Output.Directory}\
                        {/}{Stage.StaticLinkFile.Prefix}{Stage.Output.Name}{Stage.StaticLinkFile.Extension}\""
                -   Type: Repeats
                    CommandPart: " \"{Stage.Input.Path}\""
                ExpectedOutputFiles: 
                -   "{Stage.Output.Directory}{/}{Stage.StaticLinkFile.Prefix}{Stage.Output.Name}\
                    {Stage.StaticLinkFile.Extension}"
                # Setup: []
                # Cleanup: []
        Shared:
            DefaultPlatform:
                Flags: "-shared -Wl,-rpath,\\$ORIGIN"
                Executable: "g++"
                RunParts:
                -   Type: Once
                    CommandPart: "{Stage.Executable} {Stage.LinkFlags} -o \"{Stage.Output.Directory}\
                        {/}{Stage.SharedLibraryFile.Prefix}{Stage.Output.Name}\
                        {Stage.SharedLibraryFile.Extension}\""
                -   Type: Repeats
                    CommandPart: " \"{Stage.Input.Path}\""
                ExpectedOutputFiles: 
                -   "{Stage.Output.Directory}{/}{Stage.SharedLibraryFile.Prefix}{Stage.Output.Name}\
                    {Stage.SharedLibraryFile.Extension}"
                # Setup: []
                # Cleanup: []
```
