# Roadmap


## Done

### Nightly
- Check last run is shared lib or executable. Reset cache when necessary if different type

### v0.3.0
- More git options
    - Add branch/tag option for git
    - Add initialize submodule option for git
- Async/Multi-thread compile and dependencies processing
- Ability to skip DefaultPlatform and DefaultProfile
- Handle escape characters at the end
    - To avoid situation like this:
        - Substitution string: `-I "{path}"`
        - Substitution value: `.\`
        - Substituted string: `-I ".\"`
            - Where the path contains escape character which escaped the wrapping quotes
- Add platform map for PreferredProfile for user config
- Add ability to reference local YAML file for config profiles
- Add interactive tutorials and redo documentations

## Planned

### v0.4.0

- Allow runcpp2 to be library for scriptable pipeline
- Add version for default user config and prompt for update
- Add more default profiles
- Migrate to libyaml
- Ability to compile runcpp2 as single cpp

## High Priority

- Update `FileProperties.hpp` to use list of string for prefix and extension
    - Merge `SharedLinkFile` and `SharedLibraryFile`
- Add the ability for user to specify custom substitution options which applies to all fields
- Add the ability to append defines coming from the dependencies
- Add wildcard support for filenames and extensions (Files Globbing)
- Add the ability to query script build directory
- Add the ability to list script dependencies

## TBD

- Smoother CMake support by reading cmake target properties (https://stackoverflow.com/a/56738858)
<!--
if(NOT CMAKE_PROPERTY_LIST)
    execute_process(COMMAND cmake --help-property-list OUTPUT_VARIABLE CMAKE_PROPERTY_LIST)
    
    # Convert command output into a CMake list
    string(REGEX REPLACE ";" "\\\\;" CMAKE_PROPERTY_LIST "${CMAKE_PROPERTY_LIST}")
    string(REGEX REPLACE "\n" ";" CMAKE_PROPERTY_LIST "${CMAKE_PROPERTY_LIST}")
    list(REMOVE_DUPLICATES CMAKE_PROPERTY_LIST)
endif()
    
function(print_properties)
    message("CMAKE_PROPERTY_LIST = ${CMAKE_PROPERTY_LIST}")
endfunction()
    
function(print_target_properties target)
    if(NOT TARGET ${target})
      message(STATUS "There is no target named '${target}'")
      return()
    endif()

    foreach(property ${CMAKE_PROPERTY_LIST})
        string(REPLACE "<CONFIG>" "${CMAKE_BUILD_TYPE}" property ${property})

        # Fix https://stackoverflow.com/questions/32197663/how-can-i-remove-the-the-location-property-may-not-be-read-from-target-error-i
        if(property STREQUAL "LOCATION" OR property MATCHES "^LOCATION_" OR property MATCHES "_LOCATION$")
            continue()
        endif()

        get_property(was_set TARGET ${target} PROPERTY ${property} SET)
        if(was_set)
            get_target_property(value ${target} ${property})
            message("${target} ${property} = ${value}")
        endif()
    endforeach()
endfunction()

print_target_properties(matplot)
-->
- Add the ability to specify different profiles(?)/defines for different source files
- Use `<csignal>` to handle potential segfaults
- Use System2 subprocess if no prepend commands to be safer
- Add tests and examples (On Windows as well)
- Make SearchLibraryNames and SearchDirectories optional (?)
- Add cache limit
- Add system source type for dependencies
- Output compile_command.json
- Allow Languages to override FileExtensions in compiler profile (?)
- Custom Platform
- Ability to specify custom run commands

