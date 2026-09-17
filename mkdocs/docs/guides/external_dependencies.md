# Build Info Intermediates

## Parameters & Variables

Build info can be changed dynamically using parameters and variables. 

Parameters and Variables are parsed first before parsing the rest of the fields, and they act like 
macros in C where it just performs text replacements (in the YAML nodes level) to the user supplied 
or default values.

The user input values or default values are first applied to parameters, with or without constraints 
optionally. Then a variable can "aggregate" one or more parameters values into a single variable 
which can be used for text replacement.

```yaml
Parameters:
    Param1:
        Optional: true
        Default: ""
        Array: false
        Constraint: None
    Param2:
        Optional: true
        Default: "UserDefine1,UserDefine2"
        Array: true
        Constraint: None

Variables:
    VarName1: "Some string {Param1} substitution"

Defines: ["ExampleDefine=\"{VarName1}\""]
```

The value of the parameter can be changed dynamically with the following syntax for different main 
actions.

`--parameters <name1=val1;name2=val2;...>`

For example

```shell
runcpp2 run --parameters Param1=parameter;Param2=UserDefineA,UserDefineB ./main.cpp
```

where the value of `ExampleDefine` would be `"Some string parameter substitution"`, also 
`UserDefineA` and `UserDefineB` would be defined too in this instance.

