# Roadmap

## Done

!!! info "`latest` version"
    - More git options
        - Add branch/tag option for git
        - Add initialize submodule option for git
    - Async/Multi-thread compile and dependencies processing


## Planned
- Ability to skip DefaultPlatform and DefaultProfile
- Allow runcpp2 to be library for scriptable pipeline
- Smoother CMake support by reading cmake target properties (https://stackoverflow.com/a/56738858/23479578)
- Add the ability for user to specify custom substitution options which applies to all fields
- Add the ability to append defines coming from the dependencies
- Check last run is shared lib or executable. Reset cache when necessary if different type
- Add ability to reference local YAML for user config
- Add version for user config and prompt for update
- Add wildcard support for filenames and extensions (Files Globbing)
- Add the ability to query script build directory
- Add the ability to list script dependencies

## Not planned yet

- Add the ability to specify different profiles(?)/defines for different source files
- Ability to compile runcpp2 as single cpp
- Handle escape characters at the end
    - To avoid situation like this:
        - Substitution string: -I "{path}"
        - Substitution value: .\
        - Substituted string: -I ".\"
            - Where the path contains escape character which escaped the wrapping quotes
- Use <csignal> to handle potential segfaults
- Use System2 subprocess if no prepend commands to be safer
- Migrate to libyaml
- Add tests and examples (On Windows as well)
- Make SearchLibraryNames and SearchDirectories optional (?)
- Add cache limit
- Add system source type for dependencies
- Output compile_command.json
- Allow Languages to override FileExtensions in compiler profile (?)
- Custom Platform
- Ability to specify custom run commands

