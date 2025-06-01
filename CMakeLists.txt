cmake_minimum_required(VERSION 3.30)
project(ACTS LANGUAGES C CXX ASM)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Enable all compiler optimizations
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -mtune=native -Wall -pthread -Wextra -std=c++20")
# Enable ASM_NASM
enable_language(ASM_NASM)
# Add include directory
include_directories(include)

# Add assembly source file (adjust if you have more)
file(GLOB ASM_SRC "asm/*.asm")

# Add executable
add_executable(acts src/main.cpp ${ASM_SRC})
# NASM support (if using NASM)
set_property(SOURCE ${ASM_SRC} PROPERTY LANGUAGE ASM_NASM)
