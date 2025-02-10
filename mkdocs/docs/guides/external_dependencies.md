# External Dependencies

## Adding External Dependencies

runcpp2 supports external dependencies out of the box. 

You can specify the dependencies under the `Dependencies` section.

Each dependency must have the following fields, other fields are optional:

!!! note "Note: Dependencies that are imported (explained later) only need the `Source` field."
    

- **Name**: The name of the dependency
- **Platforms**: The platforms the dependency is supported on
- **Source**: The source of the dependency
- **LibraryType**: The type of the library (`Static`, `Object`, `Shared`, `Header`)

---

## Specifying Dependency Source

In order to use a dependency, it must be coming from somewhere.

This is configured under the `Source` section. We currently support 2 sources:

- **Git Repository**: The dependency is cloned from a git repository
- **Local Directory**: The dependency is copied from a local directory

???+ example
    ```yaml title="Git Dependency"
    Dependencies:
    -   Name: MyLibrary
        Platforms: [Windows, Linux, MacOS]
        Source:
            Git:
                URL: "https://github.com/MyUser/MyLibrary.git"
                
        LibraryType: Static
        IncludePaths:
        -   "include/MyLibrary"
    ```

    ```yaml title="Local Dependency"
    Dependencies:
    -   Name: LocalLibrary
        Platforms: [Windows, Linux, MacOS]
        Source:
            Local:
                Path: "./libs/LocalLibrary"
                # Optional, defaults to "Auto". Can be one of: Auto, Symlink, Hardlink, Copy
                CopyMode: "Auto"
        LibraryType: Static
        IncludePaths:
        -   "include/LocalLibrary"
    ```

---

## Specifying Git Clone Options

!!! info "This requires `latest` version"

You can specify the target branch/tag name and to clone whole git history or not with:
`Branch` and `FullHistory`. 

A normal clone without full history will be performed if none of these are specified.

???+ example
    ```yaml
    Dependencies:
    ...
        Source:
            Git:
                URL: "https://github.com/MyUser/MyLibrary.git"
                Branch: "SpecialBranch"
                FullHistory: true
    ...
    ```


---

## Adding Include Paths And Link Settings

### Include Paths

Include paths can be specified using the `IncludePaths` field. These paths are relative to the dependency's root directory:

???+ example
    ```yaml
    Dependencies:
    -   Name: MyLibrary
        # ... other fields ...
        IncludePaths:
        -   "include"              # MyLibrary/include
        -   "src/include"          # MyLibrary/src/include
        -   "external/json/single_include"
    ```

### Link Settings

For non-header libraries, you need to specify how to link against the library using `LinkProperties`:

???+ example "Basic Link Settings"
    ```yaml
    Dependencies:
    -   Name: MyLibrary
        LibraryType: Static
        LinkProperties:
            DefaultPlatform:
                "g++":
                    # Names to search for when looking for library files
                    SearchLibraryNames: ["MyLibrary"]
                    # Where to look for the library files
                    SearchDirectories: ["build"]
        # ... other fields ...
    ```

??? example "Platform-Specific Link Settings"
    ```yaml
    Dependencies:
    -   Name: MyLibrary
        LibraryType: Shared
        LinkProperties:
            Windows:
                "msvc":
                    SearchLibraryNames: ["MyLibrary"]
                    SearchDirectories: ["build/Release"]
                    # Additional linker flags
                    AdditionalLinkOptions: ["/SUBSYSTEM:WINDOWS"]
            Linux:
                "g++":
                    SearchLibraryNames: ["libMyLibrary"]
                    SearchDirectories: ["build"]
                    AdditionalLinkOptions: ["-pthread"]
        # ... other fields ...
    ```

!!! tip "Library Name Patterns"
    - The library name should be specified without extensions
    - For Windows: `MyLibrary` will match `MyLibrary.lib` and `MyLibrary.dll`
    - For Unix: `MyLibrary` will match `libMyLibrary.a` and `libMyLibrary.so`

### Excluding Libraries

Sometimes a dependency might have multiple library files, but you only want to link against specific ones. 

Use `ExcludeLibraryNames` to skip certain libraries:

???+ example
    ```yaml
    Dependencies:
    -   Name: MyLibrary
        LibraryType: Static
        LinkProperties:
            DefaultPlatform:
                "g++":
                    SearchLibraryNames: ["MyLibrary"]
                    # Don't link against debug or test libraries
                    ExcludeLibraryNames: ["MyLibrary-d", "MyLibrary-test"]
                    SearchDirectories: ["build"]
        # ... other fields ...
    ```

