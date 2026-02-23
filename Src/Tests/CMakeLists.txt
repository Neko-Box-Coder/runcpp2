add_executable(BuildsManagerTest "${CMAKE_CURRENT_LIST_DIR}/../runcpp2/BuildsManager.cpp"
                                 "${CMAKE_CURRENT_LIST_DIR}/BuildsManagerTest.cpp")

target_include_directories(BuildsManagerTest PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../Include")
target_compile_options(BuildsManagerTest PRIVATE "${RUNCPP2_STANDARD_COMPILE_FLAGS}")
target_link_libraries(BuildsManagerTest PRIVATE ghc_filesystem CppOverride ssLogger DSResult)
target_compile_definitions(BuildsManagerTest PRIVATE INTERNAL_RUNCPP2_UNIT_TESTS=1)

add_executable(IncludeManagerTest "${CMAKE_CURRENT_LIST_DIR}/../runcpp2/IncludeManager.cpp"
                                 "${CMAKE_CURRENT_LIST_DIR}/IncludeManagerTest.cpp")

target_include_directories(IncludeManagerTest PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../Include")
target_compile_options(IncludeManagerTest PRIVATE "${RUNCPP2_STANDARD_COMPILE_FLAGS}")
target_link_libraries(IncludeManagerTest PRIVATE ghc_filesystem CppOverride ssLogger DSResult)
target_compile_definitions(IncludeManagerTest PRIVATE INTERNAL_RUNCPP2_UNIT_TESTS=1)


add_executable(ConfigParsingTest "${CMAKE_CURRENT_LIST_DIR}/ConfigParsingTest.cpp")
target_include_directories(ConfigParsingTest PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../Include")
target_compile_options(ConfigParsingTest PRIVATE "${RUNCPP2_STANDARD_COMPILE_FLAGS}")
target_link_libraries(ConfigParsingTest PRIVATE ghc_filesystem CppOverride ssLogger runcpp2Override DSResult)
target_compile_definitions(ConfigParsingTest PRIVATE INTERNAL_RUNCPP2_UNIT_TESTS=1)


function(create_data_test TEST_NAME)
    add_executable("${TEST_NAME}" "${CMAKE_CURRENT_LIST_DIR}/Data/${TEST_NAME}.cpp")
    target_compile_options("${TEST_NAME}" PRIVATE "${RUNCPP2_STANDARD_COMPILE_FLAGS}")
    target_compile_definitions("${TEST_NAME}" PRIVATE INTERNAL_RUNCPP2_UNIT_TESTS=1 TERMCOLOR_USE_ANSI_ESCAPE_SEQUENCES=1)
    target_link_libraries("${TEST_NAME}" PUBLIC runcpp2Lib ssLogger)
endfunction()

create_data_test(BuildTypeTest)
create_data_test(DependencyInfoTest)
create_data_test(DependencySourceTest)
create_data_test(ProfileTest)
create_data_test(ScriptInfoTest)

if(WIN32)
    execute_process(COMMAND ${CMAKE_COMMAND} -E copy    "${CMAKE_CURRENT_LIST_DIR}/RunAllTests.bat" 
                                                        "${CMAKE_CURRENT_BINARY_DIR}")
else()
    file(   CHMOD "${CMAKE_CURRENT_LIST_DIR}/RunAllTests.sh"
            PERMISSIONS  OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_WRITE WORLD_EXECUTE)
    execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink  "${CMAKE_CURRENT_LIST_DIR}/RunAllTests.sh" 
                                                                "${CMAKE_CURRENT_BINARY_DIR}/RunAllTests.sh")
endif()

