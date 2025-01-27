# runcpp2

![](./Runcpp2Logo.png)

runcpp2 is a simple declarable, scriptable, flexible cross-platform build system build system for c or c++

- 🚀 **Simple**: `runcpp2 main.cpp`, this is all you need to get started
- 📝 **Declarable**: *Quick, Concise, Minimal* YAML format
- 🔧 **Scriptable**: *Customize, Run And Debug* your build pipeline with c++, or just use it as a script. 
                     No longer need to juggle between CMake, Python, Bash, Batch, Lua, etc...
- 🪜 **Flexible**: *YAML* for small project, *c++* for finer control

For more information, see [Full Documentation](https://neko-box-coder.github.io/runcpp2/latest/)

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

> *shell*
```shell
runcpp2 ./script.cpp <any arguments>
```

> [!NOTE]
> On Unix, if you have added runcpp2 to your PATH and add this line `//bin/true;runcpp2 "$0" "$@"; exit $?;` 
> to the top of your script, you can run the script directly by `./script.cpp <arguments>`

---

### 2. Watch and give compile errors
If you want to edit the script but want to have feedback for any error, you can use "watch" mode.

> *shell*
```shell
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
    - Both inline (but continuous) comments (`//`) and block comments are supported (`/* */`)

For a complete list of build settings, see [Build Settings](https://neko-box-coder.github.io/runcpp2/latest/build_settings/) or generate the template with

> *shell*
```shell
runcpp2 --create-script-template ./script.cpp   # Embeds the build settings template as comment
runcpp2 --create-script-template ./script.yaml  # Creates the build settings template as dedicated yaml file
runcpp2 -t ./script.cpp                         # Short form
```


