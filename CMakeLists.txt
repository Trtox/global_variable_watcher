cmake_minimum_required(VERSION 3.16)
project(gwatch LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# -----------------------------------------------------------------------------
# Main executable
# -----------------------------------------------------------------------------
add_executable(gwatch
        src/main.cpp
        src/args.cpp
        src/elf_utils.cpp
        src/debugger.cpp
        src/memory_utils.cpp
)

target_include_directories(gwatch PRIVATE src)

# -----------------------------------------------------------------------------
# GoogleTest setup
# -----------------------------------------------------------------------------
include(FetchContent)

FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# -----------------------------------------------------------------------------
# Tests
# -----------------------------------------------------------------------------
enable_testing()

add_executable(gwatch_tests
        tests/test_elf_utils.cpp
        src/memory_utils.cpp
        src/elf_utils.cpp
)

target_include_directories(gwatch_tests PRIVATE src)
target_link_libraries(gwatch_tests PRIVATE gtest_main)
add_test(NAME ELFUtilsTests COMMAND gwatch_tests)