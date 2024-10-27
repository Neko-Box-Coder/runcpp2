add_executable(BuildsManagerTest "${CMAKE_CURRENT_LIST_DIR}/../runcpp2/BuildsManager.cpp"
                                 "${CMAKE_CURRENT_LIST_DIR}/BuildsManagerTest.cpp")
                                 # "${CMAKE_CURRENT_LIST_DIR}/../Tests/BuildsManager/MockComponents.cpp")

target_include_directories(BuildsManagerTest PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../Include")
target_compile_options(BuildsManagerTest PRIVATE "${STANDARD_COMPILE_FLAGS}")
target_link_libraries(BuildsManagerTest PRIVATE ghc_filesystem CppOverride ssTest ssLogger)
target_compile_definitions(BuildsManagerTest PRIVATE INTERNAL_RUNCPP2_UNIT_TESTS=1)
# set_target_properties(BuildsManagerTest PROPERTIES  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
#                                                     LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
#                                                     RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")
