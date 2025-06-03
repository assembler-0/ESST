cmake_minimum_required(VERSION 3.30)
project(ACTS LANGUAGES C CXX ASM)

# Option to build the lite version
option(BUILD_LITE_VERSION "Build the lite version of the stresser (uses mainLite.cpp)" OFF)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Enable all compiler optimizations
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -mtune=native -Wall -pthread -Wextra -std=c++20")

# Enable ASM_NASM
enable_language(ASM_NASM)
# Add include directory
include_directories(include)

# Add assembly source files (adjust if you have more)
file(GLOB ASM_SRC "asm/*.asm")

if(BUILD_LITE_VERSION)
    set(EXECUTABLE_NAME "esst-lite")
    set(MAIN_SOURCE_FILE "src/mainLite.cpp")
else()
    set(EXECUTABLE_NAME "esst")
    set(MAIN_SOURCE_FILE "src/main.cpp")
endif()

# Add executable
add_executable(${EXECUTABLE_NAME} ${MAIN_SOURCE_FILE} ${ASM_SRC})
# NASM support (if using NASM)
set_property(SOURCE ${ASM_SRC} PROPERTY LANGUAGE ASM_NASM)