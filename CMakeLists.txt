cmake_minimum_required( VERSION 3.26 )
set( CMAKE_CXX_STANDARD 20 )
set( CMAKE_CXX_STANDARD_REQUIRED ON ) 
set( CMAKE_BUILD_TYPE "Debug" )

project(tomchain)

# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/debug)
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/release)

## fetch dependencies with FetchContent
include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

# gRPC
set(ABSL_ENABLE_INSTALL ON)
set(protobuf_MODULE_COMPATIBLE ON CACHE BOOL "")
FetchContent_Declare(
  gRPC
  GIT_REPOSITORY https://github.com/grpc/grpc
  GIT_TAG        v1.55.0
)
FetchContent_MakeAvailable(gRPC)

# ProtoBuf (cmake only)
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

add_library(grpc-proto-obj SHARED 
    OBJECT "${CMAKE_CURRENT_SOURCE_DIR}/grpc_proto/tc-server.proto" "${CMAKE_CURRENT_SOURCE_DIR}/grpc_proto/tc-server-peer.proto")
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
    PLUGIN_OPTIONS "generate_mock_code=false"
    IMPORT_DIRS ${PROTO_IMPORT_DIRS}
    PROTOC_OUT_DIR "${PROTO_BINARY_DIR}")

target_include_directories(grpc-proto-obj PUBLIC "$<BUILD_INTERFACE:${PROTO_BINARY_DIR}>")

# spdlog 
FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG        v1.11.0
)
FetchContent_MakeAvailable(spdlog)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/build/_deps/spdlog-src/include)

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

# ecdsa_cxx
FetchContent_Declare(
    ecdsa_cxx
    GIT_REPOSITORY https://github.com/gladosconn/ecdsa_cxx.git
)
FetchContent_MakeAvailable(ecdsa_cxx)

# PicoSHA2
FetchContent_Declare(
    PicoSHA2
    GIT_REPOSITORY https://github.com/okdshin/PicoSHA2.git
)
FetchContent_MakeAvailable(PicoSHA2)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/build/_deps/picosha2-src)

# oneTBB
FetchContent_Declare(
    oneTBB
    GIT_REPOSITORY https://github.com/oneapi-src/oneTBB.git
    GIT_TAG v2021.9.0
)
FetchContent_MakeAvailable(oneTBB)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/build/_deps/onetbb-src/include)

# alpaca
FetchContent_Declare(
    alpaca
    GIT_REPOSITORY https://github.com/p-ranav/alpaca.git
)
FetchContent_MakeAvailable(alpaca)

# msgpack
FetchContent_Declare(
    msgpack
    GIT_REPOSITORY https://github.com/msgpack/msgpack-c.git
    GIT_TAG cpp_master
)
FetchContent_MakeAvailable(msgpack)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/build/_deps/msgpack-src/include)

# easy_profiler 
FetchContent_Declare(
    easy_profiler
    GIT_REPOSITORY https://github.com/yse/easy_profiler.git
    GIT_TAG v2.1.0
)
FetchContent_MakeAvailable(easy_profiler)

