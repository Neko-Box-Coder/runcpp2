cmake_minimum_required(VERSION 3.10)

# For Clang to do parsing
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(runcpp2)

set(CMAKE_CXX_STANDARD 11)

option(RUNCPP2_UPDATE_DEFAULT_YAMLS "Update default yaml files" OFF)

if (CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    option(RUNCPP2_WARNINGS_AS_ERRORS "Treat warnings as errors" ON)
    option(RUNCPP2_BUILD_TESTS "Build runcpp2 tests" ON)
else()
    option(RUNCPP2_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
    option(RUNCPP2_BUILD_TESTS "Build runcpp2 tests" OFF)
endif()

# =========================================================================
# Retrieving Version String
# =========================================================================

function(get_version_from_git OUTPUT_VARIABLE)
    # Get the latest tag
    execute_process(
        COMMAND git describe --tags --abbrev=0
        OUTPUT_VARIABLE GIT_TAG
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE GIT_TAG_RESULT
        ERROR_QUIET
    )

    # Get the current commit hash
    execute_process(
        COMMAND git rev-parse --short HEAD
        OUTPUT_VARIABLE GIT_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    if(GIT_TAG_RESULT EQUAL 0)
        # Check if the tag is on the current commit
        execute_process(
            COMMAND git describe --tags --exact-match
            RESULT_VARIABLE TAG_ON_COMMIT
            OUTPUT_QUIET
            ERROR_QUIET
        )

        if(TAG_ON_COMMIT EQUAL 0)
            set(VERSION ${GIT_TAG})
        else()
            set(VERSION "${GIT_TAG}-${GIT_COMMIT_HASH}")
        endif()
    else()
        # No tags found, use v0.0.0 with commit hash
        set(VERSION "v0.0.0-${GIT_COMMIT_HASH}")
    endif()

    set(${OUTPUT_VARIABLE} ${VERSION} PARENT_SCOPE)
endfunction()

# Call the function and store the result
get_version_from_git(RUNCPP2_PROJECT_VERSION)
message("RUNCPP2_PROJECT_VERSION: ${RUNCPP2_PROJECT_VERSION}")

# =========================================================================
# External Dependencies
# =========================================================================
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/External/ssLogger")

set(RYML_DEFAULT_CALLBACK_USES_EXCEPTIONS ON CACHE BOOL "" FORCE)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/External/rapidyaml")

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/External/filesystem")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/External/System2")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/External/dylib")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/External/variant")

# Copy cfgpath.h
configure_file( "${CMAKE_CURRENT_LIST_DIR}/External/cfgpath/cfgpath.h" 
                "${CMAKE_CURRENT_LIST_DIR}/Include/cfgpath.h" COPYONLY)



if(RUNCPP2_BUILD_TESTS)
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/External/ssTest")
endif()
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/External/CppOverride")


# =========================================================================
# Generate yaml files as c
# =========================================================================
if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/DefaultYAMLs.c")
    set(EMBEDDED_FILE_SIZE 0)
else()
    file(SIZE "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/DefaultYAMLs.c" EMBEDDED_FILE_SIZE)
endif()

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/External/Embed2C")

if(RUNCPP2_UPDATE_DEFAULT_YAMLS OR EMBEDDED_FILE_SIZE LESS 1024)
    include("${CMAKE_CURRENT_LIST_DIR}/External/Embed2C/embedFile.cmake")

    set(EMBED_EXEC_PATH "")
    GET_EXEC_PATH(EMBED_EXEC_PATH)

    if("${EMBED_EXEC_PATH}" STREQUAL "")
        message(WARNING "Please build Embed2C first")
        return()
    else()
        set(FILES_TO_EMBED "${CMAKE_CURRENT_LIST_DIR}/DefaultYAMLs/DefaultUserConfig.yaml"
                            "DefaultUserConfig"
                            "${CMAKE_CURRENT_LIST_DIR}/DefaultYAMLs/DefaultScriptInfo.yaml"
                            "DefaultScriptInfo"
                            )

        EMBED_FILES("${EMBED_EXEC_PATH}"
                    "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/DefaultYAMLs.c"
                    "${FILES_TO_EMBED}")
    endif()
endif()

# =========================================================================
# runcpp2
# =========================================================================

add_library(runcpp2 STATIC
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/Data/Profile.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/Data/DependencyInfo.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/Data/DependencyLinkProperty.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/Data/DependencySource.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/Data/GitSource.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/Data/LocalSource.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/Data/FileProperties.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/Data/FlagsOverrideInfo.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/Data/ProfilesCommands.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/Data/ProfilesFlagsOverride.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/Data/ScriptInfo.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/Data/ProfilesProcessPaths.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/Data/FilesTypesInfo.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/Data/StageInfo.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/Data/ProfilesDefines.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/Data/FilesToCopyInfo.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/Data/BuildTypeHelper.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/ProfileHelper.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/CompilingLinking.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/ConfigParsing.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/DefaultYAMLs.c"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/DependenciesHelper.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/ParseUtil.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/PlatformUtil.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/StringUtil.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/runcpp2.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/BuildsManager.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/PipelineSteps.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/IncludeManager.cpp"
                        )

target_include_directories(runcpp2 PUBLIC "${CMAKE_CURRENT_LIST_DIR}/Include")
target_link_libraries(runcpp2 PRIVATE ssLogger System2 CppOverride dylib)
target_link_libraries(runcpp2 PUBLIC ghc_filesystem ryml::ryml mpark_variant)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # TODO: Try to change to /Wall
    set(RUNCPP2_STANDARD_COMPILE_FLAGS "/utf-8;/W1;/DGHC_WIN_DISABLE_WSTRING_STORAGE_TYPE=1")
    if (RUNCPP2_WARNINGS_AS_ERRORS)
        list(APPEND RUNCPP2_STANDARD_COMPILE_FLAGS "/WX")
    endif()
    
    message("RUNCPP2_STANDARD_COMPILE_FLAGS: ${RUNCPP2_STANDARD_COMPILE_FLAGS}")
else()
    set(RUNCPP2_STANDARD_COMPILE_FLAGS  "-Wall"
                                        "-Wno-return-local-addr"
                                        "-Wno-sign-compare"
                                        #"-Wno-unused-variable"
                                        #"-Wno-unused-but-set-variable"
                                        "-Wno-unused-parameter"
                                        "-Wno-switch"
                                        "-Wno-gnu-zero-variadic-macro-arguments"
                                        "-Wextra"
                                        "-pedantic")
    if (RUNCPP2_WARNINGS_AS_ERRORS)
        list(APPEND RUNCPP2_STANDARD_COMPILE_FLAGS "-Werror")
    endif()
endif()

target_compile_options(runcpp2 PRIVATE "${RUNCPP2_STANDARD_COMPILE_FLAGS}")

# Define the version macro
target_compile_definitions(runcpp2 PUBLIC RUNCPP2_VERSION="${RUNCPP2_PROJECT_VERSION}")

add_executable(runcpp2_main "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/main.cpp")
target_compile_options(runcpp2_main PRIVATE "${RUNCPP2_STANDARD_COMPILE_FLAGS}")
target_link_libraries(runcpp2_main PRIVATE runcpp2 ssLogger ghc_filesystem)
set_target_properties(runcpp2_main PROPERTIES OUTPUT_NAME "runcpp2")

if(RUNCPP2_BUILD_TESTS)
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Src/Tests")
endif()


