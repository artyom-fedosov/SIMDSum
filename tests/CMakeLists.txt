# SPDX-License-Identifier: GPL-3.0-or-later
include(FetchContent)

FetchContent_Declare(googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP ON
    EXCLUDE_FROM_ALL
    SYSTEM)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(simdsum_tests
    test_library.cpp
    test_concept.cpp
    test_add.cpp
    test_kernels.cpp
    test_dispatch.cpp)

target_link_libraries(simdsum_tests PRIVATE simdsum::simdsum simdsum_options GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(simdsum_tests DISCOVERY_TIMEOUT 60 NO_PRETTY_VALUES)
