# SPDX-License-Identifier: GPL-3.0-or-later
include(FetchContent)

FetchContent_Declare(benchmark
    URL https://github.com/google/benchmark/archive/refs/tags/v1.9.1.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP ON
    EXCLUDE_FROM_ALL)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(benchmark)

add_executable(simdsum_bench bench_add.cpp)
target_link_libraries(simdsum_bench PRIVATE simdsum::simdsum simdsum_options benchmark::benchmark)
