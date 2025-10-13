cmake_minimum_required(VERSION 3.14)
project(LitePB_BasicExample)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

get_filename_component(LITEPB_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../.." ABSOLUTE)

# Add LitePB library
add_subdirectory(${LITEPB_ROOT}/cmake ${CMAKE_CURRENT_BINARY_DIR}/litepb)

# Find litepb_gen generator
find_program(LITEPB_GENERATOR litepb_gen
    HINTS ${LITEPB_ROOT}
    REQUIRED
)

# Generate C++ code from proto files
set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${GENERATED_DIR})

add_custom_command(
    OUTPUT
        ${GENERATED_DIR}/person.pb.h
        ${GENERATED_DIR}/person.pb.cpp
    COMMAND ${LITEPB_GENERATOR}
        ${CMAKE_CURRENT_SOURCE_DIR}/proto/person.proto
        -o ${GENERATED_DIR}
        -I ${CMAKE_CURRENT_SOURCE_DIR}/proto
    DEPENDS 
        ${CMAKE_CURRENT_SOURCE_DIR}/proto/person.proto
        ${LITEPB_GENERATOR}
    COMMENT "Generating LitePB C++ code from person.proto"
)

add_executable(basic_serialization
    src/main.cpp
    ${GENERATED_DIR}/person.pb.cpp
)

target_include_directories(basic_serialization PRIVATE
    ${GENERATED_DIR}
)

target_link_libraries(basic_serialization PRIVATE
    LitePB::litepb
)
