# Build Settings

## Special Keywords

### `DefaultPlatform`
- Type: `string key`
- Description: Evaluates to the host platform.

### `DefaultProfile`
- Type: `string key`
- Description: Evaluates to the preferred profile the user has set in the config file.

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
    - `Objects`
- Optional: `true`
- Default: `Executable`
- Description: The type of output to build.
??? example
    ```yaml
    BuildType: Executable
    ```
### `RequiredProfiles`
- Type: `Platform Profile List`
- Optional: `true`
- Default: None
- Description: The profiles that are required for the script to be built. No profiles are required if this field is empty.
??? example
    ```yaml
    RequiredProfiles: 
        Windows: ["g++"]
        Linux: ["g++"]
        MacOS: ["g++"]
    ```
### `OverrideCompileFlags`
- Type: `Platform Profile Map`
- Optional: `true`
- Default: None
- Description: The compile flags to override for each platform and profile.
- Child Fields:
    - `Remove`
        - Type: `string`
        - Optional: `true`
        - Default: None
        - Description: The compile flags to remove for each platform and profile.
    - `Append`
        - Type: `string`
        - Optional: `true`
        - Default: None
        - Description: The compile flags to append for each platform and profile.
??? example
    ```yaml
    OverrideCompileFlags:
        DefaultPlatform:
            "g++":
                Remove: "-flagA -flagB"
                Append: "-flagC -flagD"
    ```
### `OverrideLinkFlags`
- Type: `Platform Profile Map` with child fields
- Optional: `true`
- Default: None
- Description: The link flags to override for each platform and profile.
- Child Fields:
    - `Remove`
        - Type: `string`
        - Optional: `true`
        - Default: None
        - Description: The link flags to remove for each platform and profile.
    - `Append`
        - Type: `string`
        - Optional: `true`
        - Default: None
        - Description: The link flags to append for each platform and profile.
??? example
    ```yaml
    OverrideLinkFlags:
        DefaultPlatform:
            "g++":
                Remove: "-flagA -flagB"
                Append: "-flagC -flagD"
    ```
### `OtherFilesToBeCompiled`
- Type: `Platform Profile Map` with `list` of `string`
- Optional: `true`
- Default: None
- Description: The source files to be compiled for each platform and profile.
??? example
    ```yaml
    OtherFilesToBeCompiled:
        DefaultPlatform:
            DefaultProfile:
            -   "./AnotherSourceFile.cpp"
    ```
### `IncludePaths`
- Type: `Platform Profile Map` with `list` of `string`
- Optional: `true`
- Default: None
- Description: The include paths to be used for each platform and profile.
??? example
    ```yaml
    IncludePaths:
        DefaultPlatform:
            DefaultProfile:
            -   "./include"
            -   "./src/include"
    ```
### `Defines`
- Type: `Platform Profile Map` with `list` of `string`
- Optional: `true`
- Default: None
- Description: The defines to be used for each platform and profile.
??? example
    ```yaml
    Defines:
        DefaultPlatform:
            DefaultProfile:
            -   "EXAMPLE_DEFINE"              # Define without a value
            -   "VERSION_MAJOR=1"             # Define with a value
    ```
### `Setup`
- Type: `Platform Profile Map` with `list` of `string`
- Optional: `true`
- Default: None
- Description: The setup commands to be used for each platform and profile.
??? example
    ```yaml
    Setup:
        DefaultPlatform:
            DefaultProfile:
            -   "echo Setting up script..."
    ```
### `PreBuild`
- Type: `Platform Profile Map` with `list` of `string`
- Optional: `true`
- Default: None
- Description: The pre-build commands to be used for each platform and profile.
??? example
    ```yaml
    PreBuild:
        DefaultPlatform:
            DefaultProfile:
            -   "echo Starting build..."
    ```
### `PostBuild`
- Type: `Platform Profile Map` with `list` of `string`
- Optional: `true`
- Default: None
- Description: The post-build commands to be used for each platform and profile.
??? example
    ```yaml
    PostBuild:
        DefaultPlatform:
            DefaultProfile:
            -   "echo Build completed..."
    ```
### `Cleanup`
- Type: `Platform Profile Map` with `list` of `string`
- Optional: `true`
- Default: None
- Description: The cleanup commands to be used for each platform and profile.
??? example
    ```yaml
    Cleanup:
        DefaultPlatform:
            DefaultProfile:
            -   "echo Cleaning up script..."
    ```
