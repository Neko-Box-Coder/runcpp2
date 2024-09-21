# runcpp2

![runcpp2 logo](./Runcpp2Logo.png)

A cross-platform tool that can let you run any c++ files as a script, just like python!

Run c++ files anytime, anywhere.

### 🛠️ Prerequisites
- Any C++ compiler. The default user config only has g++ and msvc profiles. But feel free to
add other compilers.

### 📥️ Installation
You can either build from source or use the binary release

To build from source:
1. Clone the repository with `git clone --recursive https://github.com/Neko-Box-Coder/runcpp2.git`
2. Run `Build.sh` or `Build.bat` to build

TODO: Ability to use pre-built binary

Finally, you just need to add runcpp2 binary location to the `PATH` environment variable and 
you can run c++ files anywhere you want.

### ⚡️ Getting Started

#### 1. Running directly
Suppose you have a c++ file called `script.cpp`, you can run it immediately by doing 

```shell
runcpp2 ./script.cpp <any arguments>
```

> [!NOTE]
> When invoking a c++ file with runcpp2, the first argument (`argv[0]`) to `main()` is the path
> to the script, not the path to an executable.

#### 2. Watch and give compile errors
If you want to edit the script but want to have feedback for any error, you can use "watch" mode.

```shell
runcpp2 --watch ./script.cpp
```

#### 3. Adding script build settings
If you want to add custom build settings such as compile/link flags, specify profile, etc. 
You will need to provide such settings to runcpp2 in the format of YAML.

This build settings can either be embedded as comment in the script itself, or provided as 
a YAML file.

To generate a script build settings template, do 

```shell
# Embeds the build settings template as comment
runcpp2 --create-script-template ./script.cpp

# Creates the build settings template as dedicated yaml file
runcpp2 --create-script-template ./script.yaml

# Short form
runcpp2 -t ./script.cpp
```

This will generate the script build settings template for you. 
Everything is documented as comment in the template but here's a quick summary.

- `RequiredProfiles`: To specify a specific profile for building for different platforms
- `OverrideCompileFlags`: Compile flags to be added or removed from the current profile
- `OverrideLinkFlags`: Same as `OverrideCompileFlags` but for linking
- `OtherFilesToBeCompiled`: Other source files you wish to be compiled.
- `Dependencies`: Any external libraries you wish to use. See next section.

#### 4. Using External Libraries

To use any external libraries, you need to specify them in the Dependencies section.
Here's a quick run down on the important fields

- `Source`: This specifies the source of the external dependency. 
It can either be type `Git` or `Local` where it will clone the repository if it is `Git` or 
copy the library folder in the filesystem if it is `Local`
- `LibraryType`: This specifies the dependency type to be either `Static`, `Object`, `Shared` 
or `Header`
- `IncludePaths`: The include paths relative to the root of the dependency folder
- `LinkProperties`: Settings for linking
- `Setup`, `Build` and `Cleanup`: List of shell commands for one time setup, building and 
cleaning up

To access the source files of the dependencies, you can specify runcpp2 to build locally at 
where it is invoked from by passing the `--local` flag. This is useful when you want to
look at the headers of the dependencies.

```shell
runcpp2 --local ./script.cpp
```

This will create a `.runcpp2` folder and all the builds and dependencies will be inside it.
