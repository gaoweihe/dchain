cmake_minimum_required( VERSION 3.26 )
set( CMAKE_CXX_STANDARD 20 )
set( CMAKE_CXX_STANDARD_REQUIRED ON ) 
set( CMAKE_BUILD_TYPE "Debug" )

project(tomchain)

## fetch dependencies with FetchContent
include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

# gRPC and ProtoBuf
set(ABSL_ENABLE_INSTALL ON)
set(protobuf_MODULE_COMPATIBLE ON CACHE BOOL "")
FetchContent_Declare(
  gRPC
  GIT_REPOSITORY https://github.com/grpc/grpc
  GIT_TAG        v1.55.0
)
FetchContent_MakeAvailable(gRPC)

FetchContent_Declare(
    Protobuf
    GIT_REPOSITORY https://github.com/protocolbuffers/protobuf
    GIT_TAG        v23.1 
    SOURCE_SUBDIR  cmake
)
FetchContent_MakeAvailable(Protobuf)
# Get source directory of the Protobuf
FetchContent_GetProperties(Protobuf SOURCE_DIR Protobuf_SOURCE_DIR)
# Include the script which defines 'protobuf_generate'
include(${Protobuf_SOURCE_DIR}/cmake/protobuf-generate.cmake)

set(PROTOS
    ${CMAKE_CURRENT_SOURCE_DIR}/grpc_proto/tc-server.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/grpc_proto/tc-server-peer.proto
)

add_library(grpc-proto-obj OBJECT "${CMAKE_CURRENT_SOURCE_DIR}/grpc_proto/tc-server.proto" "${CMAKE_CURRENT_SOURCE_DIR}/grpc_proto/tc-server-peer.proto")
target_link_libraries(grpc-proto-obj 
    PUBLIC 
    protobuf::libprotobuf 
    grpc++
    )
set(PROTO_BINARY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/grpc_proto")
set(PROTO_IMPORT_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/grpc_proto")

protobuf_generate(
    TARGET grpc-proto-obj
    OUT_VAR PROTO_GENERATED_FILES
    IMPORT_DIRS ${PROTO_IMPORT_DIRS}
    PROTOC_OUT_DIR "${PROTO_BINARY_DIR}")
set_source_files_properties(${PROTO_GENERATED_FILES} PROPERTIES SKIP_UNITY_BUILD_INCLUSION on)

protobuf_generate(
    TARGET grpc-proto-obj
    OUT_VAR PROTO_GENERATED_FILES
    LANGUAGE grpc
    GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
    PLUGIN "protoc-gen-grpc=${CMAKE_CURRENT_BINARY_DIR}/_deps/grpc-build/grpc_cpp_plugin"
    # PLUGIN_OPTIONS "generate_mock_code=true"
    IMPORT_DIRS ${PROTO_IMPORT_DIRS}
    PROTOC_OUT_DIR "${PROTO_BINARY_DIR}")
set_source_files_properties(${PROTO_GENERATED_FILES} PROPERTIES SKIP_UNITY_BUILD_INCLUSION on)

target_include_directories(grpc-proto-obj PUBLIC "$<BUILD_INTERFACE:${PROTO_BINARY_DIR}>")

# set(PROTO_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR}/grpc-proto-src)
# file(MAKE_DIRECTORY ${PROTO_SRC_DIR})
# include_directories(${PROTO_SRC_DIR})

# protobuf_generate(PROTO_SRCS PROTO_HDRS ${PROTO_SRC_DIR} ${PROTOS})
# grpc_generate_cpp(GRPC_SRCS GRPC_HDRS ${PROTO_SRC_DIR} ${PROTOS})

# spdlog 
FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG        v1.11.0
)
FetchContent_MakeAvailable(spdlog)

# argparse
FetchContent_Declare(
    argparse
    GIT_REPOSITORY https://github.com/p-ranav/argparse.git
    GIT_TAG        v2.9
)
FetchContent_MakeAvailable(argparse)

# json
FetchContent_Declare(
    json 
    URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
)
FetchContent_MakeAvailable(json)

include_directories(include)
include_directories(include/entity)
include_directories(src)
include_directories(src/client)
include_directories(src/server)
include_directories(grpc_proto)

# add_library(tc-grpc-server
#     ${PROTO_SRC_DIR}/tc-server.grpc.pb.h
#     ${PROTO_SRC_DIR}/tc-server.grpc.pb.cc
#     ${PROTO_SRC_DIR}/tc-server.pb.h
#     ${PROTO_SRC_DIR}/tc-server.pb.cc
#     )
# target_link_libraries(tc-grpc-server
#     grpc++
#     )
# add_library(tc-grpc-server-peer 
#     ${PROTO_SRC_DIR}/tc-server-peer.grpc.pb.h
#     ${PROTO_SRC_DIR}/tc-server-peer.grpc.pb.cc
#     ${PROTO_SRC_DIR}/tc-server-peer.pb.h
#     ${PROTO_SRC_DIR}/tc-server-peer.pb.cc
#     )
# target_link_libraries(tc-grpc-server-peer 
#     grpc++
#     )

include_directories(third_party/evmc/include)

add_library(tc-entity 
    src/entity/transaction.cpp 
    src/entity/block.cpp
    )

add_executable(tc-client 
    src/client/tc-client.cpp
    )
target_link_libraries(tc-client 
    tc-entity
    grpc-proto-obj
    argparse
    spdlog::spdlog_header_only
    nlohmann_json::nlohmann_json
    )

add_executable(tc-server 
    src/server/tc-server.cpp
    )
target_link_libraries(tc-server 
    tc-entity
    grpc-proto-obj
    argparse
    spdlog::spdlog_header_only
    nlohmann_json::nlohmann_json
    )
