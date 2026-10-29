# SPDX-License-Identifier: GPL-3.0-or-later
cmake_minimum_required(VERSION 3.28)

project(simdsum
    VERSION 1.0.0
    DESCRIPTION "Fast SIMD element-wise array addition as a C++20 shared library"
    LANGUAGES CXX)

option(SIMDSUM_BUILD_TESTS "Build the GTest test suite" ${PROJECT_IS_TOP_LEVEL})
option(SIMDSUM_BUILD_BENCHMARKS "Build the Google Benchmark suite" OFF)
option(SIMDSUM_SANITIZE "Build with AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

if(NOT CMAKE_GENERATOR MATCHES "Ninja")
    message(FATAL_ERROR "C++20 modules require the Ninja generator: cmake -G Ninja ...")
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Common warning / sanitizer settings for every target we own.
add_library(simdsum_options INTERFACE)
target_compile_options(simdsum_options INTERFACE -Wall -Wextra -Wpedantic -Werror)
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
