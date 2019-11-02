# Vulkan Testbed

A pet project for experimenting with various graphics techniques in Vulkan.

![screenshot](https://user-images.githubusercontent.com/3328360/68069386-f83d5400-fd5f-11e9-8c60-6c747b33b26b.png)

## Features
* PBR lighting with glTF 2.0 models
* Ray-traced soft shadows (requires NV ray tracing support)
* Atmospheric scattering
* SSAO (HBAO)
* TAA

## Build Instructions for Visual Studio
1. Install [Vulkan SDK](https://www.lunarg.com/vulkan-sdk/) and [CMake](https://cmake.org/).
2. Generate project files using one of the pre-existing bat files in the top directory or using CMake manually.
3. Open *VulkanTestbed.sln*.
4. Build the solution.
5. Set *VulkanTestbed* as StartUp Project.
6. Start the application.