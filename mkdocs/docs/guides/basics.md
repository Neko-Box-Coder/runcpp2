# Basics

## Main File

A main file is the file that runcpp2 uses. It can either be a standalone build info yaml file or a 
source file (which contains build info yaml as inline comment, optionally).

To embedding build info as an inline comment, simply put `runcpp2` in the first line, whether it is 
a block comment or line comment (with or without space).

???+ example
    ```cpp
    /*runcpp2
    RequiredProfiles:
        Windows: ["msvc"]
        Unix: ["g++"]
    */
    int main() { return 0; }
    ```

A single main file can only produce a single binary output, with the same name as the main file.

Suppose you have a c++ file called `main.cpp`, you can run it immediately by doing 

```shell
runcpp2 run ./main.cpp <Arguments>
```

!!! note
    On Unix, if you have added runcpp2 to your PATH and add this line 
    `//bin/true;runcpp2 run "$0" "$@"; exit $?;` 
    to the top of your script, you can run the script directly by `./main.cpp <arguments>`
    
    ??? example
        ```cpp title="main.cpp"
        //bin/true;runcpp2 run "$0" "$@"; exit $?;
        #include <iostream>
        int main(int, char**) { std::cout << "Hello World" << std::endl; }
        ```

For full reference on all the available build info options, see 
[Build Info Reference](../build_settings.md){:target="_blank"}

A build info template can also be generated with the following command
```shell
runcpp2 template ./main.cpp   # Embeds the build settings template as comment
runcpp2 template ./main.yaml  # Creates the build settings template as dedicated yaml file
```

---

## Error Feedback

If you want a live error feedback, you can use "watch" mode.

```shell title="shell"
runcpp2 watch ./main.cpp
```

---

## Platforms And Profiles

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

Below are the built-in profiles:

- **g++**: GNU c++ compiler (with alias "mingw")
- **vs2022_v17+**: Visual Studio 2022 compiler (with aliases "msvc1930+", "msvc")
- **clang++**
- **clang**

Custom profiles can be added by editing the user config file. For full reference, see 
[User Config Reference](../user_config.md){:target="_blank"}

### Specifying Build Info Values

You can specify a value that applies to all platforms and profiles like so

```yaml
Defines: ["MyDefine=1"]
```

However, you can also specify a value per platform/profile with the following syntax

```yaml
<Setting Name>:
    <Platform A>:
        <Profile A>:
            ...
    <Platform B>:
        <Profile B>:
            ...
```

For example

```yaml
Defines:
    Windows:
        "g++": ["OS=Windows", "Compiler=g++"]
        "msvc": ["OS=Windows", "Compiler=msvc"]
    Linux:
        "g++": ["OS=Linux", "Compiler=g++"]
```

You can also specify values that apply to all platforms or profiles with the following keywords

- **DefaultPlatform**: Settings that apply to any platform that doesn't have explicit settings
- **DefaultProfile**: Settings that apply to any profile that doesn't have explicit settings

For example

```yaml
Defines:
    DefaultPlatform:
        "g++": ["Compiler=g++"]
        "msvc": ["Compiler=msvc"]
        DefaultProfile: ["Compiler=other"]
```

in this case `Compiler` is defined to the name of the profile regardless of which platform you are on, 
unless you are using a profile that is not `g++` or `msvc` in which case it will be defined as `other` 
instead.

!!! important
    DefaultPlatform and DefaultProfile settings are not additive. For example:
    ```yaml
    Defines:
        DefaultPlatform:
            DefaultProfile: ["A=1"]
            "g++": ["B=1"]
    ```
    When using g++, only `A` is defined (Not both `A` and `B`).
    When using any other profile, only `B` is defined.
