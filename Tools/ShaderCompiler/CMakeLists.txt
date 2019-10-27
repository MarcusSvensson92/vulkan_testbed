# Minimum required CMake version
cmake_minimum_required(VERSION 3.8.2 FATAL_ERROR)

# Project
project(ShaderCompiler)

# Source files
file(GLOB_RECURSE SOURCE_FILES
     "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp")

# Organize source files in folder groups
get_filename_component(ABSOLUTE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/" ABSOLUTE)
foreach(SOURCE_FILE ${SOURCE_FILES})
	file(RELATIVE_PATH GROUP ${ABSOLUTE_PATH} ${SOURCE_FILE})
	string(REGEX REPLACE "(.*)(/[^/]*)$" "\\1" GROUP ${GROUP})
	string(REPLACE / \\ GROUP ${GROUP})
    source_group("${GROUP}" FILES ${SOURCE_FILE})
endforeach()

# Set output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}")

# Create executable
add_executable(${PROJECT_NAME} ${SOURCE_FILES})
