cmake_minimum_required(VERSION 3.16)
project(gwatch LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(${CMAKE_SOURCE_DIR}/include)

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
        tests/unit/test_elf_utils.cpp
        src/memory_utils.cpp
        src/elf_utils.cpp
        src/debugger.cpp
        tests/unit/test_memory_utils.cpp
        tests/unit/test_debugger_utils.cpp
        tests/integration/test_integration_gwatch.cpp
)

target_include_directories(gwatch_tests PRIVATE src ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(gwatch_tests PRIVATE gtest_main)

add_test(NAME ELFUtilsTests COMMAND gwatch_tests)
add_test(NAME MemoryUtilsTests COMMAND gwatch_tests)
add_test(NAME DebuggerUtilsTests COMMAND gwatch_tests)

# -----------------------------------------------------------------------------
# Integration test target program
# -----------------------------------------------------------------------------
add_executable(testprog tests/integration/testprog.cpp)

# -----------------------------------------------------------------------------
# Integration test
# -----------------------------------------------------------------------------
add_executable(gwatch_integration_tests
        tests/integration/test_integration_gwatch.cpp
)
target_link_libraries(gwatch_integration_tests PRIVATE gtest_main)
target_include_directories(gwatch_integration_tests PRIVATE src)

add_test(NAME IntegrationTests COMMAND gwatch_integration_tests)