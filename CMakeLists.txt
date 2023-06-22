cmake_minimum_required( VERSION 3.26 )
set( CMAKE_CXX_STANDARD 20 )
set( CMAKE_CXX_STANDARD_REQUIRED ON ) 
set( CMAKE_BUILD_TYPE "Debug" )

project(tomchain)

include(FetchContent)
set(ABSL_ENABLE_INSTALL ON)
set(FETCHCONTENT_QUIET OFF)
FetchContent_Declare(
  gRPC
  GIT_REPOSITORY https://github.com/grpc/grpc
  GIT_TAG        v1.55.0
)
FetchContent_MakeAvailable(gRPC)
FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG        v1.11.0
)
FetchContent_MakeAvailable(spdlog)
FetchContent_Declare(
    argparse
    GIT_REPOSITORY https://github.com/p-ranav/argparse.git
    GIT_TAG        v2.9
)
FetchContent_MakeAvailable(argparse)

include_directories(include)
include_directories(include/entity)
include_directories(src)
include_directories(src/client)
include_directories(src/server)
include_directories(grpc_proto)

add_library(tc-grpc-server
    grpc_proto/tc-server.grpc.pb.h
    grpc_proto/tc-server.grpc.pb.cc
    grpc_proto/tc-server.pb.h
    grpc_proto/tc-server.pb.cc
    )
target_link_libraries(tc-grpc-server
    grpc++
    )
add_library(tc-grpc-server-peer 
    grpc_proto/tc-server-peer.grpc.pb.h
    grpc_proto/tc-server-peer.grpc.pb.cc
    grpc_proto/tc-server-peer.pb.h
    grpc_proto/tc-server-peer.pb.cc
    )
target_link_libraries(tc-grpc-server-peer 
    grpc++
    )

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
    tc-grpc-server
    argparse
    spdlog::spdlog_header_only
    )

add_executable(tc-server 
    src/server/tc-server.cpp
    )
target_link_libraries(tc-server 
    tc-entity
    tc-grpc-server
    tc-grpc-server-peer 
    argparse
    spdlog::spdlog_header_only
    )
