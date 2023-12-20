set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(checked_delete)
add_subdirectory(exception_safety)

add_library(shared_ptr_util_impl INTERFACE)

target_link_libraries(shared_ptr_util_impl INTERFACE
    shared_ptr_checked_delete_impl
    shared_ptr_exception_safety_impl
)