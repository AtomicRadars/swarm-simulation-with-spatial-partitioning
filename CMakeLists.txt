cmake_minimum_required(VERSION 3.20)

project(SwarmCudaProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

find_package(GLEW QUIET)
if(NOT GLEW_FOUND)
    message(STATUS "GLEW package was not found. Fetching GLEW with FetchContent...")
    FetchContent_Declare(
        glew
        GIT_REPOSITORY https://github.com/nigels-com/glew.git
        GIT_TAG glew-2.3.1
    )
    FetchContent_MakeAvailable(glew)
endif()

find_package(OpenGL REQUIRED)

add_executable(SwarmCudaProject
    src/main.cpp
    src/app/InputController.cpp
    src/core/AgentData.cpp
    src/core/AgentInitializer.cpp
    src/cpu/CpuNaiveBackend.cpp
    src/render/OpenGLViewer.cpp
    src/benchmark/FrameStats.cpp
    src/cpu/CpuGridBackend.cpp
)

target_include_directories(SwarmCudaProject
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(SwarmCudaProject
    PRIVATE
        OpenGL::GL
        glfw
        GLEW::GLEW
)