### `Dependencies`
- Type: `list` of `Dependency`
- Optional: `true`
- Default: None
- Description: The dependencies to be used for each platform and profile.
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
- Type: `Platforms Map With Profiles Map`
- Description: A map of platforms with a map of profiles. 

???+ Example
    ```yaml
    ExampleSettings:
        Windows:
            "g++":
                ExampleSubSetting: "ExampleValue"
        Linux:
            "g++":
                ExampleSubSetting: "ExampleValue2"
        MacOS:
            "g++":
                ExampleSubSetting: "ExampleValue3"
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
- Type: `Platforms Map With Profiles List`
- Description: A map of platforms with a list of profiles.

???+ Example
    ```yaml
    Windows: ["g++", "msvc"]
    Linux: ["g++"]
    MacOS: ["g++"]
    ```

### `Dependency`
- Type: `map`

    #### `Name`
    - Type: `string`
    - Optional: `false`, `true` only if `Source.ImportPath` is specified
    - Default: None
    - Description: The name of the dependency.

    #### `Platforms`
    - Type: `list` of `string`
    - Optional: `false`
    - Default: None
    - Description: The platforms to be used for the dependency.

    #### `Source`
    - Type: `map` with child fields
    - Optional: `false`
    - Default: None
    - Description: The source of the dependency.
        
        ##### `ImportPath`
        - Type: `string`
        - Optional: `true`
        - Default: None
        - Description: The path to the dependency configuration file from the root repository of `Git.URL` or `Local.Path`
        
        ##### `Git`
        - Type: `map` with child fields
        - Optional: `true` if `ImportPath` is specified or `Local` is specified
        - Default: None
        - Description: The git source of the dependency.
        
            ###### `URL`
            - Type: `string`
            - Optional: `false`
            - Default: None
            - Description: The url of the git repository.
            
            !!! info inline end "This requires `v0.3.0` version"
            ###### `Branch`
            - Type: `string`
            - Optional: `true`
            - Default: None
            - Description: Branch name or tag name.
            
            !!! info inline end "This requires `v0.3.0` version"
            ###### `FullHistory`
            - Type: `bool`
            - Optional: `true`
            - Default: `false`
            - Description: Checkout full git history or just the target commit.
            
            !!! info inline end "This requires `v0.3.0` version"
            ###### `SubmoduleInitType`
            - Type: `enum string`, can be one of the following:
                - `None`: Do not initialize submodules
                - `Shallow`: Initialize submodules with just the target commit
                - `Full`: Initialize submodules with full git history
            - Optional: `true`
            - Default: `Shallow`
            - Description: Initialization type for all the submodules recursively 
        
        ##### `Local`
        - Type: `map` with child fields
        - Optional: `true` if `ImportPath` is specified or `Git` is specified
        - Default: None
        - Description: The local source of the dependency.
            
            ###### `Path`
            - Type: `string`
            - Optional: `false`
            - Default: None
            - Description: The path to the local dependency.
            ###### `CopyMode`
            - Type: `enum string`, can be one of the following:
                - `Auto`
                - `Symlink`
                - `Hardlink`
                - `Copy`
            - Optional: `true`
            - Default: `Auto`
            - Description: The mode to use when copying files to the build directory.
    
    #### `LibraryType`
    - Type: `enum string`, can be one of the following:
        - `Static`
        - `Object`
        - `Shared`
        - `Header`
    - Optional: `true`, only if `Source.ImportPath` is specified
    - Default: None
    - Description: The type of this dependency

    #### `IncludePaths`
    - Type: `list` of `string`
    - Optional: `true`
    - Default: None
    - Description: The include paths to be used for the dependency.

    #### `LinkProperties`
    - Type: `map` with child fields
    - Optional: `true` if `LibraryType` is `Header` or `Source.ImportPath` is specified
    - Default: None
    - Description: The link properties to be used for the dependency.
    - Child Fields:
        - `SearchLibraryNames`
            - Type: `list` of `string`
            - Optional: `true`
            - Default: None
            - Description: The library names to be searched for when linking against the script.
        - `ExcludeLibraryNames`
            - Type: `list` of `string`
            - Optional: `true`
            - Default: None
            - Description: The library names to be excluded from being linked against the script.
        - `SearchDirectories`
            - Type: `list` of `string`
            - Optional: `true`
            - Default: None
            - Description: The directories to be searched for the dependency binaries.
        - `AdditionalLinkOptions`
            - Type: `list` of `string`
            - Optional: `true`
            - Default: None
            - Description: The additional link options to be used for the dependency.
    
    #### `Setup`
    - Type: `Platform Profile Map` with `list` of `string`
    - Optional: `true`
    - Default: None
    - Description: The setup commands to be used for the dependency.

    #### `Build`
    - Type: `Platform Profile Map` with `list` of `string`
    - Optional: `true`
    - Default: None
    - Description: The build commands to be used for the dependency.

    #### `Cleanup`
    - Type: `Platform Profile Map` with `list` of `string`
    - Optional: `true`
    - Default: None
    - Description: The cleanup commands to be used for the dependency.

    #### `FilesToCopy`
    - Type: `Platform Profile Map` with `list` of `string`
    - Optional: `true`
    - Default: None
    - Description: The files to be copied to the output directory for each platform and profile.

## Template

```yaml
# This is the template for specifying build settings.
# Many of the settings are passed directly to the shell.
# Be cautious when using user-provided input in your build commands to avoid potential security risks.
# Output from commands such as Setup or Cleanup won't be shown unless log level is set to info.
# If the default is not mentioned for a setting, it will be empty.

