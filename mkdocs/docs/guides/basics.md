# Basics

## Script File

A script file is your entry point source file (typically a .cpp file) that runcpp2 uses to build 
with the config specified either:

- As inline comments in the source file:
  ```cpp
  /*runcpp2
  RequiredProfiles:
      Windows: ["msvc"]
      Unix: ["g++"]
  */
  int main() { return 0; }
  ```

- Or as a separate YAML file with the same name:
  ```
  script.cpp    # Your source file
  script.yaml   # Your build settings
  ```

The name of the final output will be the name of the script file, therefore a script file will 
always have a 1 to 1 relationship with the linker output, even if multiple sources are specified in 
the script file build settings. 

You can use any of your source files as a script file, or a dedicated .cpp file for building.

Suppose you have a c++ file called `script.cpp`, you can run it immediately by doing 

```shell
runcpp2 ./script.cpp <any arguments>
```

??? example
    ```cpp title="script.cpp"
    #include <iostream>
    int main(int argc, char** argv)
    {
        if(argc != 2)
        {
            std::cout << "Usage: runcpp2 ./script.cpp <Name>"
            return 1;
        }
        
        std::cout << "Hello " << argv[1] << std::endl;
        return 0;
    }
    ```

!!! note
    On Unix, if you have added runcpp2 to your PATH and add this line `//bin/true;runcpp2 "$0" "$@"; exit $?;` 
    to the top of your script, you can run the script directly by `./script.cpp <arguments>`
    
    ??? example
        ```cpp title="script.cpp"
        //bin/true;runcpp2 "$0" "$@"; exit $?;
        #include <iostream>
        int main(int, char**) { std::cout << "Hello World" << std::endl; }
        ```

---

## Error Feedback

If you want to edit the script but want to have feedback for any error, you can use "watch" mode.

```shell title="shell"
runcpp2 --watch ./script.cpp
```

---

## Spcifying Build Config

Build config such as compile/link flags, external dependencies, command hooks, etc.
can be spcified inlined inside a source file or as a separate yaml file in the format of YAML

- To specify build config in a dedicated yaml file:
    - The yaml file in the same directory and share the same as the source file being run will be used
- To specify inline build config inside a source file: 
    - Put them inside a comment with `runcpp2` at the beginning of the build config
    - The inline build config can exist in anywhere of the source file
    - Both inline (but continuous) comments (`#!cpp //`) and block comments are supported (`#!cpp /* */`)

??? example "Example Inline Build Config"
    ```cpp title="script.cpp"
    /*runcpp2
    OverrideCompileFlags:
        DefaultPlatform:
            "g++":
                Append: "-Wfloat-equal -Wextra"
    */
    int main(int, char**) { float a = 1.f; float b = 1.f; return a == b ? 0 : 1; }
    ```
    ```shell title="shell"
    runcpp2 script.cpp
    ```

??? example "Example Dedicated Build Config"
    ```yaml title="script.yaml"
    OverrideCompileFlags:
        DefaultPlatform:
            "g++":
                Append: "-Wfloat-equal -Wextra"
    ```
    ```cpp title="script.cpp"
    int main(int, char**) { float a = 1.f; float b = 1.f; return a == b ? 0 : 1; }
    ```
    ```shell title="shell"
    runcpp2 script.cpp
    ```

For a complete list of build settings, see [Build Settings](build_settings.md) or generate the template with
```shell
runcpp2 --create-script-template ./script.cpp   # Embeds the build settings template as comment
runcpp2 --create-script-template ./script.yaml  # Creates the build settings template as dedicated yaml file
runcpp2 -t ./script.cpp                         # Short form
```

---

## Platforms And Profiles

runcpp2 uses platforms and profiles to organize build settings. 

A platform represents a single host operating systems (not the target platform).

A profile represents a single configuration of compiler/linker toolchain.

### List Of Platforms

runcpp2 supports the following platforms:

- Windows
- Linux
- MacOS
- Unix (Linux and MacOS)

??? TODO
    Custom platforms

### Default Profiles

The default user configuration includes two compiler profiles:

- **g++**: GNU c++ compiler (with alias "mingw")
- **vs2022_v17+**: Visual Studio 2022 compiler (with aliases "msvc1930+", "msvc")

### Specifying Platform/Profile Dependent Settings

Most build settings in runcpp2 follow this structure:
```yaml
<Setting Name>:
    <Platform A>:
        <Profile A>:
            ...
    <Platform B>:
        <Profile B>:
            ...
```

???+ example
    ```yaml
    OverrideCompileFlags:
        Windows:
            "g++":
                Append: "-O2 -Wall"
            "msvc":
                Append: "/O2"
        Linux:
            "g++":
                Append: "-O3"
    ```

There are two special keywords for more flexible configuration:

- **DefaultPlatform**: Settings that apply to any platform that doesn't have explicit settings
- **DefaultProfile**: Settings that apply to any profile that doesn't have explicit settings

!!! important
    DefaultPlatform and DefaultProfile settings are not additive. For example:
    ```yaml
    OverrideCompileFlags:
        DefaultPlatform:
            DefaultProfile:
                Append: "-Wall"
            "g++":
                Append: "-O2"
    ```
    When using g++, only `-O2` will be used, not `-Wall -O2`.
    When using any other profile, only `-Wall` will be used.

!!! info inline end "This requires `v0.3.0` version"

If you have a setting that **only** has DefaultPlatform and DefaultProfile, you can directly 
specify the settings without listing it under DefaultPlatform and DefaultProfile.

!!! info inline end ""

For example:

!!! info inline end ""

```yaml
OverrideCompileFlags:
    Append: "-Wall"
```

!!! info inline end ""
is equivalent to:

!!! info inline end ""

```yaml
OverrideCompileFlags:
    DefaultPlatform:
        DefaultProfile:
            Append: "-Wall"
```
