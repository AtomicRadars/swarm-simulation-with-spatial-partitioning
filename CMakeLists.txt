cmake_minimum_required(VERSION 3.20)

project(SwarmCudaProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(SwarmCudaProject
    src/main.cpp
    src/core/AgentData.cpp
    src/core/AgentInitializer.cpp
)

target_include_directories(SwarmCudaProject
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)