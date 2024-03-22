cmake_minimum_required(VERSION 3.10)

# For Clang to do parsing
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(runcpp2)

set(CMAKE_CXX_STANDARD 11)

option(RUNCPP2_UPDATE_DEFAULT_YAMLS "Update default yaml files" OFF)

# =========================================================================
# External Dependencies
# =========================================================================
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/External/ssLogger")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/External/rapidyaml")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/External/filesystem")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/External/System2")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/External/dylib")

# Copy cfgpath.h
configure_file( "${CMAKE_CURRENT_LIST_DIR}/External/cfgpath/cfgpath.h" 
                "${CMAKE_CURRENT_LIST_DIR}/Include/cfgpath.h" COPYONLY)


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
        set(FILES_TO_EMBED "${CMAKE_CURRENT_LIST_DIR}/DefaultYAMLs/DefaultCompilerProfiles.yaml"
                            "DefaultCompilerProfiles"
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

add_executable(runcpp2  "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/Data/CompilerInfo.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/Data/CompilerProfile.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/Data/DependencyInfo.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/Data/DependencyLinkProperty.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/Data/DependencySetup.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/Data/DependencySource.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/Data/FlagsOverrideInfo.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/Data/LinkerInfo.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/Data/ScriptInfo.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/CompilerProfileHelper.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/CompilingLinking.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/ConfigParsing.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/DefaultYAMLs.c"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/DependenciesSetupHelper.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/main.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/ParseUtil.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/PlatformUtil.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/runcpp2.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/StringUtil.cpp"
                        )

target_include_directories(runcpp2 PRIVATE  "${CMAKE_CURRENT_LIST_DIR}/Include")

target_link_libraries(runcpp2 PRIVATE ssLogger ghc_filesystem System2 ryml::ryml dylib)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(STANDARD_COMPILE_FLAGS "/utf-8;/W1")
else()
    set(STANDARD_COMPILE_FLAGS  "-Wall"
                                "-Wno-return-local-addr"
                                "-Wno-sign-compare"
                                #"-Wno-unused-variable"
                                #"-Wno-unused-but-set-variable"
                                "-Wno-unused-parameter"
                                "-Wno-switch"
                                "-Wno-gnu-zero-variadic-macro-arguments"
                                "-Wextra"
                                "-pedantic"
                                "-Werror")
endif()

target_compile_options(runcpp2 PRIVATE ${STANDARD_COMPILE_FLAGS})