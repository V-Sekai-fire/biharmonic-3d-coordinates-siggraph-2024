project(bhc3d LANGUAGES CXX)
cmake_minimum_required(VERSION 3.0)

add_subdirectory(thirdparty/eigen)
add_subdirectory(thirdparty/polyscope)

set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty;${CMAKE_PREFIX_PATH}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -fopenmp -std=c++11 -fpermissive")

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/eigen" "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/polyscope/include")

add_executable(ex_bhc3d main.cpp)
