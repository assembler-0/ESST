cmake_minimum_required(VERSION 3.30)
project(ACTS LANGUAGES C CXX ASM)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optimization flags (Release mode)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -mtune=native -funroll-loops -ffast-math")
set(CMAKE_C_FLAGS_RELEASE "-O3 -march=native -mtune=native -funroll-loops -ffast-math")
set(CMAKE_ASM_FLAGS_RELEASE "-O3 -march=native -mtune=native")
find_package(Threads REQUIRED)
# Include headers
include_directories(${CMAKE_SOURCE_DIR}/include)

# Assembly source
enable_language(ASM_NASM)
set(ASM_SOURCES
    ${CMAKE_SOURCE_DIR}/asm/avx_stress.asm
)

# C++ source
set(CPP_SOURCES
    ${CMAKE_SOURCE_DIR}/src/main.cpp
)

# Executable target
add_executable(ACTS ${CPP_SOURCES})

# Optional: Rename the output binary
set_target_properties(ACTS PROPERTIES OUTPUT_NAME "acts")
