# User Config

**TODO**

## `UserConfig.yaml`
```yaml
# WARNING: All command substitutions in this file are passed directly to the shell.
#          Exercise caution when using variables or user-provided input in your build commands
#          to prevent potential security vulnerabilities.

# A profile to be used if not specified while running the build script
PreferredProfile: 
    DefaultPlatform: "g++"
    Windows: "msvc"

# List of compiler/linker profiles that instruct how to compile/link
# See "./Default/g++.yaml" for the documentation of each field in a profile entry
Profiles:
-   Import: "./Default/g++.yaml"
-   Import: "./Default/vs2022_v17+.yaml"
```

## `Default/g++.yaml`
```yaml
# DO NOT modify this file. Changes will be overwritten when there's a reset or update

# List of anchors that will be aliased later. `Template` is **NOT** part of a profile
Templates:
    "g++_CompileRunParts": &g++_CompileRunParts
    -   Type: Once
        CommandPart: "{Executable} -c {CompileFlags}"
    -   Type: Repeats
        CommandPart: " -D{DefineNameOnly}="
    -   Type: Repeats
        CommandPart: " \"-D{DefineName}={DefineValue}\""
    -   Type: Repeats
        CommandPart: " -I\"{IncludeDirectoryPath}\""
    -   Type: Once
        CommandPart: " \"{InputFilePath}\" -o \"{OutputFileDirectory}{/}{ObjectLinkFile.Prefix}{InputFileName}{ObjectLinkFile.Extension}\""
    
    "g++_CompileExpectedOutputFiles": &g++_CompileExpectedOutputFiles
    -   "{OutputFileDirectory}{/}{ObjectLinkFile.Prefix}{InputFileName}{ObjectLinkFile.Extension}"

# Name (case sensitive) of the profile that can be queried from a script
Name: "g++"

# (Optional) Name aliases (case sensitive) of the current profile
NameAliases: ["mingw"]

# The file extensions associated with the profile
FileExtensions: [.cpp, .cc, .cxx]

# The languages supported by the profile
Languages: ["c++"]

# (Optional) The commands to run in **shell** before calling the compiler/linker for each platform.
#            This is run inside the root build directory.
# Setup: 
#     DefaultPlatform: []

# (Optional) The commands to run in **shell** after calling the compiler/linker for each platform.
#            This is run inside the root build directory.
# Cleanup: 
#     DefaultPlatform: []

# The file properties for the object files for each platform. 
# See "./CommonFileTypes.yaml" for FilesTypes
# FilesTypes: ...

# We can use the "Import" field to import other yaml files. We are importing "FilesTypes" here
Import: "./CommonFileTypes.yaml"

# Specify the compiler settings
Compiler:
    # (Optional) The command to be prepend for each compile command in **shell** for each platform
    # PreRun: 
    #     DefaultPlatform: ""
    
    # Shell command to use for checking if the executable exists or not
    CheckExistence: 
        DefaultPlatform: "g++ -v"
    
    # Here are a list of substitution strings for RunParts, Setup and Cleanup. 
    # To escape '{' and '}' to avoid substitutioon, simply repeat the '{' or '}' character again.
    # So "${MyBashVariable}" will become "${{MyBashVariable}}"
    
    # {Executable}:                 Compiler executable
    # {CompileFlags}:               Compile flags from config and override
    # {InputFileName}:              Name of the input file (without directory path and extension)
    # {InputFileExtension}:         Extension of the input file
    # {InputFileDirectory}:         Directory of the input file
    # {InputFilePath}:              Full path to the input file
    # {OutputFileDirectory}:        Directory of all the output files
    # {/}:                          Filesystem separator for the host platform
    
    # {SharedLibraryFile.Prefix}
    # {SharedLinkFile.Prefix}
    # {StaticLinkFile.Prefix}
    # {ObjectLinkFile.Prefix}
    # {DebugSymbolFile.Prefix}
    
    # {SharedLibraryFile.Extension}
    # {SharedLinkFile.Extension}
    # {StaticLinkFile.Extension}
    # {ObjectLinkFile.Extension}
    # {DebugSymbolFile.Extension}
    
    # Below are iterable substitution strings, must be inside "Repeats" run type:
    # {IncludeDirectoryPath}:       Path to all the include directories
    # {DefineNameOnly}:             All the defines without a value specified (equivalent to #define X)
    # {DefineName}:                 Name of all the defines that has a value specified
    # {DefineValue}:                Value of all the defines that has a value specified (use together with {DefineName})
    CompileTypes:
        Executable:
            DefaultPlatform:
                # Default flags to be substituted as {CompileFlags}
                Flags: "-std=c++17 -Wall -g"
                
                # The executable to be substituted as {Executable}
                Executable: "g++"
                
                # The components for the command to be run
                RunParts: *g++_CompileRunParts
                
                # What files to be expected as output for the command
                ExpectedOutputFiles: *g++_CompileExpectedOutputFiles
                
                # (Optional) The commands to run in **shell** BEFORE compiling
                #            This is run inside the .runcpp2 directory where the build happens.
                # Setup: []
                
                # (Optional) The commands to run in **shell** AFTER compiling
                #            This is run inside the .runcpp2 directory where the build happens.
                # Cleanup: []
        ExecutableShared:
            DefaultPlatform:
                Flags: "-std=c++17 -Wall -g -fpic"
                Executable: "g++"
                RunParts: *g++_CompileRunParts
                ExpectedOutputFiles: *g++_CompileExpectedOutputFiles
                # Setup: []
                # Cleanup: []
        Static:
            DefaultPlatform:
                Flags: "-std=c++17 -Wall -g"
                Executable: "g++"
                RunParts: *g++_CompileRunParts
                ExpectedOutputFiles: *g++_CompileExpectedOutputFiles
                # Setup: []
                # Cleanup: []
        Shared:
            DefaultPlatform:
                Flags: "-std=c++17 -Wall -g -fpic"
                Executable: "g++"
                RunParts: *g++_CompileRunParts
                ExpectedOutputFiles: *g++_CompileExpectedOutputFiles
                # Setup: []
                # Cleanup: []

# Specify the linker settings
Linker:
    CheckExistence:
        DefaultPlatform: "g++ -v"
    
    # Here are a list of substitution strings for RunParts, Setup and Cleanup
    # {Executable}:                 Linker executable
    # {LinkFlags}:                  Link flags from config and override
    # {OutputFileName}:             Name of all the output files (without directory path and extension)
    # {OutputFileDirectory}:        Directory of all the output files
    # {/}:                          Filesystem separator for the host platform
    
    # {SharedLibraryFile.Prefix}
    # {SharedLinkFile.Prefix}
    # {StaticLinkFile.Prefix}
    # {ObjectLinkFile.Prefix}
    # {DebugSymbolFile.Prefix}
    
    # {SharedLibraryFile.Extension}
    # {SharedLinkFile.Extension}
    # {StaticLinkFile.Extension}
    # {ObjectLinkFile.Extension}
    # {DebugSymbolFile.Extension}
    
    # Below are iterable substitution strings, must be inside "Repeats" run type:
    # {LinkFileName}:               Name of the file to be linked, regardless of the build type
    # {LinkFileExtension}:          File Extension of the file to be linked, regardless of the build type
    # {LinkFileDirectory}:          Directory of the file to be linked, regardless of the build type
    # {LinkFilePath}:               Full path to the file to be linked, regardless of the build type
    
    # {LinkObjectFileName}:         Name of the object file to be linked
    # {LinkObjectFileExtension}:    File Extension of the object file to be linked
    # {LinkObjectFileDirectory}:    Directory of the object file to be linked
    # {LinkObjectFilePath}:         Full path to the object file to be linked
    
    # {LinkSharedFileName}:         Name of the shared file to be linked
    # {LinkSharedFileExtension}:    File Extension of the shared file to be linked
    # {LinkSharedFileDirectory}:    Directory of the shared file to be linked
    # {LinkSharedFilePath}:         Full path to the shared file to be linked
    
    # {LinkStaticFileName}:         Name of the static file to be linked
    # {LinkStaticFileExtension}:    File Extension of the static file to be linked
    # {LinkStaticFileDirectory}:    Directory of the static file to be linked
    # {LinkStaticFilePath}:         Full path to the static file to be linked
    LinkTypes:
        Executable:
            Unix:
                Flags: "-Wl,-rpath,\\$ORIGIN"
                Executable: "g++"
                RunParts:
                -   Type: Once
                    CommandPart: "{Executable} {LinkFlags} -o \"{OutputFileDirectory}{/}{OutputFileName}\""
                -   Type: Repeats
                    CommandPart: " \"{LinkFilePath}\""
                ExpectedOutputFiles: ["{OutputFileDirectory}{/}{OutputFileName}"]
                # Setup: []
                # Cleanup: []
            Windows:
                Flags: "-Wl,-rpath,\\$ORIGIN"
                Executable: "g++"
                RunParts:
                -   Type: Once
                    CommandPart: "{Executable} {LinkFlags} -o \"{OutputFileDirectory}{/}{OutputFileName}.exe\""
                -   Type: Repeats
                    CommandPart: " \"{LinkFilePath}\""
                ExpectedOutputFiles: ["{OutputFileDirectory}{/}{OutputFileName}.exe"]
                # Setup: []
                # Cleanup: []
        ExecutableShared:
            DefaultPlatform:
                Flags: "-shared -Wl,-rpath,\\$ORIGIN"
                Executable: "g++"
                RunParts:
                -   Type: Once
                    CommandPart: "{Executable} {LinkFlags} -o \"{OutputFileDirectory}{/}{SharedLibraryFile.Prefix}{OutputFileName}{SharedLibraryFile.Extension}\""
                -   Type: Repeats
                    CommandPart: " \"{LinkFilePath}\""
                ExpectedOutputFiles: ["{OutputFileDirectory}{/}{SharedLibraryFile.Prefix}{OutputFileName}{SharedLibraryFile.Extension}"]
                # Setup: []
                # Cleanup: []
        Static:
            DefaultPlatform:
                Flags: ""
                Executable: "g++"
                RunParts:
                -   Type: Once
                    CommandPart: "{Executable} {LinkFlags} -o \"{OutputFileDirectory}{/}{StaticLinkFile.Prefix}{OutputFileName}{StaticLinkFile.Extension}\""
                -   Type: Repeats
                    CommandPart: " \"{LinkFilePath}\""
                ExpectedOutputFiles: ["{OutputFileDirectory}{/}{StaticLinkFile.Prefix}{OutputFileName}{StaticLinkFile.Extension}"]
                # Setup: []
                # Cleanup: []
        Shared:
            DefaultPlatform:
                Flags: "-shared -Wl,-rpath,\\$ORIGIN"
                Executable: "g++"
                RunParts:
                -   Type: Once
                    CommandPart: "{Executable} {LinkFlags} -o \"{OutputFileDirectory}{/}{SharedLibraryFile.Prefix}{OutputFileName}{SharedLibraryFile.Extension}\""
                -   Type: Repeats
                    CommandPart: " \"{LinkFilePath}\""
                ExpectedOutputFiles: ["{OutputFileDirectory}{/}{SharedLibraryFile.Prefix}{OutputFileName}{SharedLibraryFile.Extension}"]
                # Setup: []
                # Cleanup: []

```

