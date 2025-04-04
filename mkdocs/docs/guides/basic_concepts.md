# Basic Concepts

## Script File

A script file is your entry point source file (typically a .cpp file) that runcpp2 uses to build 
with the settings specified either:

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

---

## Platforms And Profiles

runcpp2 uses platforms and profiles to organize build settings. 

Platforms represent different host operating systems (not the target platform), while profiles 
represent different compilers toolchains and their configurations.

### List Of Platforms

runcpp2 supports the following platforms:

- Windows
- Linux
- MacOS
- Unix (applies to Linux and MacOS)

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
                Flags: "-O2 -Wall"
            "msvc":
                Flags: "/O2"
        Linux:
            "g++":
                Flags: "-O3"
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
                Flags: "-Wall"
            "g++":
                Flags: "-O2"
    ```
    When using g++, only `-O2` will be used, not `-Wall -O2`.
    When using any other profile, only `-Wall` will be used.

!!! info "This requires `latest` version"
    If you have a setting that **only** has DefaultPlatform and DefaultProfile, you can directly 
    specify the settings without listing it under DefaultPlatform and DefaultProfile.

    For example:
    ```yaml
    OverrideCompileFlags:
        Flags: "-Wall"
    ```
    is equivalent to:
    ```yaml
    OverrideCompileFlags:
        DefaultPlatform:
            DefaultProfile:
                Flags: "-Wall"
    ```
