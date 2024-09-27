cmake_minimum_required(VERSION 3.0)
project(bhc3d LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -fopenmp -fpermissive")

# Add subdirectories for dependencies
add_subdirectory(thirdparty/eigen)
add_subdirectory(thirdparty/glfw)

# Include directories for Eigen, GLFW, and ImGui
include_directories(
    "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/eigen"
    "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glfw/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/imgui"
    "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/imgui/backends"
)

# Add source files for ImGui
file(GLOB IMGUI_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/imgui/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/imgui/backends/imgui_impl_glfw.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/imgui/backends/imgui_impl_opengl3.cpp"
)

# Create executable
add_executable(ex_bhc3d main.cpp ${IMGUI_SOURCES})

# Find OpenGL package
find_package(OpenGL REQUIRED)

# Link libraries
target_link_libraries(ex_bhc3d 
    glfw 
    ${OPENGL_gl_LIBRARY}
    Eigen3::Eigen
)