## `Default/vs2022_v17+.yaml`
```yaml
# DO NOT modify this file. Changes will be overwritten when there's a reset or update

# List of anchors that will be aliased later. `Template` is **NOT** part of a profile
Templates:
    vs2022_v17+_CompileFlags: &vs2022_v17+_CompileFlags
        Flags: "/nologo /W4 /diagnostics:caret /utf-8 /Gm- /MDd /EHar /TP /std:c++17 /GR /RTC1 /Zc:inline /Zi"
    "vs2022_v17+_CompileRunParts": &vs2022_v17+_CompileRunParts
    -   Type: Once
        CommandPart: "{Executable} /c {CompileFlags}"
    -   Type: Repeats
        CommandPart: " /D{DefineNameOnly}="
    -   Type: Repeats
        CommandPart: " \"/D{DefineName}={DefineValue}\""
    -   Type: Repeats
        CommandPart: " /I\"{IncludeDirectoryPath}\""
    -   Type: Once
        CommandPart: " /Fo\"{OutputFileDirectory}{/}{ObjectLinkFile.Prefix}{InputFileName}{ObjectLinkFile.Extension}\" \
            /Fd\"{OutputFileDirectory}{/}{DebugSymbolFile.Prefix}{InputFileName}{DebugSymbolFile.Extension}\" \
            \"{InputFilePath}\""
    "vs2022_v17+_CompileExpectedOutputFiles": &vs2022_v17+_CompileExpectedOutputFiles
    -   "{OutputFileDirectory}{/}{ObjectLinkFile.Prefix}{InputFileName}{ObjectLinkFile.Extension}"
    -   "{OutputFileDirectory}{/}{DebugSymbolFile.Prefix}{InputFileName}{DebugSymbolFile.Extension}"
    
# https://learn.microsoft.com/en-us/cpp/overview/compiler-versions?view=msvc-170
Name: "vs2022_v17+"
NameAliases: ["msvc1930+", "msvc"]
FileExtensions: [.cpp, .cc, .cxx]
Languages: ["c++"]
Import: "./CommonFileTypes.yaml"
Setup: 
    Windows:
    -   >-
        for /f "usebackq tokens=*" %i in (`CALL "C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe" 
        -version "[17.0,18.0)" -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do ( 
        echo "%i\VC\Auxiliary\Build\vcvarsall.bat" x64 > .\prerun.bat
        )
