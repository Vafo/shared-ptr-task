set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(shared_ptr_test_suite OBJECT
    shared_ptr_test.cpp
    util_test.cpp
)

target_link_libraries(shared_ptr_test_suite Catch2::Catch2)
target_link_libraries(shared_ptr_test_suite shared_ptr_impl shared_ptr_compiler_flags)

add_executable(shared_ptr_test $<TARGET_OBJECTS:shared_ptr_test_suite>)

# Catch2
target_link_libraries(shared_ptr_test shared_ptr_test_suite Catch2::Catch2WithMain)

# Output to build dir
set_target_properties(shared_ptr_test 
    PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)