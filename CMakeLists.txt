cmake_minimum_required(VERSION 3.15)

project(shared_ptr)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(shared_ptr_compiler_flags INTERFACE)
target_compile_features(shared_ptr_compiler_flags INTERFACE cxx_std_20)

# Include src folders
add_subdirectory(src)
add_subdirectory(util)
add_subdirectory(test)

include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.3.0
)

FetchContent_MakeAvailable(Catch2)

