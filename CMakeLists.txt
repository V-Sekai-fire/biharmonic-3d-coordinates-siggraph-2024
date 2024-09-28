cmake_minimum_required(VERSION 3.0)
project(bhc3d LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -fpermissive") # Removed -fopenmp optimization.

add_subdirectory(thirdparty/eigen)

add_executable(ex_bhc3d 
    main.cpp
    ${IMGUI_SOURCES}
)

find_package(OpenGL REQUIRED)

target_link_libraries(ex_bhc3d 
    Eigen3::Eigen
)
