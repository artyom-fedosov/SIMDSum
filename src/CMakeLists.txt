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
        dispatch.cpp)

generate_export_header(simdsum
    BASE_NAME SIMDSUM
    EXPORT_MACRO_NAME SIMDSUM_API
    EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/simdsum_export.h)

target_include_directories(simdsum PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(simdsum PUBLIC cxx_std_20)
target_link_libraries(simdsum PRIVATE $<BUILD_INTERFACE:simdsum_options>)
target_compile_definitions(simdsum PRIVATE SIMDSUM_VERSION_STRING="${PROJECT_VERSION}")

set_target_properties(simdsum PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
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
