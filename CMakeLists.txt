# Minimum required CMake version
cmake_minimum_required(VERSION 3.8.2 FATAL_ERROR)

# Project
project(VulkanTestbed)

# Source files
file(GLOB_RECURSE SOURCE_FILES
     "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h"
     "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp"
     "${CMAKE_CURRENT_SOURCE_DIR}/Source/Shaders/*.vert"
     "${CMAKE_CURRENT_SOURCE_DIR}/Source/Shaders/*.tesc"
     "${CMAKE_CURRENT_SOURCE_DIR}/Source/Shaders/*.tese"
     "${CMAKE_CURRENT_SOURCE_DIR}/Source/Shaders/*.geom"
     "${CMAKE_CURRENT_SOURCE_DIR}/Source/Shaders/*.frag"
     "${CMAKE_CURRENT_SOURCE_DIR}/Source/Shaders/*.comp"
	 "${CMAKE_CURRENT_SOURCE_DIR}/Source/Shaders/*.mesh"
	 "${CMAKE_CURRENT_SOURCE_DIR}/Source/Shaders/*.task"
	 "${CMAKE_CURRENT_SOURCE_DIR}/Source/Shaders/*.rgen"
	 "${CMAKE_CURRENT_SOURCE_DIR}/Source/Shaders/*.rint"
	 "${CMAKE_CURRENT_SOURCE_DIR}/Source/Shaders/*.rahit"
     "${CMAKE_CURRENT_SOURCE_DIR}/Source/Shaders/*.rchit"
     "${CMAKE_CURRENT_SOURCE_DIR}/Source/Shaders/*.rmiss"
     "${CMAKE_CURRENT_SOURCE_DIR}/Source/Shaders/*.rcall"
	 "${CMAKE_CURRENT_SOURCE_DIR}/Source/Shaders/*.glsl")

# Organize source files in folder groups
get_filename_component(ABSOLUTE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/" ABSOLUTE)
foreach(SOURCE_FILE ${SOURCE_FILES})
	file(RELATIVE_PATH GROUP ${ABSOLUTE_PATH} ${SOURCE_FILE})
	string(REGEX REPLACE "(.*)(/[^/]*)$" "\\1" GROUP ${GROUP})
	string(REPLACE / \\ GROUP ${GROUP})
    source_group("${GROUP}" FILES ${SOURCE_FILE})
endforeach()

# Update submodules
find_package(Git)
if(GIT_FOUND)
	execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
					WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif()

# Add Vulkan SDK
include_directories("$ENV{VULKAN_SDK}/Include/")

# Add cgltf
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/External/cgltf/")

# Add glfw
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/External/glfw")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/External/glfw/include/")

# Add glm
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/External/glm/glm")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/External/glm/")

# Add imgui
add_library(imgui STATIC
			"${CMAKE_CURRENT_SOURCE_DIR}/External/imgui/imgui.cpp"
			"${CMAKE_CURRENT_SOURCE_DIR}/External/imgui/imgui_demo.cpp"
			"${CMAKE_CURRENT_SOURCE_DIR}/External/imgui/imgui_draw.cpp"
			"${CMAKE_CURRENT_SOURCE_DIR}/External/imgui/imgui_widgets.cpp")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/External/imgui/")

# Add stb
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/External/stb/")

# Add volk
add_library(volk STATIC
			"${CMAKE_CURRENT_SOURCE_DIR}/External/volk/volk.c")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/External/volk/")

# Add VulkanMemoryAllocator
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/External/VulkanMemoryAllocator/src/")

# Set output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/Bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/Bin")

if(WIN32)
	# Add defines
	add_definitions(-DVK_USE_PLATFORM_WIN32_KHR -D_CRT_SECURE_NO_WARNINGS)
	
	# Add shader compilation post build step
    add_custom_target(ShaderTarget ALL DEPENDS shader_compilation)
    add_custom_command(OUTPUT shader_compilation COMMAND cmd /c ${CMAKE_CURRENT_SOURCE_DIR}/Tools/ShaderCompiler/ShaderCompiler.exe ${CMAKE_CURRENT_SOURCE_DIR}/Source/Shaders/ ${CMAKE_CURRENT_SOURCE_DIR}/Assets/Shaders/ DEPENDS always_rebuild)
    add_custom_command(OUTPUT always_rebuild COMMAND cmake -E echo)
else()
    message(FATAL_ERROR "Unsupported platform!")
endif()

# Create executable
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# Link libraries
target_link_libraries(${PROJECT_NAME} glfw imgui volk)

# Create output directory
execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_SOURCE_DIR}/Bin")

# Create shader output directory
execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_SOURCE_DIR}/Assets/Shaders")

# Set working directory for Visual Studio
set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Bin")
