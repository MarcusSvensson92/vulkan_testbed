#pragma once

#include <volk.h>

#define VMA_STATIC_VULKAN_FUNCTIONS 1
#include <vk_mem_alloc.h>

#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <functional>

inline void VkError(const std::string& message)
{
	throw std::runtime_error(message);
}
#define VK(func) { VkResult result = func; if (result != VK_SUCCESS) VkError(std::string(#func) + std::string(" returned with erroneous result code ") + std::to_string(static_cast<uint32_t>(result))); }

template<typename T>
inline T VkMin(T a, T b)
{
	return a < b ? a : b;
}
template<typename T>
inline T VkMax(T a, T b)
{
	return a > b ? a : b;
}
template<typename T>
inline T VkAlignUp(T value, T alignment)
{
	return (value + alignment - 1) / alignment * alignment;
}

struct _Vk
{
	VkInstance												Instance;

	VkDebugReportCallbackEXT								DebugCallback;

	VkSurfaceKHR											Surface;

	VkPhysicalDevice										PhysicalDevice;
	VkPhysicalDeviceProperties								PhysicalDeviceProperties;

	bool													IsRayTracingSupported;

	VkDevice												Device;

	VkQueue													GraphicsQueue;
	uint32_t												GraphicsQueueIndex;

	VkSwapchainKHR											Swapchain;
	VkExtent2D												SwapchainImageExtent;
	uint32_t												SwapchainImageCount;
	uint32_t												SwapchainImageIndex;
	std::vector<VkImage>									SwapchainImages;
	std::vector<VkImageView>								SwapchainImageViews;
	VkSurfaceFormatKHR										SwapchainSurfaceFormat;

	VmaAllocator											Allocator;

	VkBuffer												UploadBuffer;
	VmaAllocation											UploadBufferAllocation;
	VkDeviceMemory											UploadBufferMemory;
	VkDeviceSize											UploadBufferMemoryOffset;
	uint8_t*												UploadBufferMappedData;
	VkDeviceSize											UploadBufferHead;
	std::vector<VkDeviceSize>								UploadBufferTails;

	uint32_t												FrameIndexCurr;
	uint32_t												FrameIndexNext;

	VkCommandPool											CommandPool;

	std::vector<VkCommandBuffer>							CommandBuffers;
	std::vector<VkFence>									CommandBufferFences;
	std::vector<VkSemaphore>								CommandBufferSemaphores;
	std::vector<VkSemaphore>								PresentSemaphores;

	std::vector<VkDescriptorPool>							DescriptorPools;

	std::vector<std::function<void(VkCommandBuffer)>>		RecordedCommands;

	VkQueryPool												TimestampQueryPool;
	uint32_t												TimestampQueryPoolOffset;
	std::vector<std::pair<std::string, uint32_t>>			TimestampLabelsPushed;
	std::vector<std::pair<std::string, uint32_t>>			TimestampLabelsInFlight;
	std::unordered_map<std::string, float>					TimestampLabelsResult;
};
extern _Vk													Vk;

struct VkInitializeParams
{
	void*													WindowHandle;	// Platform specific
	void*													DisplayHandle;	// Platform specific
	uint32_t												BackBufferWidth;
	uint32_t												BackBufferHeight;
	uint32_t												DesiredBackBufferCount;
	bool													EnableValidationLayer;
};
void														VkInitialize(const VkInitializeParams& params);
void														VkTerminate();

void														VkResize(uint32_t width, uint32_t height);

struct VkAllocation
{
	VkBuffer												Buffer;
	VkDeviceSize											Offset;
	uint8_t*												Data;
};
VkAllocation												VkAllocateUploadBuffer(VkDeviceSize size, VkDeviceSize alignment = 256);

void														VkRecordCommands(const std::function<void(VkCommandBuffer)>& commands);

VkCommandBuffer												VkBeginFrame();
void														VkEndFrame();

struct VkDescriptorSetEntry
{
	uint32_t												Binding;
	VkDescriptorType										Type;
	uint32_t												ArrayIndex;
	uint32_t												ArrayCount;
	union
	{
		VkDescriptorImageInfo								ImageInfo;
		VkDescriptorBufferInfo								BufferInfo;
		VkBufferView										TexelBufferInfo;
		VkWriteDescriptorSetAccelerationStructureKHR		AccelerationStructureInfo;

		const VkDescriptorImageInfo*						ImageInfos;
		const VkDescriptorBufferInfo*						BufferInfos;
		const VkBufferView*									TexelBufferInfos;
		const VkWriteDescriptorSetAccelerationStructureKHR*	AccelerationStructureInfos;
	};

	VkDescriptorSetEntry(uint32_t binding, VkDescriptorType type, uint32_t array_index, VkImageView image_view, VkImageLayout image_layout, VkSampler sampler = NULL);
	VkDescriptorSetEntry(uint32_t binding, VkDescriptorType type, uint32_t array_index, VkBuffer buffer, VkDeviceSize offset = 0ULL, VkDeviceSize size = VK_WHOLE_SIZE);
	VkDescriptorSetEntry(uint32_t binding, VkDescriptorType type, uint32_t array_index, VkAccelerationStructureKHR acceleration_structure);
	
	VkDescriptorSetEntry(uint32_t binding, VkDescriptorType type, uint32_t array_index, uint32_t array_count, const VkDescriptorImageInfo* infos);
	VkDescriptorSetEntry(uint32_t binding, VkDescriptorType type, uint32_t array_index, uint32_t array_count, const VkDescriptorBufferInfo* infos);
};
VkDescriptorSet												VkCreateDescriptorSetForCurrentFrame(VkDescriptorSetLayout layout, std::initializer_list<VkDescriptorSetEntry> entries);
															
void														VkPushLabel(VkCommandBuffer cmd, const std::string& label);
void														VkPopLabel(VkCommandBuffer cmd);
float														VkGetLabel(const std::string& label);
