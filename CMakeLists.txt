cmake_minimum_required( VERSION 3.26 )
set( CMAKE_CXX_STANDARD 20 )
set(CMAKE_CXX_STANDARD_REQUIRED ON) 
set( CMAKE_BUILD_TYPE "Debug" )

project(tomchain)

# include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
# conan_basic_setup()

include_directories(include)
include_directories(src)
include_directories(src/client)
include_directories(src/server)

add_subdirectory(third_party/evmc)

