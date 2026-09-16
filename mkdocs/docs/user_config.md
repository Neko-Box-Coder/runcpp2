# User Config Reference

### `DefaultPlatform`
- Description: Evaluates to the host platform.

## Config

!!! warning
    All command substitutions in this file are passed directly to the shell. Exercise caution when using variables or user-provided input in your build commands to prevent potential security vulnerabilities.

### `PreferredProfile`
- Type: [Platform Map](#platform-map){:target="_blank"} with `string`
- Optional: `false`
- Default: None
- Description: A profile to be used if not specified while building
??? example
    ```yaml
    PreferredProfile: 
        DefaultPlatform: "gcc"
        Windows: "msvc"
    ```

### `Profiles`
- Type: [`Profile[]`](#profile){:target="_blank"}
- Optional: `false`
- Default: None
- Description: List of compiler/linker profiles that instruct how to compile/link

### `Parameters`
- Type: [ParametersInfo](./build_settings.md#parametersinfo){:target="_blank"}
- Optional: `true`
- Default: None
- Description: See [ParametersInfo](./build_settings.md#parametersinfo){:target="_blank"}
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
- Type: [VariablesInfo](./build_settings.md#variablesinfo){:target="_blank"}
- Optional: `true`
- Default: None
- Description: See [VariablesInfo](./build_settings.md#variablesinfo){:target="_blank"}
??? example
    ```yaml
    Variables:
        VarName1: "Some string {Param1} substitution"
    ```

### `Import`
- Type: `string` or `string[]`
- Optioanl: `true`
- Default: None
- Description: Import other yaml files to merge to this file. Import can either be a single path or 
a list of paths. If there's any parameters/variables in the import file, it will applied to that file 
first before merging.
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

### `FileInfo`
- Type: `map`
- Description: Information of different file types
- Child Fields:
    - `Prefix`
        - Type: [Platform Map](#platform-map){:target="_blank"} with `string`
        - Optional: `false`
        - Default: None
        - Description: Prefix text of the file

    - `Extension`
        - Type: [Platform Map](#platform-map){:target="_blank"} with `string`
        - Optional: `false`
        - Default: None
        - Description: Extension text of the file (including .)
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

### `RunPartInfo`
- Type: `map`
- Description: Information of part of a command
- Child Fields:
    - `Type`
        - Type: `enum string`, can be one of the following:
            - `Once`: This part is only appended once
            - `Repeats`: This part is appended repeatedly
        - Optional: `false`
        - Default: None
        - Description: Type of the command part, whether it is repeating or not. If this is `Once`, then array variables/parameters are not allowed. This follows the substitution rule specified in [VariablesInfo](./build_settings.md#variablesinfo){:target="_blank"}.
    - `CommandPart`
        - Type: `string`
        - Optional: `false`
        - Default: None
        - Description: The content to be appended to the command string

??? example
    ```yaml
    Type: Once
    CommandPart: "{Stage.Executable} {Stage.LinkFlags} -o \"{Stage.Output.Directory}\
        {/}{Stage.Output.Name}\""
    ```
    ```yaml
    Type: Repeats
    CommandPart: " \"{Stage.Input.Path}\""
    ```

### `CommandInfo`
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
        - Type: [`RunPartInfo[]`](#runpartinfo){:target="_blank"}
        - Optional: `false`
        - Default: None
        - Description: The components for the command to be run
    - `ExpectedOutputFiles`
        - Type: `string[]`
        - Optional: `false`
        - Default: None
        - Description: The expected files after running this command
        ??? todo
            Actually use this...
??? example
    ```yaml
    Flags: "-FlagA -FlagB"
    Executable: "g++"
    RunParts:
    -   Type: Once
        CommandPart: "{Stage.Executable} -c {Stage.CompileFlags}"
    -   Type: Repeats
        CommandPart: " -I\"{Stage.IncludeDirectory.Path}\""
    -   Type: Once
        CommandPart: " \"{Stage.Input.Path}\" -o \"{Stage.Output.Directory}{/}\
            {Stage.ObjectLinkFile.Prefix}{Stage.Input.Name}{Stage.ObjectLinkFile.Extension}\""
    ExpectedOutputFiles: 
    -   "{Stage.Output.Directory}{/}{Stage.ObjectLinkFile.Prefix}{Stage.Input.Name}{Stage.ObjectLinkFile.Extension}"
    ```



### `Profile`

#### `Name`
- Type: `string`
- Optional: `false`
- Default: None
- Description: Name (case sensitive) of the profile that can be queried from the build info
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
- Type: [Platform Map](#platform-map){:target="_blank"} with `string[]`
- Optional: `true`
- Default: None
- Description: The commands to run in **shell** before calling the compiler/linker for each platform. This is run inside the root build directory.

#### `Cleanup`
- Type: [Platform Map](#platform-map){:target="_blank"} with `string[]`
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
        - Type: [FileInfo](#fileinfo){:target="_blank"}
        - Optional: `false`
        - Default: None
        - Description: The file properties for the files to be **linked** as object file for each platform
    
    - `SharedLinkFile`
        - Type: [FileInfo](#fileinfo){:target="_blank"}
        - Optional: `false`
        - Default: None
        - Description: The file properties for the files to be **linked** as shared libraries for each platform
    
    - `SharedLibraryFile`
        - Type: [FileInfo](#fileinfo){:target="_blank"}
        - Optional: `false`
        - Default: None
        - Description: The file properties for the files to be **copied** as shared libraries for each platform

    - `StaticLinkFile`
        - Type: [FileInfo](#fileinfo){:target="_blank"}
        - Optional: `false`
        - Default: None
        - Description: The file properties for the files to be linked as static libraries for each platform

    - `ExecutableFile`
        - Type: [FileInfo](#fileinfo){:target="_blank"}
        - Optional: `false`
        - Default: None
        - Description: The file properties for the files to be **copied** as executable for each platform

    - `DebugSymbolFile`
        - Type: [FileInfo](#fileinfo){:target="_blank"}
        - Optional: `true`
        - Default: None
        - Description: The file properties for debug symbols to be copied alongside the binary for each platform

#### `Import`
- Type: `string` or `string[]`
- Optioanl: `true`
- Default: None
- Description: See [Import](./build_settings.md#import){:target="_blank"}

#### `Parameters`
- Type: [ParametersInfo](./build_settings.md#parametersinfo){:target="_blank"}
- Optional: `true`
- Default: None
- Description: See [ParametersInfo](./build_settings.md#parametersinfo){:target="_blank"}
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
- Type: [VariablesInfo](./build_settings.md#variablesinfo){:target="_blank"}
- Optional: `true`
- Default: None
- Description: See [VariablesInfo](./build_settings.md#variablesinfo){:target="_blank"}
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
    - Type: [Platform Map](#platform-map){:target="_blank"} with `string`
    - Optional: `false`
    - Default: None
    - Description: Shell command to use for checking if the executable exists or not
    
    ##### `CompileTypes`
    ??? info
        Here are a list of built-in variables for RunParts, Setup and Cleanup

        **Constants**
        
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
        
        
        **Stage Info**
        
        - `{Stage.Executable}`: Compiler executable
        - `{Stage.CompileFlags}`: Compile flags from config and override
        
        
        **Input/Output Info**
        
        - `{Stage.Input.Name}`: Name of the current input source file (without directory path and extension)
        - `{Stage.Input.Extension}`: Extension of the current input source file
        - `{Stage.Input.Directory}`: Directory of the current input source file
        - `{Stage.Input.Path}`: Full path to the current input source file
        - `{Stage.Output.Directory}`: Directory of all the output files
        
        
        **Iterable variables, must be inside "Repeats" run type**
        
        - `{Stage.DefineNameOnly}`: All the defines without a value specified (equivalent to #define X)
        - `{Stage.DefineName}`: Name of all the defines that has a value specified
        - `{Stage.DefineValue}`: Value of all the defines that has a value specified (use together with {Stage.DefineName})
        - `{Stage.IncludeDirectory.Path}`: Path to all the include directories
            - `{Stage.IncludeDirectory.Source.Path}`: Path to source include directories, sub array
            - `{Stage.IncludeDirectory.Dep.Path}`: Path to dependencies include directories, sub array
    - Type: `map`
    - Optional: `false`
    - Default: None
    - Description: Compilation commands for different file types
    - Child Fields:
        - `Executable`
            - Type: [Platform Map](#platform-map){:target="_blank"} with [CommandInfo](#commandinfo){:target="_blank"}
            - Optional: `false`
            - Default: None
            - Description: Compilation commands for executable
        - `Static`
            - Type: [Platform Map](#platform-map){:target="_blank"} with [CommandInfo](#commandinfo){:target="_blank"}
            - Optional: `false`
            - Default: None
            - Description: Compilation commands for static library
        - `Shared`
            - Type: [Platform Map](#platform-map){:target="_blank"} with [CommandInfo](#commandinfo){:target="_blank"}
            - Optional: `false`
            - Default: None
            - Description: Compilation commands for shared library

#### `Linker`
- Type: `map`
- Optional: `false`
- Default: none
- Description: Linker settings, run once
- Child Fields:
    
    ##### `CheckExistence`
    - Type: [Platform Map](#platform-map){:target="_blank"} with `string`
    - Optional: `false`
    - Default: None
    - Description: Shell command to use for checking if the executable exists or not

    ##### `LinkTypes`
    ??? info
        Here are a list of built-in variables for RunParts, Setup and Cleanup

        **Constants**
        
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
        
        
        **Stage Info**
        
        - `{Stage.Executable}`: Linker executable
        - `{Stage.LinkFlags}`: Link flags from config and override
        
        
        **Output Info**
        
        - `{Stage.Output.Name}`: Name of the output file (without directory path and extension)
        - `{Stage.Output.Directory}`: Directory of all the output files
        
        **Iterable variables, must be inside "Repeats" run type**
        
        - `{Stage.Input.Name}`: Name of the files to be linked, regardless of the build type
            - `{Stage.Input.Dep.Name}`: Name of the dependencies files to be linked, regardless of the build type, sub array
            - `{Stage.Input.Source.Name}`: Name of the source files to be linked, regardless of the build type, sub array
            - `{Stage.Input.Object.Name}`: Name of the object files to be linked, sub array
                - `{Stage.Input.Dep.Object.Name}`: Name of the dependencies object files to be linked, Sub array
                - `{Stage.Input.Source.Object.Name}`: Name of the source object files to be linked, Sub array
            - `{Stage.Input.Shared.Name}`: Name of the shared dependencies files to be linked, sub array
            - `{Stage.Input.Static.Name}`: Name of the static dependencies files to be linked, sub array
        
        - `{Stage.Input.Extension}`: File Extensions of the files to be linked, regardless of the build type
            - `{Stage.Input.Dep.Extension}`: File Extensions of the dependencies files to be linked, regardless of the build type
            - `{Stage.Input.Source.Extension}`: File Extensions of the source files to be linked, regardless of the build type
            - `{Stage.Input.Object.Extension}`: File Extensions of the object files to be linked, sub array
                - `{Stage.Input.Dep.Object.Extension}`: File Extensions of the dependencies object files to be linked, sub array
                - `{Stage.Input.Source.Object.Extension}`: File Extensions of the source object files to be linked, sub array
            - `{Stage.Input.Shared.Extension}`: File Extensions of the shared dependencies files to be linked, sub array
            - `{Stage.Input.Static.Extension}`: File Extensions of the static dependencies files to be linked, sub array
        
        - `{Stage.Input.Directory}`: Directories of the files to be linked, regardless of the build type
            - `{Stage.Input.Dep.Directory}`: Directories of the dependencies files to be linked, regardless of the build type
            - `{Stage.Input.Source.Directory}`: Directories of the source files to be linked, regardless of the build type
            - `{Stage.Input.Object.Directory}`: Directories of the object files to be linked, sub array
                - `{Stage.Input.Dep.Object.Directory}`: Directories of the dependencies object files to be linked, sub array
                - `{Stage.Input.Source.Object.Directory}`: Directories of the source object files to be linked, sub array
            - `{Stage.Input.Shared.Directory}`: Directories of the shared dependencies files to be linked, sub array
            - `{Stage.Input.Static.Directory}`: Directories of the static dependencies files to be linked, sub array
        
        - `{Stage.Input.Path}`: Full paths to the files to be linked, regardless of the build type
            - `{Stage.Input.Dep.Path}`: Full paths to the dependencies files to be linked, regardless of the build type
            - `{Stage.Input.Source.Path}`: Full paths to the source files to be linked, regardless of the build type
            - `{Stage.Input.Object.Path}`: Full paths to the object files to be linked, sub array
                - `{Stage.Input.Dep.Object.Path}`: Full paths to the dependencies object files to be linked, sub array
                - `{Stage.Input.Source.Object.Path}`: Full paths to the source object files to be linked, sub array
            - `{Stage.Input.Shared.Path}`: Full paths to the shared dependencies files to be linked, sub array
            - `{Stage.Input.Static.Path}`: Full paths to the static dependencies files to be linked, sub array
    - Type: `map`
    - Optional: `false`
    - Default: None
    - Description: Link commands for different file types
    - Child Fields:
        - `Executable`
            - Type: [Platform Map](#platform-map){:target="_blank"} with [CommandInfo](#commandinfo){:target="_blank"}
            - Optional: `false`
            - Default: None
            - Description: Compilation commands for executable
        - `Static`
            - Type: [Platform Map](#platform-map){:target="_blank"} with [CommandInfo](#commandinfo){:target="_blank"}
            - Optional: `false`
            - Default: None
            - Description: Compilation commands for static library
        - `Shared`
            - Type: [Platform Map](#platform-map){:target="_blank"} with [CommandInfo](#commandinfo){:target="_blank"}
            - Optional: `false`
            - Default: None
            - Description: Compilation commands for shared library

