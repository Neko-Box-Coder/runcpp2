cmake_minimum_required(VERSION 3.10)

# For Clang to do parsing
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(runcpp2)

# if(TARGET runcpp2)
#     message("runcpp2 cmake target has been added before. Skipping cmake...")
#     return()
# endif()

set(CMAKE_CXX_STANDARD 11)

option(RUNCPP2_UPDATE_DEFAULT_YAMLS "Update default yaml files" OFF)

set(RUNCPP2_CONFIG_VERSION "1" CACHE STRING "Default Config Version")

if (CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    option(RUNCPP2_WARNINGS_AS_ERRORS "Treat warnings as errors" ON)
    option(RUNCPP2_BUILD_TESTS "Build runcpp2 tests" ON)
else()
    option(RUNCPP2_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
    option(RUNCPP2_BUILD_TESTS "Build runcpp2 tests" OFF)
endif()

# Don't build any tests for now until we have rewritten th
option(RUNCPP2_BUILD_TESTS "Build runcpp2 tests" OFF)
set(RUNCPP2_BUILD_TESTS OFF)

# =========================================================================
# Retrieving Version String
# =========================================================================

function(get_version_from_git OUTPUT_VARIABLE)
    execute_process(
        COMMAND git tag -d nightly
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    
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
            set(VERSION "${GIT_TAG}")
        else()
            set(VERSION "${GIT_TAG}-${GIT_COMMIT_HASH}")
        endif()
    else()
        # No tags found, use v0.0.0 with commit hash
        # set(VERSION "v0.0.0-${GIT_COMMIT_HASH}")
        
        message(FATAL_ERROR "Failed to get tag: ${GIT_TAG}")
    endif()

    set(${OUTPUT_VARIABLE} "${VERSION}" PARENT_SCOPE)
endfunction()

# Call the function and store the result
get_version_from_git(RUNCPP2_PROJECT_VERSION)
message("RUNCPP2_PROJECT_VERSION: ${RUNCPP2_PROJECT_VERSION}")

# =========================================================================
# External Dependencies
# =========================================================================

# runcpp2 doesn't rely on install at all. Disabling it.
# if(NOT DEFINED RUNCPP2_DISABLE_INSTALLATION)
#     # This variable is responsible for installation disabling.
#     set(RUNCPP2_DISABLE_INSTALLATION ON)
# 
#     # Replace install() with conditional installation.
#     macro(install)
#         if(NOT RUNCPP2_DISABLE_INSTALLATION)
#             _install(${ARGN})
#         endif()
#     endmacro()
# endif()

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/External/ssLogger")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/External/filesystem")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/External/System2")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/External/dylib")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/External/variant")

set(EXPECTED_BUILD_PACKAGE OFF)
set(BUILD_TESTING OFF)
set(DS_USE_DEBUG_BREAK ON)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/External/DSResult")

set(BUILD_TESTING ON)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/External/libyaml")
set(BUILD_TESTING OFF)

# NOTE: Just include the file. Can't add subdirectory since it install stuff
# add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/External/string-view-lite")
add_library(string-view-lite INTERFACE)
target_include_directories( string-view-lite INTERFACE 
                            "${CMAKE_CURRENT_LIST_DIR}/External/string-view-lite/include")

# Copy cfgpath.h
configure_file( "${CMAKE_CURRENT_LIST_DIR}/External/cfgpath/cfgpath.h" 
                "${CMAKE_CURRENT_LIST_DIR}/Include/cfgpath.h" COPYONLY)

if(RUNCPP2_BUILD_TESTS)
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/External/ssTest")
endif()
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/External/CppOverride")

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/External/MacroPowerToys")


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
        set(FILES_TO_EMBED  "${CMAKE_CURRENT_LIST_DIR}/DefaultYAMLs/DefaultUserConfig.yaml"
                            "DefaultUserConfig"
                            
                            "${CMAKE_CURRENT_LIST_DIR}/DefaultYAMLs/Default/CommonFileTypes.yaml"
                            "CommonFileTypes"
                            
                            "${CMAKE_CURRENT_LIST_DIR}/DefaultYAMLs/Default/g++.yaml"
                            "G_PlusPlus"
                            
                            "${CMAKE_CURRENT_LIST_DIR}/DefaultYAMLs/Default/vs2022_v17+.yaml"
                            "Vs2022_v17Plus"
                            
                            "${CMAKE_CURRENT_LIST_DIR}/DefaultYAMLs/DefaultScriptInfo.yaml"
                            "DefaultScriptInfo"
                            )

        EMBED_FILES("${EMBED_EXEC_PATH}"
                    "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/DefaultYAMLs.c"
                    "${FILES_TO_EMBED}")
    endif()
endif()

# =========================================================================
# runcpp2 library
# =========================================================================

set(RUNCPP2_SOURCE_FILES    "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/Data/Profile.cpp"
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
                            "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/LibYAML_Wrapper.cpp")

add_library(runcpp2Lib STATIC ${RUNCPP2_SOURCE_FILES})

set(RUNCPP2_PRIVATE_LINK_LIBS ssLogger System2 CppOverride dylib)
set(RUNCPP2_PUBLIC_LINK_LIBS ghc_filesystem mpark_variant MacroPowerToys string-view-lite yaml DSResult)

target_include_directories(runcpp2Lib PUBLIC "${CMAKE_CURRENT_LIST_DIR}/Include")
target_link_libraries(runcpp2Lib PRIVATE ${RUNCPP2_PRIVATE_LINK_LIBS})
target_link_libraries(runcpp2Lib PUBLIC ${RUNCPP2_PUBLIC_LINK_LIBS})

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
                                        "-Wpedantic")
    if (RUNCPP2_WARNINGS_AS_ERRORS)
        list(APPEND RUNCPP2_STANDARD_COMPILE_FLAGS "-Werror")
    endif()
