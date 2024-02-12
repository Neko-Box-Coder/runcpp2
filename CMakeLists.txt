cmake_minimum_required(VERSION 3.10)

# For Clang to do parsing
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(runcpp2)

set(CMAKE_CXX_STANDARD 11)

option(RUNCPP2_UPDATE_DEFAULT_YAMLS "Update default yaml files" OFF)

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/External/CLI11")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/External/ssLogger")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/External/yaml-cpp")

# =========================================================================
# mkdirp
# =========================================================================
add_library(path_normalize "${CMAKE_CURRENT_LIST_DIR}/External/mkdirp/deps/path-normalize/path-normalize.c")
target_include_directories(path_normalize PUBLIC "${CMAKE_CURRENT_LIST_DIR}/External/mkdirp/deps")

add_library(strdup "${CMAKE_CURRENT_LIST_DIR}/External/mkdirp/deps/strdup/strdup.c")
target_include_directories(strdup PUBLIC "${CMAKE_CURRENT_LIST_DIR}/External/mkdirp/deps")

add_library(mkdirp "${CMAKE_CURRENT_LIST_DIR}/External/mkdirp/src/mkdirp.c")
target_include_directories(mkdirp PUBLIC "${CMAKE_CURRENT_LIST_DIR}/External/mkdirp/src")

target_link_libraries(mkdirp PRIVATE path_normalize strdup)


# =========================================================================
# Generate yaml files as c
# =========================================================================
if(RUNCPP2_UPDATE_DEFAULT_YAMLS)
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/External/Embed2C")
    include("${CMAKE_CURRENT_LIST_DIR}/External/Embed2C/embedFile.cmake")

    set(EMBED_EXEC_PATH "")
    GET_EXEC_PATH(EMBED_EXEC_PATH)

    set(FILES_TO_EMBED "${CMAKE_CURRENT_LIST_DIR}/DefaultYAMLs/DefaultCompilerProfiles.yaml"
                        "DefaultCompilerProfiles"
                        "${CMAKE_CURRENT_LIST_DIR}/DefaultYAMLs/DefaultScriptInfo.yaml"
                        "DefaultScriptInfo"
                        )

    EMBED_FILES("${EMBED_EXEC_PATH}"
                "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/DefaultYAMLs.c"
                "${FILES_TO_EMBED}")
endif()

# =========================================================================
# runcpp2
# =========================================================================

add_executable(runcpp2  "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/CompilerInfo.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/CompilerProfile.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/DefaultYAMLs.c"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/DependencyInfo.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/DependencySource.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/FlagsOverrideInfo.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/LinkerInfo.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/main.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/ParseUtil.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/PlatformUtil.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/runcpp2.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/ScriptInfo.cpp"
                        "${CMAKE_CURRENT_LIST_DIR}/Src/runcpp2/StringUtil.cpp"
                        )

target_include_directories(runcpp2 PRIVATE  "${CMAKE_CURRENT_LIST_DIR}/Include"
                                            "${CMAKE_CURRENT_LIST_DIR}/External/tinydir"
                                            "${CMAKE_CURRENT_LIST_DIR}/External/cfgpath")

target_link_libraries(runcpp2 PRIVATE CLI11::CLI11 ssLogger mkdirp yaml-cpp)