# Each of the platform dependent settings can be listed under
# - DefaultPlatform
# - Windows
# - Linux
# - MacOS
# - Unix

# You can find all the profiles in your config folder. 
# This can be found by running `runcpp2 --show-config-path`. 
# Specifying "DefaultProfile" in the profile name will allow any profiles 
#   and use the user's preferred one.

# (Optional) Whether to pass the script path as the second parameter when running. Default is false
PassScriptPath: false

# (Optional) Language of the script. Default is determined by file extension
Language: "c++"

# (Optional) The type of output to build. Default is Executable
# Supported types:
# - Executable: Build as executable that can be run
# - Static: Build as static library (.lib/.a)
# - Shared: Build as shared library (.dll/.so)
# - Objects: Only compile to object files without linking
BuildType: Executable

# TODO: Rename this
# (Optional) Allowed profiles for the script for each platform.
#            Any profiles will be used if none is specified for the platform. 
RequiredProfiles: 
    Windows: ["g++"]
    Linux: ["g++"]
    MacOS: ["g++"]

# (Optional) Override the default compile flags for each platform.
OverrideCompileFlags:
    # Target Platform
    DefaultPlatform:
        # Profile with the respective flags to override
        "g++":
            # (Optional) Flags to be removed from the default compile flags, separated by space
            Remove: ""
            
            # (Optional) Additional flags to be appended to the default compile flags, separated by space
            Append: ""
    
# (Optional) Override the default link flags for each platform.
OverrideLinkFlags:
    # Target Platform
    DefaultPlatform:
        # Profile with the respective flags to override
        "g++":
            # (Optional) Flags to be removed from the default link flags, separated by space
            Remove: ""
            
            # (Optional) Additional flags to be appended to the default link flags, 
            #            separated by space
            Append: ""

# (Optional) Other source files (relative to script file path) to be compiled.
OtherFilesToBeCompiled:
    # Target Platform
    DefaultPlatform:
        # Target Profile
        DefaultProfile:
        -   "./AnotherSourceFile.cpp"

# (Optional) Include paths (relative to script file path) for each platform and profile
IncludePaths:
    # Target Platform
    DefaultPlatform:
        # Target Profile
        DefaultProfile:
        -   "./include"
        -   "./src/include"

# (Optional) Define cross-compiler defines for each platform and profile.
#            Defines can be specified as just a name or as a name-value pair.
Defines:
    # Target Platform
    DefaultPlatform:
        # Profile name
        DefaultProfile:
        -   "EXAMPLE_DEFINE"              # Define without a value
        -   "VERSION_MAJOR=1"             # Define with a value

# (Optional) Setup commands are run once before the script is first built.
#            These commands are run at the script's location when no build directory exists.
Setup:
    # Target Platform
    DefaultPlatform:
        # Profile name
        DefaultProfile:
        # List of setup commands
        -   "echo Setting up script..."

# (Optional) PreBuild commands are run before each build.
#            These commands are run in the build directory before compilation starts.
PreBuild:
    # Target Platform
    DefaultPlatform:
        # Profile name
        DefaultProfile:
        -   "echo Starting build..."

