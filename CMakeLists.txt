# SPDX-License-Identifier: GPL-3.0-or-later
cmake_minimum_required(VERSION 4.3)

# `import std;` is still experimental in CMake; the gate value is specific to each CMake release.
if(CMAKE_VERSION VERSION_GREATER_EQUAL 4.4)
    message(FATAL_ERROR "CMake ${CMAKE_VERSION}: update the CMAKE_EXPERIMENTAL_CXX_IMPORT_STD gate for this release")
endif()
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "451f2fe2-a8a2-47c3-bc32-94786d8fc91b")

project(simdsum
    VERSION 1.0.0
    DESCRIPTION "Fast SIMD element-wise array addition as a C++ module-based shared library"
    LANGUAGES CXX)

option(SIMDSUM_BUILD_TESTS "Build the GTest test suite" ${PROJECT_IS_TOP_LEVEL})
option(SIMDSUM_BUILD_BENCHMARKS "Build the Google Benchmark suite" OFF)
option(SIMDSUM_SANITIZE "Build with AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

if(NOT CMAKE_GENERATOR MATCHES "Ninja")
    message(FATAL_ERROR "C++ modules require the Ninja generator: cmake -G Ninja ...")
endif()

if(NOT "23" IN_LIST CMAKE_CXX_COMPILER_IMPORT_STD)
    message(FATAL_ERROR "${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION} has no usable `import std;` for C++23 "
                        "(needs GCC >= 15 or Clang >= 18.1.2 with a standard library module)")
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Common warning / sanitizer settings for every target we own.
add_library(simdsum_options INTERFACE)
target_compile_options(simdsum_options INTERFACE -Wall -Wextra -Wpedantic -Werror
    # CMake passes -c alongside --precompile when Clang builds module BMIs.
    $<$<CXX_COMPILER_ID:Clang>:-Wno-unused-command-line-argument>)
if(SIMDSUM_SANITIZE)
    target_compile_options(simdsum_options INTERFACE
        -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer)
    target_link_options(simdsum_options INTERFACE -fsanitize=address,undefined)
endif()

add_subdirectory(src)

if(SIMDSUM_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(SIMDSUM_BUILD_BENCHMARKS)
    add_subdirectory(bench)
endif()