Cleanup: 
    Windows: [ "del .\\prerun.bat" ]
Compiler:
    PreRun: 
        Windows: ".\\prerun.bat"
    CheckExistence: 
        Windows: "where.exe CL.exe"
    CompileTypes:
        Executable:
            Windows:
                <<: *vs2022_v17+_CompileFlags
                Executable: "CL.exe"
                RunParts: *vs2022_v17+_CompileRunParts
                ExpectedOutputFiles: *vs2022_v17+_CompileExpectedOutputFiles
        ExecutableShared:
            Windows:
                <<: *vs2022_v17+_CompileFlags
                Executable: "CL.exe"
                RunParts: *vs2022_v17+_CompileRunParts
                ExpectedOutputFiles: *vs2022_v17+_CompileExpectedOutputFiles
        Static:
            Windows:
                <<: *vs2022_v17+_CompileFlags
                Executable: "CL.exe"
                RunParts: *vs2022_v17+_CompileRunParts
                ExpectedOutputFiles: *vs2022_v17+_CompileExpectedOutputFiles
        Shared:
            Windows:
                <<: *vs2022_v17+_CompileFlags
                Executable: "CL.exe"
                RunParts: *vs2022_v17+_CompileRunParts
                ExpectedOutputFiles: *vs2022_v17+_CompileExpectedOutputFiles
