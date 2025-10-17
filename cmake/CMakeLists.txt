cmake_minimum_required(VERSION 3.14)

project(LitePB 
    VERSION 1.0.2
    DESCRIPTION "Lightweight Protocol Buffer serialization library for embedded systems"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

get_filename_component(LITEPB_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)

set(LITEPB_GENERATOR "${LITEPB_ROOT}/litepb_gen" CACHE FILEPATH "Path to LitePB code generator")

add_library(litepb STATIC
    ${LITEPB_ROOT}/cpp/src/litepb/core/proto_reader.cpp
    ${LITEPB_ROOT}/cpp/src/litepb/core/proto_writer.cpp
    ${LITEPB_ROOT}/cpp/src/litepb/core/unknown_fields.cpp
)

target_include_directories(litepb
    PUBLIC
        $<BUILD_INTERFACE:${LITEPB_ROOT}/cpp/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(litepb PUBLIC cxx_std_17)

set_target_properties(litepb PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

add_library(LitePB::litepb ALIAS litepb)

install(TARGETS litepb
    EXPORT LitePBTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY ${LITEPB_ROOT}/cpp/include/litepb
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

install(PROGRAMS ${LITEPB_ROOT}/litepb_gen
    DESTINATION share/litepb
)

install(DIRECTORY ${LITEPB_ROOT}/generator
    DESTINATION share/litepb
    PATTERN ".venv" EXCLUDE
    PATTERN "__pycache__" EXCLUDE
    PATTERN "*.pyc" EXCLUDE
)

install(EXPORT LitePBTargets
    FILE LitePBTargets.cmake
    NAMESPACE LitePB::
    DESTINATION lib/cmake/LitePB
)

include(CMakePackageConfigHelpers)

set(litepb_share_dir ${CMAKE_INSTALL_PREFIX}/share/litepb)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/LitePBConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/LitePBConfig.cmake
    INSTALL_DESTINATION lib/cmake/LitePB
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/LitePBConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/LitePBConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/LitePBConfigVersion.cmake
    DESTINATION lib/cmake/LitePB
)
