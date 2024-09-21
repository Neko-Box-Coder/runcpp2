cmake_minimum_required(VERSION 3.10)

# For Clang to do parsing
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(runcpp2)

set(CMAKE_CXX_STANDARD 11)

option(RUNCPP2_UPDATE_DEFAULT_YAMLS "Update default yaml files" OFF)
option(RUNCPP2_BUILD_TESTS "Build runcpp2 tests" OFF)

# =========================================================================
# External Dependencies
# =========================================================================
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/External/ssLogger")

set(RYML_DEFAULT_CALLBACK_USES_EXCEPTIONS ON CACHE BOOL "" FORCE)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/External/rapidyaml")

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/External/filesystem")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/External/System2")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/External/dylib")

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

add_executable(runcpp2  
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/Data/Profile.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/Data/DependencyInfo.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/Data/DependencyLinkProperty.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/Data/DependencyCommands.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/Data/DependencySource.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/Data/FileProperties.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/Data/FlagsOverrideInfo.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/Data/ProfilesFlagsOverride.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/Data/ScriptInfo.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/Data/ProfilesCompilesFiles.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/Data/FilesTypesInfo.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/Data/StageInfo.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/ProfileHelper.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/CompilingLinking.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/ConfigParsing.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/DefaultYAMLs.c"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/DependenciesHelper.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/main.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/ParseUtil.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/PlatformUtil.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/runcpp2.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/StringUtil.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/BuildsManager.cpp"
                        
                        )

target_include_directories(runcpp2 PRIVATE "${CMAKE_CURRENT_LIST_DIR}/Include")

target_link_libraries(runcpp2 PRIVATE ssLogger ghc_filesystem System2 ryml::ryml dylib CppOverride)

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



# TODO: Maybe move this to the UnitTests sub folder
# Unit Tests
if(RUNCPP2_BUILD_TESTS)
    add_executable(BuildsManagerTest "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/BuildsManager.cpp"
                                     "${CMAKE_CURRENT_LIST_DIR}/Src/UnitTests/BuildsManagerTest.cpp")
                                     # "${CMAKE_CURRENT_LIST_DIR}/Src/UnitTests/BuildsManager/MockComponents.cpp")

    target_include_directories(BuildsManagerTest PRIVATE "${CMAKE_CURRENT_LIST_DIR}/Include")
    target_compile_options(BuildsManagerTest PRIVATE "${STANDARD_COMPILE_FLAGS}")
    target_link_libraries(BuildsManagerTest PRIVATE ghc_filesystem CppOverride ssTest ssLogger)
    target_compile_definitions(BuildsManagerTest PRIVATE INTERNAL_RUNCPP2_UNIT_TESTS=1)
    # set_target_properties(BuildsManagerTest PROPERTIES  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
    #                                                     LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
    #                                                     RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")
endif()



