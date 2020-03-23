# Vulkan Testbed

A pet project for experimenting with various graphics techniques in Vulkan.

![screenshot](https://user-images.githubusercontent.com/3328360/77348147-3e0fae80-6d39-11ea-8d42-04c43f7d7cc6.png)

## Features
* PBR lighting with glTF 2.0 models
* Ray-traced soft shadows (requires VK_KHR_ray_tracing support)
* Atmospheric scattering
* SSAO (HBAO)
* TAA

## Build Instructions

### Visual Studio
1. Install [Vulkan SDK](https://www.lunarg.com/vulkan-sdk/) and [CMake](https://cmake.org/).
2. Generate project files using one of the pre-existing bat files in the top directory or using CMake manually.
3. Open *VulkanTestbed.sln*.
4. Build the solution.
5. Set *VulkanTestbed* as StartUp Project.

### Unix Makefiles
1. Install [Vulkan SDK](https://www.lunarg.com/vulkan-sdk/) and [CMake](https://cmake.org/).
2. Create a new directory for the build files.
```
mkdir Build
cd Build
```
3. Generate a makefile using CMake.
```
cmake -G "Unix Makefiles" ..
```
4. Build the makefile.
```
make
```