cmake_minimum_required(VERSION 3.20)
project(FluidSim1 VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SFML 3 COMPONENTS Graphics Window System REQUIRED)

add_executable(FluidSim
    src/main.cpp
    src/FluidSimulation.cpp
)

target_include_directories(FluidSim PRIVATE include)
target_link_libraries(FluidSim PRIVATE SFML::Graphics SFML::Window SFML::System)

if(MSVC)
    target_compile_options(FluidSim PRIVATE /W4 /permissive-)
else()
    target_compile_options(FluidSim PRIVATE -Wall -Wextra -Wpedantic)
endif()