---

## Adding Setup, Build and Cleanup Commands

runcpp2 supports external dependencies with any build systems by allowing you to specify different command hooks similar to [command hooks in your project](building_project_sources.md#adding-command-hooks)

The only difference is that `PreBuild` and `PostBuild` hooks are replaced with `Build` hook which is run together when building your project source files.

??? example
    ```yaml
    Dependencies:
    -   Name: MyLibrary
        # ... other fields ...
        Setup:
            DefaultPlatform:
                "g++":
                    -   "mkdir build"
        Build:
            DefaultPlatform:
                "g++":
                    -   "cmake --build build"
        Cleanup:
            DefaultPlatform:
                "g++":
                    -   "rm -rf build"
    ```

---

## Copying Files

Sometimes dependencies need additional files (like DLLs, shaders, or assets) to be copied next to your executable. You can specify these files using the `FilesToCopy` field.

All paths are relative to the dependency's root directory. The files are copied to the output directory where the executable is located.

???+ example "Basic File Copying"
    ```yaml
    Dependencies:
    -   Name: MyLibrary
        # ... other fields ...
        FilesToCopy:
            DefaultPlatform:
                DefaultProfile:
                -   "assets/shaders/default.glsl"    # Copy shader file
                -   "data/config.json"               # Copy config file
    ```

??? example "Copying Platform-Specific Files"
    ```yaml
    Dependencies:
    -   Name: MyLibrary
        # ... other fields ...
        FilesToCopy:
            Windows:
                "msvc":
                -   "assets/fonts/windows.ttf"        # Windows-specific font
            Linux:
                "g++":
                -   "assets/fonts/linux.ttf"          # Linux-specific font
    ```

---

## Importing Dependency Info

You can separate dependency info into standalone YAML files and import them into your project.

The standalone YAML file is the same as a single dependency entry in the `Dependencies` section.

??? example
    If you have:
    ```yaml
    Dependencies:
    -   Name: MyLibrary
        Platforms: [Windows, Linux, MacOS]
        Source:
            Git:
                URL: "https://github.com/MyUser/MyLibrary.git"
        LibraryType: Header
    ```
    Then you can create a standalone YAML file:
    ```yaml
    Name: MyLibrary
    Platforms: [Windows, Linux, MacOS]
    Source:
        Git:
            URL: "https://github.com/MyUser/MyLibrary.git"
    LibraryType: Header
    ```

To import a dependency info, use the `ImportPath` field under the `Source` section:

Just like previously, you can import the dependency info from a git repository or a local directory.

When using `ImportPath`:

- For Git sources: `ImportPath` is relative to the git repository root
- For Local sources: `ImportPath` is relative to the `Path` specified under `Local`
- If neither Git nor Local source is specified, `ImportPath` is relative to the script directory

!!! note
    When using `ImportPath`, Any fields in the dependency entry are not needed and will be ignored.

???+ example "Importing from a Git Repository"
    ```text title="Remote Git Repository Structure"
    project/
    ├── src/
    │   └── (source files...)
    └── config/
        └── build_info.yaml
    ```

    ```yaml title="Build Settings In Your Project"
    Dependencies:
    -   Source:
            ImportPath: "config/build_info.yaml"
            Git:
                URL: "https://github.com/MyUser/MyLibrary.git"
    ```

???+ example "Importing from a Local Directory"
    ```text title="Local Directory Structure"
    project/
    ├── main.yaml
    ├── main.cpp
    ├── libs/
    │   └── LocalLibrary/
    │       ├── (source files...)
    │       └── config/
    │           └── build_info.yaml
    └── src/
        └── (source files...)
    ```

    ```yaml title="main.yaml"
    Dependencies:
    -   Source:
            ImportPath: "config/build_info.yaml"
            Local:
                # NOTE: This can be an absolute path
                Path: "./libs/LocalLibrary"
    ```



Example of a dependency configuration file (dependency.yaml):
```yaml
Name: ImportedLibrary
Platforms: [Windows, Linux, MacOS]
LibraryType: Static
IncludePaths:
-   "src/include"
Build:
    DefaultPlatform:
        "g++":
        -   "cmake -B build"
        -   "cmake --build build"
```
