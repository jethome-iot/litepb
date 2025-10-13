cmake_minimum_required(VERSION 3.14)
project(litepb_interop_tests C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required system libraries
find_package(PkgConfig REQUIRED)

# Find Protobuf
pkg_check_modules(PROTOBUF REQUIRED protobuf)
message(STATUS "Found Protobuf ${PROTOBUF_VERSION} via pkg-config")

# Find Abseil
pkg_check_modules(ABSL_BASE REQUIRED absl_base)
pkg_check_modules(ABSL_STRINGS REQUIRED absl_strings)
pkg_check_modules(ABSL_HASH REQUIRED absl_hash)
pkg_check_modules(ABSL_LOG QUIET absl_log)
pkg_check_modules(ABSL_LOG_INITIALIZE QUIET absl_log_initialize)
message(STATUS "Found Abseil via pkg-config")

# Find protoc executable
find_program(PROTOC_EXECUTABLE protoc REQUIRED)
if(NOT PROTOC_EXECUTABLE)
  message(FATAL_ERROR "protoc executable not found. Please install protobuf compiler:\n"
          "  Ubuntu/Debian: apt install protobuf-compiler\n"
          "  macOS: brew install protobuf\n"
          "  Fedora: dnf install protobuf-compiler")
endif()
message(STATUS "Found protoc: ${PROTOC_EXECUTABLE}")

# Create a target for protoc
add_executable(protobuf::protoc IMPORTED)
set_target_properties(protobuf::protoc PROPERTIES
  IMPORTED_LOCATION ${PROTOC_EXECUTABLE}
)

# Fetch Unity test framework (lightweight, OK to download)
include(FetchContent)
FetchContent_Declare(
    unity
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
    GIT_TAG v2.5.2
)
FetchContent_MakeAvailable(unity)

# Set up paths
set(LITEPB_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../..")
set(LITEPB_GENERATOR "${LITEPB_ROOT}/litepb_gen")
set(LITEPB_INCLUDE "${LITEPB_ROOT}/cpp/include")
set(LITEPB_SRC "${LITEPB_ROOT}/cpp/src")

# Generated code output directory
set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY ${GENERATED_DIR})
file(MAKE_DIRECTORY ${GENERATED_DIR}/protoc)

# Generate protoc C++ code from test_vectors.proto
add_custom_command(
    OUTPUT 
        ${GENERATED_DIR}/protoc/test_vectors.pb.h
        ${GENERATED_DIR}/protoc/test_vectors.pb.cc
    COMMAND $<TARGET_FILE:protobuf::protoc>
        --cpp_out=${GENERATED_DIR}/protoc
        --proto_path=${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/test_vectors.proto
    DEPENDS 
        ${CMAKE_CURRENT_SOURCE_DIR}/test_vectors.proto
        protobuf::protoc
    COMMENT "Generating protoc C++ code from test_vectors.proto"
)

# Generate LitePB C++ code from test_vectors.proto with namespace prefix
add_custom_command(
    OUTPUT 
        ${GENERATED_DIR}/test_vectors.pb.h
        ${GENERATED_DIR}/test_vectors.pb.cpp
    COMMAND ${LITEPB_GENERATOR}
        ${CMAKE_CURRENT_SOURCE_DIR}/test_vectors.proto
        -o ${GENERATED_DIR}
        -I ${CMAKE_CURRENT_SOURCE_DIR}
        --namespace-prefix=litepb_
    DEPENDS 
        ${CMAKE_CURRENT_SOURCE_DIR}/test_vectors.proto
        ${LITEPB_GENERATOR}
    COMMENT "Generating LitePB C++ code from test_vectors.proto with namespace prefix"
)

# Unity source files
set(UNITY_SRC_DIR "${unity_SOURCE_DIR}/src")

# LitePB core sources
set(LITEPB_CORE_SOURCES
    ${LITEPB_SRC}/litepb/core/proto_reader.cpp
    ${LITEPB_SRC}/litepb/core/proto_writer.cpp
    ${LITEPB_SRC}/litepb/core/unknown_fields.cpp
)

# Create the test executable
add_executable(interop_tests
    interop.cpp
    protoc_helpers.cpp
    test_runner.cpp
    ${GENERATED_DIR}/test_vectors.pb.cpp
    ${GENERATED_DIR}/protoc/test_vectors.pb.cc
    ${LITEPB_CORE_SOURCES}
    ${UNITY_SRC_DIR}/unity.c
)

# Include directories
target_include_directories(interop_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${GENERATED_DIR}
    ${GENERATED_DIR}/protoc
    ${LITEPB_INCLUDE}
    ${UNITY_SRC_DIR}
    ${PROTOBUF_INCLUDE_DIRS}
)

# Link libraries
target_link_directories(interop_tests PRIVATE 
    ${PROTOBUF_LIBRARY_DIRS}
    ${ABSL_BASE_LIBRARY_DIRS}
    ${ABSL_STRINGS_LIBRARY_DIRS}
    ${ABSL_HASH_LIBRARY_DIRS}
)

target_link_libraries(interop_tests PRIVATE 
    ${PROTOBUF_LIBRARIES}
    ${ABSL_BASE_LIBRARIES}
    ${ABSL_STRINGS_LIBRARIES} 
    ${ABSL_HASH_LIBRARIES}
)

# Add abseil log if available
if(ABSL_LOG_FOUND)
    target_link_directories(interop_tests PRIVATE ${ABSL_LOG_LIBRARY_DIRS})
    target_link_libraries(interop_tests PRIVATE ${ABSL_LOG_LIBRARIES})
    target_compile_definitions(interop_tests PRIVATE HAVE_ABSL_LOG)
endif()

# Add abseil log_initialize if available (required on some systems)
if(ABSL_LOG_INITIALIZE_FOUND)
    target_link_directories(interop_tests PRIVATE ${ABSL_LOG_INITIALIZE_LIBRARY_DIRS})
    target_link_libraries(interop_tests PRIVATE ${ABSL_LOG_INITIALIZE_LIBRARIES})
endif()

# Compile options
target_compile_options(interop_tests PRIVATE ${PROTOBUF_CFLAGS_OTHER})

# Set preprocessor defines for Unity configuration
target_compile_definitions(interop_tests PRIVATE
    UNITY_INCLUDE_DOUBLE
    UNITY_SUPPORT_64
)

# Create a custom target for running tests
add_custom_target(run_tests
    COMMAND interop_tests
    DEPENDS interop_tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running interop tests"
)