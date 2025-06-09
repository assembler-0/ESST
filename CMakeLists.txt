cmake_minimum_required(VERSION 3.16)
project(ESST LANGUAGES C CXX ASM)

# Build options
option(BUILD_OLD_VERSION "Build the OLD version of the stresser (uses mainOld.cpp)" OFF)
option(BUILD_LIBRARY "Build as static library instead of executable" OFF)
option(ENABLE_GPU "Enable GPU/ROCm support (requires HIP)" ON)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required libraries
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

# Find compression libraries
find_package(ZLIB REQUIRED)

# Try to find LZMA
find_package(LibLZMA)
if(NOT LibLZMA_FOUND)
    # Fallback to pkg-config
    pkg_check_modules(LZMA liblzma)
    if(LZMA_FOUND)
        set(LibLZMA_FOUND TRUE)
        set(LibLZMA_LIBRARIES ${LZMA_LIBRARIES})
        set(LibLZMA_INCLUDE_DIRS ${LZMA_INCLUDE_DIRS})
    endif()
endif()

if(NOT LibLZMA_FOUND)
    message(WARNING "LZMA library not found. Compression module may not work properly.")
endif()

# Conditional GPU support
set(ENABLE_GPU_BUILD OFF)
if(ENABLE_GPU AND NOT BUILD_LIBRARY)
    # Full executable always tries to enable GPU
    set(ENABLE_GPU_BUILD ON)
elseif(ENABLE_GPU AND BUILD_LIBRARY)
    # Library with GPU support - user's choice
    set(ENABLE_GPU_BUILD ON)
endif()

# Setup GPU support if enabled
if(ENABLE_GPU_BUILD)
    # Add HIP language
    enable_language(HIP)
    list(APPEND CMAKE_PREFIX_PATH "/opt/rocm")

    # Find ROCm and HIP
    find_package(HIP QUIET)
    if(HIP_FOUND)
        message(STATUS "HIP found - GPU support enabled")
        set(CMAKE_HIP_STANDARD 20)
        set(CMAKE_HIP_STANDARD_REQUIRED ON)
        set(CMAKE_C_COMPILER hipcc)
        set(CMAKE_CXX_COMPILER hipcc)

        # HIP optimization flags
        set(CMAKE_HIP_FLAGS "${CMAKE_HIP_FLAGS} -O3 -march=native -mtune=native -Wall -pthread -Wextra -std=c++20 -use_fast_math")
    else()
        message(WARNING "HIP not found - disabling GPU support")
        set(ENABLE_GPU_BUILD OFF)
    endif()
endif()

# Compiler flags
if(ENABLE_GPU_BUILD)
    # Use HIP compiler flags
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -mtune=native -Wall -pthread -Wextra -std=c++20")
else()
    # Standard compiler optimization
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -mtune=native -Wall -pthread -Wextra -std=c++20")
endif()

# Enable NASM
enable_language(ASM_NASM)

# Include directories
include_directories(include)
if(ENABLE_GPU_BUILD)
    include_directories(rocm)
endif()

# Source files
file(GLOB MAIN_HEADER_FILE "include/core.hpp")
file(GLOB ASM_SRC "asm/*.asm")
file(GLOB EXTRA_CPP_MODULES "src/*.module.cpp")

# Conditional GPU sources
if(ENABLE_GPU_BUILD)
    file(GLOB HIP_SRC "rocm/*.hip.cpp")
    file(GLOB HIP_HEADERS "rocm/*.h")
else()
    set(HIP_SRC "")
    set(HIP_HEADERS "")
endif()