endif()

target_compile_options(runcpp2Lib PRIVATE "${RUNCPP2_STANDARD_COMPILE_FLAGS}")

# Define the runcpp2 and default config version macro
target_compile_definitions(runcpp2Lib PUBLIC    RUNCPP2_VERSION="${RUNCPP2_PROJECT_VERSION}"
                                                RUNCPP2_CONFIG_VERSION=${RUNCPP2_CONFIG_VERSION})

# =========================================================================
# runcpp2 executable
# =========================================================================

add_executable(runcpp2 "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/main.cpp")
target_compile_options(runcpp2 PRIVATE "${RUNCPP2_STANDARD_COMPILE_FLAGS}")
target_link_libraries(runcpp2 PUBLIC runcpp2Lib ssLogger)

# =========================================================================
# runcpp2 library override
# =========================================================================

if(RUNCPP2_BUILD_TESTS)
    add_library(runcpp2Override STATIC ${RUNCPP2_SOURCE_FILES})
    target_include_directories(runcpp2Override PUBLIC "${CMAKE_CURRENT_LIST_DIR}/Include")
    target_link_libraries(runcpp2Override PRIVATE ${RUNCPP2_PRIVATE_LINK_LIBS})
    target_link_libraries(runcpp2Override PUBLIC ${RUNCPP2_PUBLIC_LINK_LIBS})
    target_compile_options(runcpp2Override PRIVATE "${RUNCPP2_STANDARD_COMPILE_FLAGS}")
    target_compile_definitions( runcpp2Override PUBLIC   
                                RUNCPP2_VERSION="${RUNCPP2_PROJECT_VERSION}" 
                                RUNCPP2_CONFIG_VERSION=${RUNCPP2_CONFIG_VERSION}
                                INTERNAL_RUNCPP2_UNIT_TESTS=1)
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Src/Tests")
endif()

# set(RUNCPP2_DISABLE_INSTALLATION OFF)