# flatbuffers
FetchContent_Declare(
    flatbuffers
    GIT_REPOSITORY https://github.com/google/flatbuffers.git
)
FetchContent_MakeAvailable(flatbuffers)
execute_process(
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/_deps/flatbuffers-build/flatc --cpp --grpc --gen-mutable -o ${CMAKE_CURRENT_SOURCE_DIR}/fb_schema ${CMAKE_CURRENT_SOURCE_DIR}/fb_schema/consensus.fbs
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  RESULT_VARIABLE FLATBUFFERS_RESULT
)
include_directories(${FLATBUFFERS_SRC_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/_deps/flatbuffers-src/include
    ${CMAKE_CURRENT_BINARY_DIR}/fb_schema 
    )

set(LIBBLS_SOURCES_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/third_party/libBLS/deps)
set(LIBBLS_INSTALL_ROOT ${LIBBLS_SOURCES_ROOT}/deps_inst/x86_or_x64)

include_directories(
    ${LIBBLS_INSTALL_ROOT}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/libBLS 
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/libBLS/threshold_encryption
    )
set(CMAKE_PREFIX_PATH ${LIBBLS_INSTALL_ROOT})

find_library(BLS_LOC bls PATHS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/libBLS/build)
find_library( GMP_LIBRARY NAMES "gmp" PATHS "${LIBBLS_INSTALL_ROOT}/lib" )
find_library( GMPXX_LIBRARY NAMES "gmpxx" PATHS "${LIBBLS_INSTALL_ROOT}/lib" )
find_library( SSL_LIB NAMES "ssl" PATHS "${LIBBLS_INSTALL_ROOT}/lib")
find_library( CRYPTO_LIB NAMES "crypto"  PATHS "${LIBBLS_INSTALL_ROOT}/lib")
find_library( FF_LIB NAMES NAMES "ff" PATHS "${LIBBLS_INSTALL_ROOT}/lib")

include_directories(include)
include_directories(include/entity)
include_directories(src)
include_directories(src/client)
include_directories(src/server)
include_directories(grpc_proto)
include_directories(fb_schema)

include_directories(third_party)
include_directories(third_party/hashmap/inc)
include_directories(third_party/evmc/include)
include_directories(build/_deps/ecdsa_cxx-src/src)
include_directories(third_party/c-plus-plus-serializer)

include_directories(deps/folly/include)
link_directories(deps/folly/lib)

# Data structures 
add_library(tc-entity
    src/entity/transaction.cpp 
    src/entity/block.cpp
    )
target_link_libraries(tc-entity 
    nlohmann_json::nlohmann_json
    TBB::tbb
    picosha2
    easy_profiler
    ${BLS_LOC} ${FF_LIB} ${GMPXX_LIBRARY} ${GMP_LIBRARY} ${BOOST_LIBS_4_BLS} ${CRYPTO_LIB} ${SSL_LIB}
    )

# TomChain client 
add_executable(tc-client 
    src/client/tc-client.cpp
    )
target_link_libraries(tc-client 
    tc-entity
    grpc-proto-obj
    argparse
    spdlog::spdlog_header_only
    nlohmann_json::nlohmann_json
    ecdsa++
    alpaca
    picosha2
    flatbuffers
    easy_profiler
    ${BLS_LOC} ${FF_LIB} ${GMPXX_LIBRARY} ${GMP_LIBRARY} ${BOOST_LIBS_4_BLS} ${CRYPTO_LIB} ${SSL_LIB}
    TBB::tbb
    )

# TomChain server 
add_executable(tc-server 
    src/server/tc-server.cpp
    )
target_link_libraries(tc-server 
    tc-entity
    grpc-proto-obj
    argparse
    spdlog::spdlog_header_only
    nlohmann_json::nlohmann_json
    ecdsa++
    alpaca
    picosha2
    flatbuffers
    easy_profiler
    ${BLS_LOC} ${FF_LIB} ${GMPXX_LIBRARY} ${GMP_LIBRARY} ${BOOST_LIBS_4_BLS} ${CRYPTO_LIB} ${SSL_LIB}
    TBB::tbb
    )

# tests
add_executable(test_msgpack
    test/test_msgpack.cpp
    )
target_link_libraries(test_msgpack
    tc-entity
    ${BLS_LOC} ${FF_LIB} ${GMPXX_LIBRARY} ${GMP_LIBRARY} ${BOOST_LIBS_4_BLS} ${CRYPTO_LIB} ${SSL_LIB}
    TBB::tbb
    )

add_executable(test_chm
    test/test_chm.cpp
    )
target_link_libraries(test_chm
    tc-entity   
    )
add_executable(test_parallel_merge
    test/test_parallel_merge.cpp
    )
target_link_libraries(test_parallel_merge
    ${BLS_LOC} ${FF_LIB} ${GMPXX_LIBRARY} ${GMP_LIBRARY} ${BOOST_LIBS_4_BLS} ${CRYPTO_LIB} ${SSL_LIB}
    TBB::tbb
)
add_executable(test_flatbuffers
    test/test_flatbuffers.cpp
    )
target_link_libraries(test_flatbuffers
    ${BLS_LOC} ${FF_LIB} ${GMPXX_LIBRARY} ${GMP_LIBRARY} ${BOOST_LIBS_4_BLS} ${CRYPTO_LIB} ${SSL_LIB}
    TBB::tbb
    flatbuffers
)