Linker:
    PreRun: 
        Windows: ".\\prerun.bat"
    CheckExistence:
        Windows: "where.exe link.exe"
    LinkTypes:
        Executable:
            Windows:
                Flags: >-
                    /NOLOGO kernel32.lib user32.lib gdi32.lib winspool.lib shell32.lib ole32.lib
                    oleaut32.lib uuid.lib comdlg32.lib advapi32.lib /manifest:embed /SUBSYSTEM:CONSOLE
                    /DEBUG /MANIFESTUAC:"level='asInvoker'"
                Executable: "link.exe"
                RunParts:
                -   Type: Once
                    CommandPart: >-
                        {Executable} {LinkFlags}
                        /OUT:"{OutputFileDirectory}{/}{OutputFileName}.exe"
                -   Type: Repeats
                    CommandPart: " \"{LinkFilePath}\""
                ExpectedOutputFiles: ["{OutputFileDirectory}{/}{OutputFileName}.exe"]
        ExecutableShared:
            Windows:
                Flags: >-
                    /NOLOGO kernel32.lib user32.lib gdi32.lib winspool.lib shell32.lib ole32.lib
                    oleaut32.lib uuid.lib comdlg32.lib advapi32.lib /manifest:embed /SUBSYSTEM:CONSOLE
                    /DEBUG /DLL /MANIFESTUAC:"level='asInvoker'"
                Executable: "link.exe"
                RunParts:
                -   Type: Once
                    CommandPart: >-
                        {Executable} {LinkFlags}
                        /OUT:"{OutputFileDirectory}{/}{SharedLibraryFile.Prefix}{OutputFileName}{SharedLibraryFile.Extension}"
                        /IMPLIB:"{OutputFileDirectory}{/}{SharedLinkFile.Prefix}{OutputFileName}{SharedLinkFile.Extension}"
                        /DEF:".\temp.def"
                -   Type: Repeats
                    CommandPart: " \"{LinkFilePath}\""
                ExpectedOutputFiles: ["{OutputFileDirectory}{/}{SharedLibraryFile.Prefix}{OutputFileName}{SharedLibraryFile.Extension}"]
                Setup: [ "echo EXPORTS > .\\temp.def", "echo.   main @1 >> .\\temp.def" ]
                Cleanup: [ "del .\\temp.def" ]
        Static:
            Windows:
                Flags: "/NOLOGO"
                Executable: "lib.exe"
                RunParts:
                -   Type: Once
                    CommandPart: >-
                        {Executable} {LinkFlags}
                        /OUT:"{OutputFileDirectory}{/}{StaticLinkFile.Prefix}{OutputFileName}{StaticLinkFile.Extension}"
                        /IMPLIB:"{OutputFileDirectory}{/}{SharedLinkFile.Prefix}{OutputFileName}{SharedLinkFile.Extension}"
                -   Type: Repeats
                    CommandPart: " \"{LinkFilePath}\""
                ExpectedOutputFiles: ["{OutputFileDirectory}{/}{StaticLinkFile.Prefix}{OutputFileName}{StaticLinkFile.Extension}"]
        Shared:
            Windows:
                Flags: >-
                    /NOLOGO kernel32.lib user32.lib gdi32.lib winspool.lib shell32.lib ole32.lib
                    oleaut32.lib uuid.lib comdlg32.lib advapi32.lib /manifest:embed /SUBSYSTEM:CONSOLE
                    /DEBUG /DLL /MANIFESTUAC:"level='asInvoker'"
                Executable: "link.exe"
                RunParts:
                -   Type: Once
                    CommandPart: >-
                        {Executable} {LinkFlags}
                        /OUT:"{OutputFileDirectory}{/}{SharedLibraryFile.Prefix}{OutputFileName}{SharedLibraryFile.Extension}"
                        /IMPLIB:"{OutputFileDirectory}{/}{SharedLinkFile.Prefix}{OutputFileName}{SharedLinkFile.Extension}"
                -   Type: Repeats
                    CommandPart: " \"{LinkFilePath}\""
                ExpectedOutputFiles: ["{OutputFileDirectory}{/}{SharedLibraryFile.Prefix}{OutputFileName}{SharedLibraryFile.Extension}"]

