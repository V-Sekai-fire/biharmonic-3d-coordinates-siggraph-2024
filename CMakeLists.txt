cmake_minimum_required(VERSION 3.0)
project(bhc3d LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -fopenmp -fpermissive")

add_definitions(-DGLM_ENABLE_EXPERIMENTAL)

include_directories(thirdparty/polyscope_2/include thirdparty/polyscope_2/deps/imgui/imgui thirdparty/polyscope_2/deps/glm)

add_subdirectory(thirdparty/eigen)
add_subdirectory(thirdparty/polyscope_2)

add_executable(ex_bhc3d main.cpp ${IMGUI_SOURCES})

find_package(OpenGL REQUIRED)

target_link_libraries(ex_bhc3d 
    glfw 
    polyscope
    ${OPENGL_gl_LIBRARY}
    Eigen3::Eigen
)
