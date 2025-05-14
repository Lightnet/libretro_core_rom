cmake_minimum_required(VERSION 3.20)
project(hello_world_core LANGUAGES C)

# Enable FetchContent
include(FetchContent)

# Fetch libretro-common for headers
FetchContent_Declare(
    libretro-common
    GIT_REPOSITORY https://github.com/libretro/libretro-common.git
    GIT_TAG master
)
FetchContent_MakeAvailable(libretro-common)

# Fetch miniz for ZIP handling
FetchContent_Declare(
    miniz
    GIT_REPOSITORY https://github.com/richgel999/miniz.git
    GIT_TAG master
)
FetchContent_MakeAvailable(miniz)

# Define the shared library
add_library(hello_world_core SHARED
    src/hello_world_core.c
    src/font.h
    ${miniz_SOURCE_DIR}/miniz.c
    ${miniz_SOURCE_DIR}/miniz_tdef.c
    ${miniz_SOURCE_DIR}/miniz_tinfl.c
    ${miniz_SOURCE_DIR}/miniz_zip.c
    # ${miniz_BINARY_DIR}
)

# Set include directories
target_include_directories(hello_world_core PRIVATE
    ${libretro-common_SOURCE_DIR}/include
    ${miniz_SOURCE_DIR}
    ${miniz_BINARY_DIR}
)

# Set compile definitions
target_compile_definitions(hello_world_core PRIVATE
    _CRT_SECURE_NO_WARNINGS
)

# Ensure the output is a .dll with no prefix
set_target_properties(hello_world_core PROPERTIES
    PREFIX ""
    OUTPUT_NAME "hello_world_core"
    SUFFIX ".dll"
)

# Set C standard
set_property(TARGET hello_world_core PROPERTY C_STANDARD 99)
# test unzip file read data size
add_executable(miniz_unzip 
  src/miniz_unzip.c
  ${miniz_SOURCE_DIR}/miniz.c
  ${miniz_SOURCE_DIR}/miniz_tdef.c
  ${miniz_SOURCE_DIR}/miniz_tinfl.c
  ${miniz_SOURCE_DIR}/miniz_zip.c
)

target_include_directories(miniz_unzip PRIVATE
    ${miniz_SOURCE_DIR}
    ${miniz_BINARY_DIR}
)

