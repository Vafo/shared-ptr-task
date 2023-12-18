set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


add_library(shared_ptr_impl INTERFACE)

target_include_directories(shared_ptr_impl INTERFACE .)

target_link_libraries(shared_ptr_impl INTERFACE 
    shared_ptr_checked_delete_impl
    shared_ptr_exception_safety_impl
    shared_ptr_compiler_flags
)