# (Optional) PostBuild commands are run after each successful build.
#            These commands are run in the output directory where binaries are located.
PostBuild:
    # Target Platform
    DefaultPlatform:
        # Profile name
        DefaultProfile:
        -   "echo Build completed..."

# (Optional) Cleanup commands are run when using the --cleanup option.
#            These commands are run at the script's location before the build directory is removed.
Cleanup:
    # Target Platform
    DefaultPlatform:
        # Profile name
        DefaultProfile:
        -   "echo Cleaning up script..."

# (Optional) The list of dependencies needed by the script
Dependencies:
    # Dependency name
-   Name: MyLibrary
    
    # Supported platforms of the dependency
    Platforms: [Windows, Linux, MacOS]
    
    # Where to get and copy the dependency (Git, Local)
    # Either Git or Local can exist, not both
    Source:
        # (Optional) Import dependency configuration from a YAML file if this field exists
        #            All other fields (Name, Platforms, etc...) are not needed if this field exists
        #            For Git source: Path is relative to the git repository root
        #            For Local source: Path is relative to the path specified under `Local`
        #            If neither source exists, local source with root script directory is assumed.
        ImportPath: "config/dependency.yaml"
        
        # Dependency or import YAML file exists in a git server, and needs to be cloned to build directory
        Git:
            # Git repository URL
            URL: "https://github.com/MyUser/MyLibrary.git"
            
            # (Optional) Branch name or tag name
            #            Defaults to default branch on specified git repo if this is not specified
            # Branch: ""
            
            # (Optional) Checkout full git history or just the target commit. Defaults to false
            # FullHistory: false
            
            # (Optional) Initialization type for all the submodules recursively
            #            - "None": Don't initialize any submodules
            #            - "Shallow": Only checkout the target commit of all the submodules (default)
            #            - "Full": Checkout the full git history of all the submodules
            # SubmoduleInitType: "Shallow"
        
        # Dependency or import YAML file exists in local filesystem directory, 
        #   and needs to be copied to build directory
        Local:
            # Path to the library directory
            Path: "./libs/LocalLibrary"
            
            # (Optional) How to handle copying files to build directory
            # Values:
            #   - "Auto" (default): Try symlink first, then hardlink, then copy as fallback
            #   - "Symlink": Create symbolic links only, fail if not possible
            #   - "Hardlink": Create hard links only, fail if not possible
            #   - "Copy": Copy files to build directory
            CopyMode: "Auto"

    # Library Type (Static, Object, Shared, Header)
    LibraryType: Static
    
    # (Optional) Paths to be added to the include paths, relative to the dependency folder
    IncludePaths:
    -   "src/include"
    
    # (Optional if LibraryType is Header) Link properties of the dependency
    LinkProperties:
        # Properties for searching the library binary for each platform
        DefaultPlatform:
            # Profile-specific properties
            "g++":
                # The library names to be searched for when linking against the script. 
                # Binaries with linkable extension that contains one of the names will be linked
                SearchLibraryNames: ["MyLibrary"]
                
                # (Optional) The library names to be excluded from being searched.
                #            Works the same as SearchLibraryNames but will NOT be linked instead
                ExcludeLibraryNames: []
                
                # The path (relative to the dependency folder) to be searched for the dependency binaries
                SearchDirectories: ["./build"]
                
                # (Optional) Additional link flags for this dependency
                AdditionalLinkOptions: []
    
    # (Optional) Setup commands are run once when the dependency is populated
    Setup:
        # Target Platform
        DefaultPlatform:
            # Setup shell commands for the specified profile. 
            # Default commands are run in the dependency folder
            # You can also use "DefaultProfile" if all the compilers run the same setup commands
            "g++":
            -   "mkdir build"
            
    
    # (Optional) Build commands are run every time before the script is being built
    Build:
        # Target Platform
        DefaultPlatform:
            # Target Profile
            "g++":
            -   "cd build && cmake .."
            -   "cd build && cmake --build ."
    
    # (Optional) Cleanup commands are run when the reset option is present. Normally nothing needs
    #            to be done since the dependency folder will be removed automatically.
    Cleanup:
        # Target Platform
        Linux:
            # Target Profile
            "g++":
            -   "sudo apt purge MyLibrary"

    # (Optional) Files to be copied to next to output binary for each platform and profile
    FilesToCopy:
        # Target Platform
        DefaultPlatform:
            # Profile name
            DefaultProfile:
            # List of files to copy (relative to the dependency folder)
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