# Create compilation target
if(BUILD_LIBRARY)
    # === LIBRARY BUILD ===
    set(LIBRARY_NAME "esst")

    if(ENABLE_GPU_BUILD)
        set(LIBRARY_NAME "esst-gpu")
        message(STATUS "Building library WITH GPU support")
    else()
        set(LIBRARY_NAME "esst-cpu")
        message(STATUS "Building library WITHOUT GPU support (CPU-only)")
    endif()

    add_library(${LIBRARY_NAME} STATIC
            ${ASM_SRC}
            ${MAIN_HEADER_FILE}
            ${EXTRA_CPP_MODULES}
            ${HIP_SRC}
            ${HIP_HEADERS}
    )

    # Set ASM language
    set_property(SOURCE ${ASM_SRC} PROPERTY LANGUAGE ASM_NASM)

    # Set HIP language if GPU enabled
    if(ENABLE_GPU_BUILD AND HIP_SRC)
        set_property(SOURCE ${HIP_SRC} PROPERTY LANGUAGE HIP)
    endif()

    # Include directories for library
    target_include_directories(${LIBRARY_NAME} PRIVATE include/)
    target_include_directories(${LIBRARY_NAME} PUBLIC include/)

    if(ENABLE_GPU_BUILD)
        target_include_directories(${LIBRARY_NAME} PRIVATE rocm/)
    endif()

    # Link libraries
    target_link_libraries(${LIBRARY_NAME} PUBLIC Threads::Threads)
    target_link_libraries(${LIBRARY_NAME} PUBLIC ZLIB::ZLIB)

    if(LibLZMA_FOUND)
        if(TARGET LibLZMA::LibLZMA)
            target_link_libraries(${LIBRARY_NAME} PUBLIC LibLZMA::LibLZMA)
        else()
            target_link_libraries(${LIBRARY_NAME} PUBLIC ${LibLZMA_LIBRARIES})
            target_include_directories(${LIBRARY_NAME} PRIVATE ${LibLZMA_INCLUDE_DIRS})
        endif()
        target_compile_definitions(${LIBRARY_NAME} PRIVATE HAVE_LZMA=1)
    endif()

    # GPU-specific linking
    if(ENABLE_GPU_BUILD AND HIP_FOUND)
        target_link_libraries(${LIBRARY_NAME} PUBLIC hip::host hip::device)
        target_compile_definitions(${LIBRARY_NAME} PRIVATE ENABLE_GPU=1)
    endif()

    # Set output name
    set_target_properties(${LIBRARY_NAME} PROPERTIES OUTPUT_NAME ${LIBRARY_NAME})

else()
    # === EXECUTABLE BUILD ===
    if(BUILD_OLD_VERSION)
        set(EXECUTABLE_NAME "esstOld")
        set(MAIN_SOURCE_FILE "src/mainOld.cpp")
    else()
        set(EXECUTABLE_NAME "esst")
        set(MAIN_SOURCE_FILE "src/main.cpp")
    endif()

    message(STATUS "Building executable WITH GPU support")

    add_executable(${EXECUTABLE_NAME}
            ${MAIN_SOURCE_FILE}
            ${ASM_SRC}
            ${EXTRA_CPP_MODULES}
            ${HIP_SRC}
            ${HIP_HEADERS}
    )

    # Set ASM language
    set_property(SOURCE ${ASM_SRC} PROPERTY LANGUAGE ASM_NASM)

    # Set HIP language if GPU sources exist
    if(ENABLE_GPU_BUILD AND HIP_SRC)
        set_property(SOURCE ${HIP_SRC} PROPERTY LANGUAGE HIP)
    endif()

    # Include directories
    target_include_directories(${EXECUTABLE_NAME} PRIVATE include/)
    if(ENABLE_GPU_BUILD)
        target_include_directories(${EXECUTABLE_NAME} PRIVATE rocm/)
    endif()

    # Link libraries
    target_link_libraries(${EXECUTABLE_NAME} PUBLIC Threads::Threads)
    target_link_libraries(${EXECUTABLE_NAME} PUBLIC ZLIB::ZLIB)

    if(LibLZMA_FOUND)
        if(TARGET LibLZMA::LibLZMA)
            target_link_libraries(${EXECUTABLE_NAME} PUBLIC LibLZMA::LibLZMA)
        else()
            target_link_libraries(${EXECUTABLE_NAME} PUBLIC ${LibLZMA_LIBRARIES})
            target_include_directories(${EXECUTABLE_NAME} PRIVATE ${LibLZMA_INCLUDE_DIRS})
        endif()
        target_compile_definitions(${EXECUTABLE_NAME} PRIVATE HAVE_LZMA=1)
    endif()

    # GPU-specific linking (always enabled for executable)
    if(ENABLE_GPU_BUILD AND HIP_FOUND)
        target_link_libraries(${EXECUTABLE_NAME} PUBLIC hip::host hip::device)
        target_compile_definitions(${EXECUTABLE_NAME} PRIVATE ENABLE_GPU=1)
    endif()
endif()

# Print build configuration
message(STATUS "=== ESST Build Configuration ===")
if(BUILD_LIBRARY)
    message(STATUS "Build type: Library (${LIBRARY_NAME})")
else()
    message(STATUS "Build type: Executable (${EXECUTABLE_NAME})")
endif()
message(STATUS "GPU support: ${ENABLE_GPU_BUILD}")
message(STATUS "ZLIB found: ${ZLIB_FOUND}")
message(STATUS "LZMA found: ${LibLZMA_FOUND}")
if(ENABLE_GPU_BUILD)
    message(STATUS "HIP found: ${HIP_FOUND}")
    message(STATUS "Compiler: hipcc")
else()
    message(STATUS "Compiler: ${CMAKE_CXX_COMPILER}")
endif()
message(STATUS "==================================")