# Build Info Reference

## Platforms And Profiles

Each of the platform dependent settings can be listed under

- DefaultPlatform
- Windows
- Linux
- MacOS
- Unix

You can find all the profiles in your config folder. This can be found by running `runcpp2 show-config-path`. 

### `DefaultPlatform`
- Description: Evaluates to the host platform.

### `DefaultProfile`
- Description: Allow any profiles and evaluates to the preferred profile the user has set in the config file when usable.

---

## Settings

### `PassScriptPath`
- Type: `bool`
- Optional: `true`
- Default: `false`
- Description: Whether to pass the script path as the second parameter when running in additional to the binary path.
??? example
    ```yaml
    PassScriptPath: false
    ```
### `Language`
- Type: `string`
- Optional: `true`
- Default: Determined by file extension
- Description: The language of the script.
??? example
    ```yaml
    Language: "c++"
    ```
### `BuildType`
- Type: `enum string`, can be one of the following:
    - `Executable`
    - `Static`
    - `Shared`
- Optional: `true`
- Default: `Executable`
- Description: The type of output to build.
??? example
    ```yaml
    BuildType: Executable
    ```

### `RequiredProfiles`
- Type: [Platform Profile List](#platform-profile-list){:target="_blank"}
- Optional: `true`
- Default: None
- Description: Allowed profiles for the script for each platform. Any profiles will be used if none is specified for the platform.
??? example
    ```yaml
    RequiredProfiles: 
        Windows: ["g++"]
        Linux: ["g++"]
        MacOS: ["g++"]
    ```
??? TODO
    Rename this

### `Import`
- Type: `string` or `string[]`
- Optioanl: `true`
- Default: None
- Description: Import other yaml files to merge to this file. Import can either be a single path or a list of paths. If there's any parameter/variables in the import file, it will applied to that file first before merging
??? example
    ```yaml
    Import: "./OtherDefines.yaml"
    ```

### `Parameters`
- Type: [ParametersInfo](#parametersinfo){:target="_blank"}
- Optional: `true`
- Default: None
- Description: See [ParametersInfo](#parametersinfo){:target="_blank"}
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
- Type: [VariablesInfo](#variablesinfo){:target="_blank"}
- Optional: `true`
- Default: None
- Description: See [VariablesInfo](#variablesinfo){:target="_blank"}
??? example
    ```yaml
    Variables:
        VarName1: "Some string {Param1} substitution"
    ```


### `OverrideCompileFlags`
- Type: [Platform Profile Map](#platform-profile-map){:target="_blank"} with child fields
- Optional: `true`
- Default: None
- Description: Override the default compile flags for each platform.
- Child Fields:
    - `Remove`
        - Type: `string`
        - Optional: `true`
        - Default: None
        - Description: Flags to be removed from the default compile flags, separated by space
    - `Append`
        - Type: `string`
        - Optional: `true`
        - Default: None
        - Description: Additional flags to be appended to the default compile flags, separated by space
??? example
    ```yaml
    OverrideCompileFlags:
        DefaultPlatform:
            "g++":
                Remove: "-flagA -flagB"
                Append: "-flagC -flagD"
    ```

### `OverrideLinkFlags`
- Type: [Platform Profile Map](#platform-profile-map){:target="_blank"} with child fields
- Optional: `true`
- Default: None
- Description: Override the default link flags for each platform.
- Child Fields:
    - `Remove`
        - Type: `string`
        - Optional: `true`
        - Default: None
        - Description: Flags to be removed from the default link flags, separated by space
    - `Append`
        - Type: `string`
        - Optional: `true`
        - Default: None
        - Description: Additional flags to be appended to the default link flags, separated by space
??? example
    ```yaml
    OverrideLinkFlags:
        DefaultPlatform:
            "g++":
                Remove: "-flagA -flagB"
                Append: "-flagC -flagD"
    ```

### `SourceFiles`
- Type: [Platform Profile Map](#platform-profile-map){:target="_blank"} with `string[]`
- Optional: `true`
- Default: None
- Description: Other source files (relative to script file path) to be compiled.
??? example
    ```yaml
    SourceFiles:
        DefaultPlatform:
            DefaultProfile:
            -   "./AnotherSourceFile.cpp"
    ```

### `IncludePaths`
- Type: [Platform Profile Map](#platform-profile-map){:target="_blank"} with `string[]`
- Optional: `true`
- Default: None
- Description: Include paths (relative to script file path) for each platform and profile
??? example
    ```yaml
    IncludePaths:
        DefaultPlatform:
            DefaultProfile:
            -   "./include"
            -   "./src/include"
    ```

### `Defines`
- Type: [Platform Profile Map](#platform-profile-map){:target="_blank"} with `string[]`
- Optional: `true`
- Default: None
- Description: Defines for each platform and profile. Defines can be specified as just a name or as a name-value pair.
??? example
    ```yaml
    Defines:
        DefaultPlatform:
            DefaultProfile:
            -   "EXAMPLE_DEFINE"              # Define without a value
            -   "VERSION_MAJOR=1"             # Define with a value
    ```

### `Setup`
- Type: [Platform Profile Map](#platform-profile-map){:target="_blank"} with `string[]`
- Optional: `true`
- Default: None
- Description: Setup commands are run once before the script is first built. These commands are run at the script's location when no build directory exists.
??? example
    ```yaml
    Setup:
        DefaultPlatform:
            DefaultProfile:
            -   "echo Setting up script..."
    ```

### `PreBuild`
- Type: [Platform Profile Map](#platform-profile-map){:target="_blank"} with `string[]`
- Optional: `true`
- Default: None
- Description: PreBuild commands are run before each build. These commands are run in the build directory before compilation starts.
??? example
    ```yaml
    PreBuild:
        DefaultPlatform:
            DefaultProfile:
            -   "echo Starting build..."
    ```

### `PostBuild`
- Type: [Platform Profile Map](#platform-profile-map){:target="_blank"} with `string[]`
- Optional: `true`
- Default: None
- Description: PostBuild commands are run after each successful build. These commands are run in the output directory where binaries are located.
??? example
    ```yaml
    PostBuild:
        DefaultPlatform:
            DefaultProfile:
            -   "echo Build completed..."
    ```

### `Cleanup`
- Type: [Platform Profile Map](#platform-profile-map){:target="_blank"} with `string[]`
- Optional: `true`
- Default: None
- Description: The cleanup commands used for cleaning up for each platform and profile. This runs when `runcpp2 reset ...` is used.
??? example
    ```yaml
    Cleanup:
        DefaultPlatform:
            DefaultProfile:
            -   "echo Cleaning up script..."
    ```

### `Dependencies`
- Type: `Dependency[]`
- Optional: `true`
- Default: None
- Description: The list of dependencies needed by the script. See [Dependency](#dependency){:target="_blank"}
??? example
    ```yaml
    Dependencies:
    -   Name: MyLibrary
        Platforms: [Windows, Linux, MacOS]
        Source:
            # ImportPath: "config/dependency.yaml"
            Git:
                URL: "https://github.com/MyUser/MyLibrary.git"
            # Local:
            #     Path: "./libs/LocalLibrary"
            #     CopyMode: "Auto"
        LibraryType: Static
        IncludePaths:
        -   "src/include"
        LinkProperties:
            DefaultPlatform:
                "g++":
                    SearchLibraryNames: ["MyLibrary"]
                    ExcludeLibraryNames: []
                    SearchDirectories: ["./build"]
                    AdditionalLinkOptions: []
        CompileProperties:
            Defines: []
            AdditionalCompileOptions: []
        Setup:
            DefaultPlatform:
                "g++":
                -   "mkdir build"
        Build:
            DefaultPlatform:
                "g++":
                -   "cd build && cmake .."
                -   "cd build && cmake --build ."
        Cleanup:
            Linux:
                "g++":
                -   "sudo apt purge MyLibrary"
        FilesToCopy:
            DefaultPlatform:
                DefaultProfile:
                -  "assets/textures/sprite.png"
            Windows:
                "msvc":
                -  "assets/textures/sprite.png"
                -  "assets/fonts/windows_specific_font.ttf"
            Linux:
                "g++":
                -  "assets/textures/sprite.png"
                -  "assets/shaders/linux_optimized_shader.glsl"
    ```

## Special Types

### `Platform Profile Map`
- Description: A map of platforms with a map of profiles. 

???+ Example
    ```yaml
    ExampleSettings:
        Windows:
            "g++":
                ...
        Linux:
            "g++":
                ...
        MacOS:
            "g++":
                ...
    ```

If platform and profile are not specified, the default platform and profile are used. So
    
!!! info inline end "This requires `v0.3.0` version"
```yaml
ExampleSettings:
    ExampleSubSetting: "ExampleValue"
```

!!! info inline end ""

is the same as

!!! info inline end ""
```yaml
ExampleSettings:
    DefaultPlatform:
        DefaultProfile:
            ExampleSubSetting: "ExampleValue"
```

### `Platform Profile List`
- Description: A map of platforms with a list of profiles.

???+ Example
    ```yaml
    Windows: ["g++", "msvc"]
    Linux: ["g++"]
    MacOS: ["g++"]
    ```

### `ParametersInfo`
- Type: `map`
- Optional: `true`
- Default: None
- Description: Parameters are user supplied input that can be substituted into any keys or values, with the syntax of `{<parameter name>}`. This is only applied to the same file.
- `map` Key: Name of the parameter
- Child Fields:
    - `Optional`
        - Type: `bool`
        - Optional: `true`
        - Default: `true`
        - Description: If this parameter is mandatory
    - `Default`
        - Type: `string`
        - Optional: `true`
        - Default: Empty String
        - Description: Default value of the parameter
    - `Array`
        - Type: `bool`
        - Optional: `true`
        - Default: `false`
        - Description: Parameter for array. If this is true, comma separated values is expected. If this is true, this parameter can only be used in config values that expect an array.
    - `Constraint`
        - Type: `string` or `string[]`
        - Optional: `true`
        - Default: `"None"`
        - Description: Constraint of this parameter value. An array means a multiple choice constraint. Can be one of the following:
            - `"None"`: No constraint
            - `"Bool"`: `true`, `false`, `1` or `0`
            - `"Float[:<Min Float>,<Max Float>]"`: Floating point number with optional inclusive min and max
            ??? example
                ```yaml
                Parameters:
                    Param1:
                        Constraint: "Float:0.1,0.5"
                ```
            - `"Int[:<Min Int>,<Max Int>]"`: Integer number  with optional inclusive min and max
            ??? example
                ```yaml
                Parameters:
                    Param1:
                        Constraint: "Int:1,5"
                ```
            - `["<Choice 1>[:Mapped Value 1]", "<Choice 2>[:Mapped Value 2]", ...]`: List of choices with optional corresponding mapped values
            ??? example
                ```yaml
                Parameters:
                    Param1:
                        Constraint: ["A:A_Value", "B:B_Value", "C:C_Value", "All:A_Value B_Value C_Value"]
                ```
??? example
    ```yaml
    Parameters:
        Param1:
            Optional: true
            Default: ""
            Array: false
            Constraint: "None"
    ```

### `VariablesInfo`
- Type: `map`
- Optional: `true`
- Default: None
- Description: Variables can be substituted into any keys or values (excluding "Parameters"), with the syntax of `{<variable name>}`. This is only applied to the same file.
- `map` Key: Name of the variable
- Child Fields:
    - `map` Value
        - Type: `string` or `string[]`
        - Optional: `false`
        - Default: `""`
        - Description: Variables where the map key is the variable name and the map value is the variable value, where parameters will be substituted when using syntax of `{<parameter name>}`.
            - If this contains an array parameter value, then this will become an array variable and can only be used in config values that expect an array; where the content of this variable will be repeated with the corresponding array parameter value.
            - If there are multiple array parameters present, they _must_ have the same length. 
            - If a mixture of array and non array parameters are present, then the same non array parameters will be substituted for each iteration.
            - To escape '{' and '}' to avoid substitutioon, simply repeat the '{' or '}' character again. So to escape `"${MyBashVariable}"`, it will become `"${{MyBashVariable}}"` 
??? example
    ```yaml
    # If {Param1} is "value", then {VarName1} will become "Some string value substitution"
    Variables:
        VarName1: "Some string {Param1} substitution"
    ```
    ```yaml
    # If {ParamArray} is "1,2,3" and {ParamConstant} is "a", then {VarName1} will become ["1 a", "2 a", "3 a"]
    Variables:
        VarName1: "{ParamArray} {ParamConstant}"
    ```
??? todo
    Conditional variables


### `Dependency`
- Type: `map`

    #### `Name`
    - Type: `string`
    - Optional: `false`, `true` only if `Source.ImportPath` is specified
    - Default: None
    - Description: Dependency name

    #### `Platforms`
    - Type: `string[]`
    - Optional: `false`
    - Default: None
    - Description: Supported platforms for the dependency

    #### `Source`
    - Type: `map` with child fields
    - Optional: `false`
    - Default: None
    - Description: Where to get and copy the dependency
    - Child Fields: 
        - `ImportPath`
            - Type: `string`
            - Optional: `true`
            - Default: None
            - Description: Import dependency configuration from a YAML file if this field exists. 
            All other fields in `Dependency` (Name, Platforms, etc...) are not needed if this 
            field exists. 
                - For Git source: Path is relative to the git repository root. 
                - For Local source: Path is relative to the path specified under `Local`. 
                - If neither source exists, local source with root script directory is assumed.
        - `Git`
            - Type: `map` with child fields
            - Optional: `true` if `ImportPath` is specified or `Local` is specified
            - Default: None
            - Description: Dependency or import YAML file exists in a git server, and needs to be cloned to build directory
            - Child Fields: 
                - `URL`
                    - Type: `string`
                    - Optional: `false`
                    - Default: None
                    - Description: Git repository URL
                
                !!! info inline end "This requires `v0.3.0` version"
                - `Branch`
                    - Type: `string`
                    - Optional: `true`
                    - Default: Default branch on specified git repo
                    - Description: Branch name or tag name
                
                !!! info inline end "This requires `v0.3.0` version"
                - `FullHistory`
                    - Type: `bool`
                    - Optional: `true`
                    - Default: `false`
                    - Description: Checkout full git history or just the target commit.
                
                !!! info inline end "This requires `v0.3.0` version"
                - `SubmoduleInitType`
                    - Type: `enum string`, can be one of the following:
                        - `None`: Don't initialize submodules
                        - `Shallow`: Only checkout the target commit of all the submodules
                        - `Full`: Checkout the full git history of all the submodules
                    - Optional: `true`
                    - Default: `Shallow`
                    - Description: Initialization type for all the submodules recursively 
        
        - `Local`
            - Type: `map` with child fields
            - Optional: `true` if `ImportPath` is specified or `Git` is specified
            - Default: None
            - Description: Dependency or import YAML file exists in local filesystem directory, and needs to be copied to build directory
            - Child Fields: 
                - `Path`
                    - Type: `string`
                    - Optional: `false`
                    - Default: None
                    - Description: Path to the library directory
                
                - `CopyMode`
                    - Type: `enum string`, can be one of the following:
                        - `Auto`: Try symlink first, then hardlink, then copy as fallback
                        - `Symlink`: Create symbolic links only, fail if not possible
                        - `Hardlink`: Create hard links only, fail if not possible
                        - `Copy`: Copy files to build directory
                    - Optional: `true`
                    - Default: `Auto`
                    - Description: How to handle copying files to build directory
    
    #### `Parameters`
    - Type: [ParametersInfo](#parametersinfo){:target="_blank"}
    - Optional: `true`
    - Default: None
    - Description: See [ParametersInfo](#parametersinfo){:target="_blank"}

    #### `Variables`
    - Type: [VariablesInfo](#variablesinfo){:target="_blank"}
    - Optional: `true`
    - Default: None
    - Description: See [VariablesInfo](#variablesinfo){:target="_blank"}

    #### `LibraryType`
    - Type: `enum string`, can be one of the following:
        - `Static`
        - `Object`
        - `Shared`
        - `Header`
    - Optional: `true`, only if `Source.ImportPath` is specified
    - Default: None
    - Description: Library Type

    #### `IncludePaths`
    - Type: `string[]`
    - Optional: `true`
    - Default: None
    - Description: Paths to be added to the include paths, relative to the dependency folder

    #### `LinkProperties`
    - Type: `map`
    - Optional: `true` if `LibraryType` is `Header` or `Source.ImportPath` is specified
    - Default: None
    - Description: Link properties of the dependency
    - Child Fields:
        - `SearchLibraryNames`
            - Type: `string[]`
            - Optional: `true`
            - Default: None
            - Description: The library names to be searched for when linking against the script. Binaries with linkable extension that contains one of the names will be linked
        - `ExcludeLibraryNames`
            - Type: `string[]`
            - Optional: `true`
            - Default: None
            - Description: The library names to be excluded from being searched. Works the same as SearchLibraryNames but will NOT be linked instead
        - `SearchDirectories`
            - Type: `string[]`
            - Optional: `true`
            - Default: None
            - Description: The path (relative to the dependency folder) to be searched for the dependency binaries
        - `AdditionalLinkOptions`
            - Type: `string[]`
            - Optional: `true`
            - Default: None
            - Description: Additional link flags for this dependency
    
    #### `CompileProperties`
    - Type: `map`
    - Optional: `true`
    - Default: None
    - Description: Compile properties of the dependency
    - Child Fields:
        - `Defines`
            - Type: `string[]`
            - Optional: `true`
            - Default: None
            - Description: Additional defines for this dependency when compiling source
        - `AdditionalCompileOptions`
            - Type: `string[]`
            - Optional: `true`
            - Default: None
            - Description: Additional compile flags for this dependency when compiling source
    
    #### `Setup`
    - Type: [Platform Profile Map](#platform-profile-map){:target="_blank"} with `string[]`
    - Optional: `true`
    - Default: None
    - Description: Setup commands are run once when the dependency is populated
    ??? example
        ```yaml
        Setup:
            Linux:
                "g++": ["mkdir build"]
        ```

    #### `Build`
    - Type: [Platform Profile Map](#platform-profile-map){:target="_blank"} with `string[]`
    - Optional: `true`
    - Default: None
    - Description: Build commands are run every time before the script is being built

    #### `Cleanup`
    - Type: [Platform Profile Map](#platform-profile-map){:target="_blank"} with `string[]`
    - Optional: `true`
    - Default: None
    - Description: Cleanup commands are run when reset is performed. Normally nothing needs to be done since the dependency folder will be removed automatically.

    #### `FilesToCopy`
    - Type: [Platform Profile Map](#platform-profile-map){:target="_blank"} with `string[]`
    - Optional: `true`
    - Default: None
    - Description: Files to be copied to next to output binary for each platform and profile

## Template
```yaml
# See https://neko-box-coder.github.io/runcpp2/latest/build_settings/ for full reference
# The following are example values for each field

# PassScriptPath: false

# Language: "c++"

# BuildType: Executable # `Executable`, `Static`, `Shared`

# RequiredProfiles:
#     Windows: ["g++"]
#     Linux: ["g++"]
#     MacOS: ["g++"]

# Import: "./OtherDefines.yaml"

# Parameters:
#     Param1:
#         Optional: true
#         Default: ""
#         Array: false
#         Constraint: "None"

# Variables:
#     VarName1: "Some string {Param1} substitution"

# OverrideCompileFlags:
#     Remove: ""
#     Append: ""

# OverrideLinkFlags:
#     Remove: ""
#     Append: ""

# SourceFiles: ["./AnotherSourceFile.cpp"]

# IncludePaths: ["./include", "./src/include"]

# Defines: ["EXAMPLE_DEFINE", "VERSION_MAJOR=1"]

# Setup: ["echo Setting up script..."]

# PreBuild: ["echo Starting build..."]

# PostBuild: ["echo Build completed..."]

# Cleanup: ["echo Cleaning up script..."]

# Dependencies:
# -   Name: MyLibrary
#     Platforms: [DefaultPlatform]
#     Source: # Either Git or Local, not both
#         ImportPath: "config/dependency.yaml"
#         Git:
#             URL: "https://github.com/MyUser/MyLibrary.git"
#             Branch: ""
#             FullHistory: false
#             SubmoduleInitType: "Shallow" # None, Shallow, Full
#         Local:
#             Path: "./libs/LocalLibrary"
#             CopyMode: "Auto" # Auto, Symlink, Hardlink, Copy
#     Parameters:
#         Param1:
#             Optional: true
#             Default: ""
#             Array: false
#             Constraint: "None"
#     Variables:
#         VarName1: "Some string {Param1} substitution"
#     LibraryType: Static # Static, Object, Shared, Header
#     IncludePaths: ["src/include"]
#     LinkProperties:
#         SearchLibraryNames: ["MyLibrary"]
#         ExcludeLibraryNames: []
#         SearchDirectories: ["./build"]
#         AdditionalLinkOptions: []
#     CompileProperties:
#         Defines: []
#         AdditionalCompileOptions: []
#     Setup: ["mkdir build"]
#     Build: ["cd build && cmake ..", "cd build && cmake --build ."]
#     Cleanup: ["sudo apt purge MyLibrary"]
#     FilesToCopy: ["assets/textures/sprite.png"]
```
