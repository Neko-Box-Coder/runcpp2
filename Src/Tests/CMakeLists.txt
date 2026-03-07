# set(INTERNAL_RUNCPP2_UNIT_TESTS_GENERIC 1)
set(INTERNAL_RUNCPP2_UNIT_TESTS_CONFIG_PARSING 2)
set(INTERNAL_RUNCPP2_UNIT_TESTS_INCLUDE_MANAGER 3)
set(INTERNAL_RUNCPP2_UNIT_TESTS_BUILDS_MANAGER 4)

function(add_unit_test_defines TEST_NAME)
    target_compile_definitions( "${TEST_NAME}" 
                                # PUBLIC INTERNAL_RUNCPP2_UNIT_TESTS_GENERIC=${INTERNAL_RUNCPP2_UNIT_TESTS_GENERIC}
                                PUBLIC INTERNAL_RUNCPP2_UNIT_TESTS_CONFIG_PARSING=${INTERNAL_RUNCPP2_UNIT_TESTS_CONFIG_PARSING}
                                PUBLIC INTERNAL_RUNCPP2_UNIT_TESTS_INCLUDE_MANAGER=${INTERNAL_RUNCPP2_UNIT_TESTS_INCLUDE_MANAGER}
                                PUBLIC INTERNAL_RUNCPP2_UNIT_TESTS_BUILDS_MANAGER=${INTERNAL_RUNCPP2_UNIT_TESTS_BUILDS_MANAGER})
endfunction()



add_executable(BuildsManagerTest "${CMAKE_CURRENT_LIST_DIR}/BuildsManagerTest.cpp")
target_link_libraries(BuildsManagerTest PRIVATE runcpp2Lib)
target_compile_definitions(BuildsManagerTest PRIVATE INTERNAL_RUNCPP2_UNIT_TESTS=${INTERNAL_RUNCPP2_UNIT_TESTS_BUILDS_MANAGER})
add_unit_test_defines(BuildsManagerTest)




add_executable(IncludeManagerTest "${CMAKE_CURRENT_LIST_DIR}/IncludeManagerTest.cpp")
target_link_libraries(IncludeManagerTest PRIVATE runcpp2Lib)
target_compile_definitions(IncludeManagerTest PRIVATE INTERNAL_RUNCPP2_UNIT_TESTS=${INTERNAL_RUNCPP2_UNIT_TESTS_INCLUDE_MANAGER})
add_unit_test_defines(IncludeManagerTest)





add_executable(ConfigParsingTest "${CMAKE_CURRENT_LIST_DIR}/ConfigParsingTest.cpp")
target_link_libraries(ConfigParsingTest PRIVATE runcpp2Lib)
target_compile_definitions(ConfigParsingTest PRIVATE INTERNAL_RUNCPP2_UNIT_TESTS=${INTERNAL_RUNCPP2_UNIT_TESTS_CONFIG_PARSING})
add_unit_test_defines(ConfigParsingTest)





function(create_data_test TEST_NAME)
    add_executable("${TEST_NAME}" "${CMAKE_CURRENT_LIST_DIR}/Data/${TEST_NAME}.cpp")
    target_compile_options("${TEST_NAME}" PRIVATE "${RUNCPP2_STANDARD_COMPILE_FLAGS}")
    target_compile_definitions("${TEST_NAME}" PRIVATE   # INTERNAL_RUNCPP2_UNIT_TESTS=${INTERNAL_RUNCPP2_UNIT_TESTS_GENERIC}
                                                        TERMCOLOR_USE_ANSI_ESCAPE_SEQUENCES=1)
    target_link_libraries("${TEST_NAME}" PUBLIC runcpp2Lib)
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

