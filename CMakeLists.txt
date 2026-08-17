# Set the minimum version of CMake that can be used
cmake_minimum_required(VERSION 3.20)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Project definition
project(DiVa_FW_V2)

# Enable CMake support for ASM and C/CPP languages
enable_language(C ASM)

set(BUILD_UNIT_0_NAME DiVa_FW_V2)

# Create an executable object type
add_executable(${BUILD_UNIT_0_NAME})

# STM32CubeIDE for Visual Studio Code managed CMake file
include(cmake/vscode_generated.cmake)

# Compiler options
target_compile_options(${BUILD_UNIT_0_NAME} PRIVATE
  # Added to suppress the errors that are stopping your build
  -Wno-int-conversion 
  -Wno-discarded-qualifiers
)

# Linker options
target_link_options(${BUILD_UNIT_0_NAME} PRIVATE 
  # User defined linker options
)

# Add sources to executable/library
target_sources(${BUILD_UNIT_0_NAME} PRIVATE
  # User defined sources
)

# Add include directories
target_include_directories(${BUILD_UNIT_0_NAME} PRIVATE
  # User defined include directories
)


target_link_libraries(${BUILD_UNIT_0_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/CMSIS/DSP_DRIVERS/Lib/libarm_cortexM4lf_math.a")
# Linker options
target_link_options(${BUILD_UNIT_0_NAME} PRIVATE 
  "-L${CMAKE_CURRENT_SOURCE_DIR}/Drivers/CMSIS/DSP_DRIVERS/Lib"
)