cmake_minimum_required(VERSION 3.20)

project(SwarmCudaProject LANGUAGES C CXX CUDA)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)

include(FetchContent)

find_package(glfw3 CONFIG QUIET)

if(NOT glfw3_FOUND)
    message(STATUS "glfw3 package was not found. Fetching GLFW with FetchContent...")

    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4
    )

    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(glfw)
endif()

find_package(OpenGL REQUIRED)

add_library(glad STATIC
    external/glad/src/glad.c
)

target_include_directories(glad
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/external/glad/include
)

add_executable(SwarmCudaProject
    src/main.cpp
    src/app/InputController.cpp
    src/core/AgentData.cpp
    src/core/AgentInitializer.cpp
    src/cpu/CpuNaiveBackend.cpp
    src/cpu/CpuGridBackend.cpp
    src/render/OpenGLViewer.cpp
    src/render/AgentRenderer.cpp
    src/render/Shader.cpp
    src/benchmark/FrameStats.cpp
    src/gpu/CudaSmokeTest.cu
    src/gpu/CudaAgentData.cu
    src/gpu/CudaNaiveBackend.cu
)

target_include_directories(SwarmCudaProject
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(SwarmCudaProject
    PRIVATE
        glad
        OpenGL::GL
        glfw
)

set_target_properties(SwarmCudaProject
    PROPERTIES
        CUDA_ARCHITECTURES 75
)