TODO:
- Ability to skip DefaultPlatform and DefaultProfile
- Allow runcpp2 to be library
- Add the ability to specify different profiles(?)/defines for different source files
- Add the ability to append defines coming from the dependencies
- Add the ability for user to specify custom substitution options which applies to all fields
- Ability to compile runcpp2 as single cpp
- Async compile
- Check last run is shared lib or executable. Reset cache when necessary if different type
- Handle escape characters at the end
    - To avoid situation like this:
        - Substitution string: -I "{path}"
        - Substitution value: .\
        - Substituted string: -I ".\"
            - Where the path contains escape character which escaped the wrapping quotes
- Use <csignal> to handle potential segfaults
- Separate git and local source options
    - Add branch/tag option for git
    - Add initialize submodule option for git
- Use System2 subprocess if no prepend commands to be safer
- Migrate to libyaml
- Add wildcard support for filenames and extensions
- Add tests and examples (On Windows as well)
- Make SearchLibraryNames and SearchDirectories optional (?)
- Add cache limit
- Add system source type for dependencies
- Add ability to reference local YAML for user config
- Add version for user config and prompt for update
- Output compile_command.json
- Allow Languages to override FileExtensions in compiler profile (?)



