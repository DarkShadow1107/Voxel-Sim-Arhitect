cmake_minimum_required(VERSION 3.20)
project(VoxelSimArchitect)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# FetchContent for Dependencies
include(FetchContent)

find_package(OpenGL REQUIRED)

# GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.3.8
)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# Use GLFW's bundled GLAD loader (avoids requiring an external loader on Windows/MinGW).
# This is the same loader used by GLFW's own examples.
include_directories(${glfw_SOURCE_DIR}/deps)

# FastNoiseLite
FetchContent_Declare(
    fastnoise
    GIT_REPOSITORY https://github.com/Auburn/FastNoiseLite.git
    GIT_TAG master
)
FetchContent_MakeAvailable(fastnoise)

# Dear ImGui
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG docking
)
FetchContent_MakeAvailable(imgui)

# Include directories
include_directories(include)
include_directories(external)
include_directories(${fastnoise_SOURCE_DIR}/Cpp)
include_directories(${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)

# Source files
set(SOURCES
    src/main.cpp
    src/Allocator.cpp
    src/TaskScheduler.cpp
    src/Compression.cpp
    src/Renderer.cpp
    src/Framebuffer.cpp
    src/World.cpp
    src/GUIManager.cpp
    src/Camera.cpp
    src/Chunk.cpp
    src/StructureGenerator.cpp
    src/GLMesh.cpp
    src/MeshBuilder.cpp
    src/Shader.cpp
    src/Texture.cpp
    src/MobAI.cpp
    src/AudioManager.cpp
    src/Registry.cpp
    ${glfw_SOURCE_DIR}/deps/glad_gl.c
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME} glfw OpenGL::GL)
if(WIN32)
    target_link_libraries(${PROJECT_NAME} psapi comdlg32)
endif()

message(STATUS "Voxel-Sim Architect project configured with FetchContent.")