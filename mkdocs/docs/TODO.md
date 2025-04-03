# Roadmap

## Done

!!! info "`latest` version"
    - More git options
        - Add branch/tag option for git
        - Add initialize submodule option for git
    - Async/Multi-thread compile and dependencies processing


## Planned

### v0.3.0

- Ability to skip DefaultPlatform and DefaultProfile
- Allow runcpp2 to be library for scriptable pipeline
- Add ability to reference local YAML for user config
- Add version for default user config and prompt for update
- Add more default profiles

### v0.4.0

- Migrate to libyaml
- Ability to compile runcpp2 as single cpp

### TBD

- Add the ability for user to specify custom substitution options which applies to all fields
- Add the ability to append defines coming from the dependencies
- Check last run is shared lib or executable. Reset cache when necessary if different type
- Add wildcard support for filenames and extensions (Files Globbing)
- Add the ability to query script build directory
- Add the ability to list script dependencies

## Not planned yet

- Smoother CMake support by reading cmake target properties (https://stackoverflow.com/a/56738858/23479578)
- Add the ability to specify different profiles(?)/defines for different source files
- Handle escape characters at the end
    - To avoid situation like this:
        - Substitution string: -I "{path}"
        - Substitution value: .\
        - Substituted string: -I ".\"
            - Where the path contains escape character which escaped the wrapping quotes
- Use <csignal> to handle potential segfaults
- Use System2 subprocess if no prepend commands to be safer
- Add tests and examples (On Windows as well)
- Make SearchLibraryNames and SearchDirectories optional (?)
- Add cache limit
- Add system source type for dependencies
- Output compile_command.json
- Allow Languages to override FileExtensions in compiler profile (?)
- Custom Platform
- Ability to specify custom run commands

