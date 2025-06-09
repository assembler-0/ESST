cmake_minimum_required(VERSION 3.15)
project(ESST LANGUAGES C CXX ASM)

# General compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -mtune=native -Wall -Wextra -pthread -std=c++20")

# Check OS and set compiler
if(UNIX AND NOT APPLE)
    message(STATUS "Detected Linux")

    # Find MinGW cross-compiler (C/C++ only)
    find_program(MINGW_GPP NAMES x86_64-w64-mingw32-g++)
    find_program(MINGW_GCC NAMES x86_64-w64-mingw32-gcc)

    # Find NASM separately
    find_program(NASM NAMES nasm)

    if(MINGW_GPP AND MINGW_GCC AND NASM)
        message(STATUS "Using MinGW g++: ${MINGW_GPP}")
        message(STATUS "Using NASM: ${NASM}")

        # Set C/C++ compilers
        set(CMAKE_CXX_COMPILER ${MINGW_GPP})
        set(CMAKE_C_COMPILER ${MINGW_GCC})

        # Configure NASM separately
        set(CMAKE_ASM_NASM_COMPILER ${NASM})
        set(CMAKE_ASM_NASM_FLAGS "-f win64")
        enable_language(ASM_NASM)


    else()
        message(FATAL_ERROR "Required tools not found: "
                "MinGW g++: ${MINGW_GPP}, "
                "MinGW gcc: ${MINGW_GCC}, "
                "NASM: ${NASM}")
    endif()
else()
    message(STATUS "Non-Linux system detected. Using default compiler.")
endif()

# Source files - separate C++ and ASM files
file(GLOB_RECURSE CPP_SOURCES src/*.cpp)
file(GLOB_RECURSE ASM_SOURCES asm/*.asm)

# Ignore old main
list(FILTER CPP_SOURCES EXCLUDE REGEX "mainOld.cpp")

# Include directories
include_directories(include)

# Executable
add_executable(esst.exe "src/main.cpp" ${CPP_SOURCES} ${ASM_SOURCES})

# Explicitly set NASM for assembly files
if(CMAKE_ASM_NASM_COMPILER)
    set_source_files_properties(${ASM_SOURCES} PROPERTIES
            LANGUAGE ASM_NASM
            COMPILE_FLAGS "-f win64"
    )
endif()

# Windows-specific linker flags
if(MINGW_GPP)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
endif()