See [ParametersInfo](../build_settings.md#parametersinfo){:target="_blank"} and 
[VariablesInfo](../build_settings.md#variablesinfo) for details on all possible child fields.

---

## Adding Dependencies

runcpp2 supports any dependencies as it is invoking the compiler/linker toolchains directly. So as 
long as you know/can build the dependencies locally or have the prebuilt binaries, you can link 
against or include it fairly trivially.

Unless you are importing a standalone dependency YAML (which will be explained later), a dependency 
must have at least the following fields:

- **Name**: The name of the dependency
- **Platforms**: Supported host platforms that can build the dependency
- **Source**: Where to look for (and copy) the dependency
- **LibraryType**: The type of the library (`Static`, `Object`, `Shared`, `Header`)

So a minimum dependency in a build info could look like this

```yaml
Dependencies:
-   Name: spdlog
    Platforms: [ DefaultPlatorm ]
    Source:
        Git: 
            URL: https://github.com/gabime/spdlog.git
            Branch: "v1.17.0"
    LibraryType: Header
    IncludePaths: ["./include"]
```

There are more fields available for dependency. For more details, see 
[Dependency](../build_settings.md#dependency).

For `Source`, it can either be a `Git` source or a `Local` source, which has different child fields 
correspondingly. 

It is recommended to use `Git` source for _script like_ files or _small portable_ projects, and 
`Local` source for anything else where the `Path` just points to the root of the dependency. 

The reason is being `Git` source is to work with public git repositories, and not designed as a 
dependency management alternative. The goal of this is to provide a way of rapid prototyping or 
script like eco-system, not dependency/package management; Use `Local` source for that instead.

### Linking

When using a non-header dependency, you will need to specify the binary files to link against. This 
can be done by specifying the binary names to search for and the directories to search in.

Additionally, you can also specify the link options needed by this dependency when building your 
project.

For example

```yaml
Dependencies:
-   Name: "yaml-cpp (Shared)"
    Platforms: [ DefaultPlatorm ]
    Source:
        Git:
            URL: https://github.com/jbeder/yaml-cpp.git
            Branch: "yaml-cpp-0.9.0"
    LibraryType: Shared
    IncludePaths: ["./include"]
    LinkProperties:
        SearchLibraryNames: ["yaml-cpp"] # Find .lib/.dll/.so that contains the name "yaml-cpp"
        SearchDirectories: ["./build", "./build/Release"]
    Setup: # Build once on setup
        DefaultPlatform:
            DefaultProfile: 
            -   "mkdir build"
            -   "cd build && cmake .. -DYAML_BUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Release"
            -   "cd build && cmake --build . -j 4"
        Windows:
            msvc: 
            -   "mkdir build"
            -   "cd build && cmake .. -DYAML_BUILD_SHARED_LIBS=ON" 
            -   "cmake --build . -j 4 --config Release"
```

When linking against a shared library, runcpp2 will automatically copy the corresponding runtime 
library files (i.e. DLL) if they exist. 

??? info
    Many dependencies are using CMake as their build system. 
    
    To figure out what values go to each field in the dependency info (such as 
    `CompileProperties.Defines`), you can use the given cmake snippet at the end of the dependency's 
    root CMake script on a given target.
    
    ```cmake
    # https://stackoverflow.com/a/56738858
    if(NOT CMAKE_PROPERTY_LIST)
        execute_process(COMMAND cmake --help-property-list OUTPUT_VARIABLE CMAKE_PROPERTY_LIST)
        
        # Convert command output into a CMake list
        string(REGEX REPLACE ";" "\\\\;" CMAKE_PROPERTY_LIST "${CMAKE_PROPERTY_LIST}")
        string(REGEX REPLACE "\n" ";" CMAKE_PROPERTY_LIST "${CMAKE_PROPERTY_LIST}")
        list(REMOVE_DUPLICATES CMAKE_PROPERTY_LIST)
    endif()
        
    function(print_properties)
        message("CMAKE_PROPERTY_LIST = ${CMAKE_PROPERTY_LIST}")
    endfunction()
        
    function(print_target_properties target)
        if(NOT TARGET ${target})
          message(STATUS "There is no target named '${target}'")
          return()
        endif()

        foreach(property ${CMAKE_PROPERTY_LIST})
            string(REPLACE "<CONFIG>" "${CMAKE_BUILD_TYPE}" property ${property})

            # Fix https://stackoverflow.com/questions/32197663/how-can-i-remove-the-the-location-property-may-not-be-read-from-target-error-i
            if(property STREQUAL "LOCATION" OR property MATCHES "^LOCATION_" OR property MATCHES "_LOCATION$")
                continue()
            endif()

            get_property(was_set TARGET ${target} PROPERTY ${property} SET)
            if(was_set)
                get_target_property(value ${target} ${property})
                message("${target} ${property} = ${value}")
            endif()
        endforeach()
    endfunction()

    print_target_properties(<your target>)
    ```
    
    The properties you should be looking for would be ones with `INTERFACE_` prefix. 

### Copying Files

If you need to copy additional files from dependencies to the build folder, you can specify like so

```yaml
Dependencies:
-   Name: MyLibraryA
    FilesToCopy:
        Windows:
            DefaultProfile: ["binaries/Windows/ExternalBinaries.dll"]
        Linux:
            DefaultProfile: ["binaries/Linux/ExternalBinaries.dll"]
```

### Importing



You can separate dependency info into standalone dependency YAML files and 
import them into your project.

The standalone dependency YAML file is the same as a single dependency entry in the `Dependencies` section.

???+ example
    If you have:
    ```yaml title="main.yaml"
    # ... other fields ...
    Dependencies:
    -   Name: MyLibrary
        Platforms: [Windows, Linux, MacOS]
        Source:
            Git:
                URL: "https://github.com/MyUser/MyLibrary.git"
        LibraryType: Header
    # ... other fields ...
    ```
    Then the standalone dependency YAML file will look like this:
    ```yaml title="MyLibrary.yaml"
    Name: MyLibrary
    Platforms: [Windows, Linux, MacOS]
    Source:
        Git:
            URL: "https://github.com/MyUser/MyLibrary.git"
    LibraryType: Header
    ```

To import a standalone dependency YAML, use the `ImportPath` field under the `Source` section:

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

???+ example "Example of `build_info.yaml` in above cases:"
    ```yaml title="build_info.yaml"
    Name: MyLibrary
    Platforms: [Windows, Linux, MacOS]
    LibraryType: Static
    IncludePaths:
    -   "src/include"
    Build:
        DefaultPlatform:
            "g++":
            -   "cd .. && cmake -B build"
            -   "cd .. && cmake --build build"
    ```
