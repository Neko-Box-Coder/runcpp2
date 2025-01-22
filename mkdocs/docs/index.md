# Home

![](./Runcpp2Logo.png)

runcpp2 is a simple declarable, scriptable, flexible cross-platform build system build system for c or c++

- 🚀 **Simple**: `#!shell runcpp2 main.cpp`, this is all you need to get started
- 📝 **Declarable**: *Quick, Concise, Minimal* YAML format
- 🔧 **Scriptable**: *Customize, Run And Debug* your build pipeline with c++, or just use it as a script. 
                     No longer need to juggle between CMake, Python, Bash, Batch, Lua, etc...
- 🪜 **Flexible**: *YAML* for small project, *c++* for finer control


## 🛠️ Prerequisites
- Any c or c++ compiler. The default user config only has g++ and msvc profiles. But feel free to
add other compilers.

## 📥️ Installation
You can either build from source or use the binary release

Binary Release (Only Linux and Windows for now): 
[https://github.com/Neko-Box-Coder/runcpp2/releases](https://github.com/Neko-Box-Coder/runcpp2/releases)


Finally, you just need to add runcpp2 binary location to the `PATH` environment variable and 
you can run c++ files anywhere you want.

## ⚡️ Getting Started

### 1. Running source file directly
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

### 2. Watch and give compile errors
If you want to edit the script but want to have feedback for any error, you can use "watch" mode.

```shell title="shell"
runcpp2 --watch ./script.cpp
```

---

### 3. Sepcifying Build Settings
Build settings such as compile/link flags, external dependencies, command hooks, etc.
can be spcified inlined inside a source file or as a separate yaml file in the format of YAML

- To specify build settings in a dedicated yaml file:
    - The yaml file in the same directory and share the same as the source file being run will be used
- To specify inline build settings inside a source file: 
    - Put them inside a comment with `runcpp2` at the beginning of the build settings
    - The inline build settings can exist in anywhere of the source file
    - Both inline (but continuous) comments (`#!cpp //`) and block comments are supported (`#!cpp /* */`)

??? example "Example Inline Build Settings"
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

??? example "Example Dedicated Build Settings"
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