```

## `Default/CommonFileTypes.yaml`
```yaml
# DO NOT modify this file. Changes will be overwritten when there's a reset or update

FilesTypes:
    # The file properties for the files to be **linked** as object file for each platform
    ObjectLinkFile:
        Prefix:
            DefaultPlatform: ""
        Extension:
            Windows: ".obj"
            Unix: ".o"
    # The file properties for the files to be **linked** as shared libraries for each platform
    SharedLinkFile:
        Prefix:
            Windows: ""
            Linux: "lib"
            MacOS: ""
        Extension:
            Windows: ".lib"
            Linux: ".so"
            MacOS: ".dylib"
    # The file properties for the files to be **copied** as shared libraries for each platform
    SharedLibraryFile:
        Prefix:
            Windows: ""
            Linux: "lib"
            MacOS: ""
        Extension:
            Windows: ".dll"
            Linux: ".so"
            MacOS: ".dylib"
    # The file properties for the files to be linked as static libraries for each platform
    StaticLinkFile:
        Prefix:
            Unix: "lib"
            Windows: ""
        Extension:
            Windows: ".lib"
            Unix: ".a"
    # (Optional) The file properties for debug symbols to be copied alongside the binary 
    #               for each platform
    DebugSymbolFile:
        Prefix:
            Windows: ""
            Unix: ""
        Extension:
            Windows: ".pdb"
            Unix: ""

```
