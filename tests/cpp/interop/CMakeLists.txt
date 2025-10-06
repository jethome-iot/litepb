cmake_minimum_required(VERSION 3.14)
project(litepb_interop_tests C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# CPM Package Manager auto-download
set(CPM_DOWNLOAD_VERSION 0.40.2)
set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")

if(NOT (EXISTS ${CPM_DOWNLOAD_LOCATION}))
  message(STATUS "Downloading CPM.cmake")
  file(DOWNLOAD
    https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake
    ${CPM_DOWNLOAD_LOCATION}
  )
endif()

include(${CPM_DOWNLOAD_LOCATION})

# Try to find system protobuf first (fast in CI), fall back to CPM (for local dev)
find_package(Protobuf QUIET)

if(Protobuf_FOUND)
  message(STATUS "Using system Protobuf ${Protobuf_VERSION}")
  
  # Need to find abseil too when using system protobuf
  find_package(absl QUIET)
  if(NOT absl_FOUND)
    message(STATUS "System Protobuf found but Abseil not found, downloading Abseil via CPM")
    CPMAddPackage(
      NAME abseil-cpp
      URL https://github.com/abseil/abseil-cpp/archive/refs/tags/20240722.0.tar.gz
      VERSION 20240722.0
      DOWNLOAD_EXTRACT_TIMESTAMP TRUE
      OPTIONS
        "ABSL_ENABLE_INSTALL ON"
        "ABSL_PROPAGATE_CXX_STD ON"
    )
  else()
    message(STATUS "Using system Abseil")
  endif()
else()
  message(STATUS "System Protobuf not found, downloading via CPM")
  
  # Add Abseil (must be before Protobuf)
  CPMAddPackage(
    NAME abseil-cpp
    URL https://github.com/abseil/abseil-cpp/archive/refs/tags/20240722.0.tar.gz
    VERSION 20240722.0
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    OPTIONS
      "ABSL_ENABLE_INSTALL ON"
      "ABSL_PROPAGATE_CXX_STD ON"
  )
  
  # Add Protobuf
  CPMAddPackage(
    NAME protobuf
    URL https://github.com/protocolbuffers/protobuf/archive/refs/tags/v27.4.tar.gz
    VERSION 27.4
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    OPTIONS
      "protobuf_BUILD_TESTS OFF"
      "protobuf_ABSL_PROVIDER package"
      "protobuf_BUILD_SHARED_LIBS OFF"
  )
endif()

# Fetch Unity test framework
include(FetchContent)
FetchContent_Declare(
    unity
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
    GIT_TAG v2.5.2
)
FetchContent_MakeAvailable(unity)

# Set up paths
set(LITEPB_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../..")
set(LITEPB_GENERATOR "${LITEPB_ROOT}/generator/litepb_gen.py")
set(LITEPB_INCLUDE "${LITEPB_ROOT}/cpp/include")
set(LITEPB_SRC "${LITEPB_ROOT}/cpp/src")

# Generated code output directory
set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY ${GENERATED_DIR})
file(MAKE_DIRECTORY ${GENERATED_DIR}/protoc)

# Generate protoc C++ code from test_vectors_protoc.proto
add_custom_command(
    OUTPUT 
        ${GENERATED_DIR}/protoc/test_vectors_protoc.pb.h
        ${GENERATED_DIR}/protoc/test_vectors_protoc.pb.cc
    COMMAND $<TARGET_FILE:protobuf::protoc>
        --cpp_out=${GENERATED_DIR}/protoc
        --proto_path=${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/test_vectors_protoc.proto
    DEPENDS 
        ${CMAKE_CURRENT_SOURCE_DIR}/test_vectors_protoc.proto
        protobuf::protoc
    COMMENT "Generating protoc C++ code from test_vectors_protoc.proto"
)

# Generate LitePB C++ code from test_vectors.proto
add_custom_command(
    OUTPUT 
        ${GENERATED_DIR}/test_vectors.pb.h
        ${GENERATED_DIR}/test_vectors.pb.cpp
    COMMAND python3 ${LITEPB_GENERATOR}
        ${CMAKE_CURRENT_SOURCE_DIR}/test_vectors.proto
        -o ${GENERATED_DIR}
        -I ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS 
        ${CMAKE_CURRENT_SOURCE_DIR}/test_vectors.proto
        ${LITEPB_GENERATOR}
    COMMENT "Generating LitePB C++ code from test_vectors.proto"
)

# Unity source files
set(UNITY_SRC_DIR "${unity_SOURCE_DIR}/src")

# LitePB core sources
set(LITEPB_CORE_SOURCES
    ${LITEPB_SRC}/litepb/core/proto_reader.cpp
    ${LITEPB_SRC}/litepb/core/proto_writer.cpp
)

# Create the test executable
add_executable(interop_tests
    interop.cpp
    protoc_helpers.cpp
    test_runner.cpp
    ${GENERATED_DIR}/test_vectors.pb.cpp
    ${GENERATED_DIR}/protoc/test_vectors_protoc.pb.cc
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
)

# Link libraries
# Use protobuf::libprotobuf for system installs, protobuf::protobuf for CPM builds
if(TARGET protobuf::libprotobuf)
  set(PROTOBUF_LIBRARY protobuf::libprotobuf)
else()
  set(PROTOBUF_LIBRARY protobuf::protobuf)
endif()

target_link_libraries(interop_tests PRIVATE
    ${PROTOBUF_LIBRARY}
    absl::strings
    absl::hash
    absl::base
)

# absl::log_initialize is needed for absl::InitializeLog()
# Also need log_internal libraries that protobuf uses
if(TARGET absl::log_initialize)
    target_link_libraries(interop_tests PRIVATE 
        absl::log_initialize
        absl::log_internal_check_op
        absl::log_internal_message
    )
    target_compile_definitions(interop_tests PRIVATE HAVE_ABSL_LOG)
elseif(TARGET absl::log)
    target_link_libraries(interop_tests PRIVATE absl::log)
    target_compile_definitions(interop_tests PRIVATE HAVE_ABSL_LOG)
endif()

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
