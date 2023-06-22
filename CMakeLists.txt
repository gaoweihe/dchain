cmake_minimum_required( VERSION 3.26 )
set( CMAKE_CXX_STANDARD 20 )
set(CMAKE_CXX_STANDARD_REQUIRED ON) 
set( CMAKE_BUILD_TYPE "Debug" )

project(tomchain)

# include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
# conan_basic_setup()

include_directories(include)
include_directories(include/entity)
include_directories(src)
include_directories(src/client)
include_directories(src/server)

include_directories(third_party/evmc/include)

add_library(tc-entity 
    src/entity/transaction.cpp 
    src/entity/block.cpp
    )

add_executable(tc-client 
    src/client/tc-client.cpp
    )
target_link_libraries(tc-client tc-entity)

add_executable(tc-server 
    src/server/tc-server.cpp
    )
target_link_libraries(tc-server tc-entity)
