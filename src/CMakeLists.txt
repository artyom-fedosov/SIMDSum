# SPDX-License-Identifier: GPL-3.0-or-later
include(GenerateExportHeader)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

add_library(simdsum SHARED)
add_library(simdsum::simdsum ALIAS simdsum)

target_sources(simdsum
    PUBLIC
        FILE_SET CXX_MODULES
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES simdsum.cppm
    PRIVATE
        dispatch.cpp
        kernels_scalar.cpp)

# One translation unit per instruction set, each compiled with its own target flags (see kernel_loop.inc).
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|amd64|i[3-6]86)$")
    target_sources(simdsum PRIVATE kernels_sse2.cpp kernels_avx2.cpp kernels_avx512.cpp)
    set_source_files_properties(kernels_sse2.cpp PROPERTIES COMPILE_OPTIONS "-msse2")
    set_source_files_properties(kernels_avx2.cpp PROPERTIES COMPILE_OPTIONS "-mavx2")
    set_source_files_properties(kernels_avx512.cpp PROPERTIES COMPILE_OPTIONS "-mavx512f;-mavx512bw")
endif()
set_source_files_properties(kernels_scalar.cpp kernels_sse2.cpp kernels_avx2.cpp kernels_avx512.cpp
    PROPERTIES CXX_SCAN_FOR_MODULES OFF)

generate_export_header(simdsum
    BASE_NAME SIMDSUM
    EXPORT_MACRO_NAME SIMDSUM_API
    EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/simdsum_export.h)

target_include_directories(simdsum PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(simdsum PUBLIC cxx_std_23)
target_link_libraries(simdsum PRIVATE $<BUILD_INTERFACE:simdsum_options>)
target_compile_definitions(simdsum PRIVATE SIMDSUM_VERSION_STRING="${PROJECT_VERSION}")

set_target_properties(simdsum PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    CXX_MODULE_STD ON
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON)

install(TARGETS simdsum
    EXPORT simdsumTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    FILE_SET CXX_MODULES DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/simdsum/modules)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/simdsum_export.h
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/simdsum/modules)

install(EXPORT simdsumTargets
    NAMESPACE simdsum::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/simdsum
    CXX_MODULES_DIRECTORY modules)

configure_package_config_file(${PROJECT_SOURCE_DIR}/cmake/simdsumConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/simdsumConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/simdsum)
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/simdsumConfigVersion.cmake
    COMPATIBILITY SameMajorVersion)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/simdsumConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/simdsumConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/simdsum)
