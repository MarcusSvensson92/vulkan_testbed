#include "VkTexture.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYEXR_IMPLEMENTATION
#include <tinyexr.h>

static VkImageAspectFlags ToVkImageAspectMask(VkFormat format)
{
    switch (format)
    {
        case VK_FORMAT_D16_UNORM:
        case VK_FORMAT_D32_SFLOAT:
            return VK_IMAGE_ASPECT_DEPTH_BIT;
        case VK_FORMAT_D16_UNORM_S8_UINT:
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
            return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    return VK_IMAGE_ASPECT_COLOR_BIT;
}
static VkAccessFlags ToVkAccessMask(VkImageLayout layout)
{
    switch (layout)
    {
        case VK_IMAGE_LAYOUT_GENERAL:                           return VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:          return VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:  return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:   return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:          return VK_ACCESS_SHADER_READ_BIT;
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:              return VK_ACCESS_TRANSFER_READ_BIT;
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:              return VK_ACCESS_TRANSFER_WRITE_BIT;
        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:                   return VK_ACCESS_MEMORY_READ_BIT;
    }
    return 0;
}
static VkPipelineStageFlags ToVkPipelineStageMask(VkImageLayout layout)
{
    switch (layout)
    {
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:          return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:  return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:   return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:              return VK_PIPELINE_STAGE_TRANSFER_BIT;
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:              return VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
}

VkTexture VkTextureCreate(const VkTextureCreateParams& params)
{
    VkImageCreateInfo image_info = {};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = params.Type;
    image_info.extent.width = params.Width;
    image_info.extent.height = params.Height;
    image_info.extent.depth = params.Depth;
    image_info.mipLevels = params.GenerateMipmaps ? static_cast<uint32_t>(log(static_cast<double>(VkMax(params.Width, params.Height))) / log(2)) + 1 : 1;
    image_info.arrayLayers = 1;
    image_info.format = params.Format;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.usage = params.Usage;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo image_allocation_info = {};
    image_allocation_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    VkImage image = VK_NULL_HANDLE;
    VmaAllocation image_allocation = VK_NULL_HANDLE;
    VK(vmaCreateImage(Vk.Allocator, &image_info, &image_allocation_info, &image, &image_allocation, NULL));

    VkImageAspectFlags aspect_mask = ToVkImageAspectMask(params.Format);
    VkImageAspectFlags access_mask = ToVkAccessMask(params.InitialLayout);
    VkImageAspectFlags stage_mask = ToVkPipelineStageMask(params.InitialLayout);

    VkImageViewCreateInfo image_view_info = {};
    image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_info.image = image;
    image_view_info.viewType = params.ViewType;
    image_view_info.format = params.Format;
    image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.subresourceRange.aspectMask = aspect_mask;
    image_view_info.subresourceRange.baseMipLevel = 0;
    image_view_info.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    image_view_info.subresourceRange.baseArrayLayer = 0;
    image_view_info.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

    VkImageView image_view = VK_NULL_HANDLE;
    VK(vkCreateImageView(Vk.Device, &image_view_info, NULL, &image_view));

    if (params.Data != NULL)
    {
        VkAllocation allocation = VkAllocateUploadBuffer(params.DataSize);
        memcpy(allocation.Data, params.Data, params.DataSize);

        if (params.GenerateMipmaps)
        {
            VkRecordCommands(
                [=](VkCommandBuffer cmd)
                {
                    VkImageMemoryBarrier barrier = {};
                    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    barrier.image = image;
                    barrier.subresourceRange.aspectMask = aspect_mask;
                    barrier.subresourceRange.baseMipLevel = 0;
                    barrier.subresourceRange.levelCount = image_info.mipLevels;
                    barrier.subresourceRange.baseArrayLayer = 0;
                    barrier.subresourceRange.layerCount = 1;
                    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                    barrier.srcAccessMask = 0;
                    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);

                    VkBufferImageCopy copy_region = {};
                    copy_region.bufferOffset = allocation.Offset;
                    copy_region.imageSubresource.aspectMask = aspect_mask;
                    copy_region.imageSubresource.mipLevel = 0;
                    copy_region.imageSubresource.baseArrayLayer = 0;
                    copy_region.imageSubresource.layerCount = 1;
                    copy_region.imageExtent.width = image_info.extent.width;
                    copy_region.imageExtent.height = image_info.extent.height;
                    copy_region.imageExtent.depth = 1;
                    vkCmdCopyBufferToImage(cmd, allocation.Buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

                    uint32_t mip_width = image_info.extent.width;
                    uint32_t mip_height = image_info.extent.height;
                    for (uint32_t mip = 1; mip < image_info.mipLevels; ++mip)
                    {
                        barrier.subresourceRange.baseMipLevel = mip - 1;
                        barrier.subresourceRange.levelCount = 1;
                        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);

                        VkImageBlit region = {};
                        region.srcOffsets[1].x = mip_width;
                        region.srcOffsets[1].y = mip_height;
                        region.dstOffsets[1].x = VkMax(mip_width >> 1U, 1U);
                        region.dstOffsets[1].y = VkMax(mip_height >> 1U, 1U);
                        region.srcOffsets[1].z = region.dstOffsets[1].z = 1;
                        region.srcSubresource.mipLevel = mip - 1;
                        region.dstSubresource.mipLevel = mip;
                        region.srcSubresource.aspectMask = region.dstSubresource.aspectMask = aspect_mask;
                        region.srcSubresource.baseArrayLayer = region.dstSubresource.baseArrayLayer = 0;
                        region.srcSubresource.layerCount = region.dstSubresource.layerCount = 1;
                        vkCmdBlitImage(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region, VK_FILTER_LINEAR);

                        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                        barrier.newLayout = params.InitialLayout;
                        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                        barrier.dstAccessMask = access_mask;
                        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, stage_mask, 0, 0, NULL, 0, NULL, 1, &barrier);

                        mip_width = VkMax(mip_width >> 1U, 1U);
                        mip_height = VkMax(mip_height >> 1U, 1U);
                    }

                    barrier.subresourceRange.baseMipLevel = image_info.mipLevels - 1;
                    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                    barrier.newLayout = params.InitialLayout;
                    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                    barrier.dstAccessMask = access_mask;
                    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, stage_mask, 0, 0, NULL, 0, NULL, 1, &barrier);
                });
        }
        else
        {
            VkRecordCommands(
                [=](VkCommandBuffer cmd)
                {
                    VkImageMemoryBarrier barrier = {};
                    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    barrier.image = image;
                    barrier.subresourceRange.aspectMask = aspect_mask;
                    barrier.subresourceRange.baseMipLevel = 0;
                    barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
                    barrier.subresourceRange.baseArrayLayer = 0;
                    barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
                    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                    barrier.srcAccessMask = 0;
                    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

                    VkBufferImageCopy copy_region = {};
                    copy_region.bufferOffset = allocation.Offset;
                    copy_region.imageSubresource.aspectMask = aspect_mask;
                    copy_region.imageSubresource.mipLevel = 0;
                    copy_region.imageSubresource.baseArrayLayer = 0;
                    copy_region.imageSubresource.layerCount = 1;
                    copy_region.imageExtent.width = image_info.extent.width;
                    copy_region.imageExtent.height = image_info.extent.height;
                    copy_region.imageExtent.depth = 1;
                    vkCmdCopyBufferToImage(cmd, allocation.Buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

                    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                    barrier.newLayout = params.InitialLayout;
                    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                    barrier.dstAccessMask = access_mask;
                    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, stage_mask, 0, 0, nullptr, 0, nullptr, 1, &barrier);
                });
        }
    }
    else
    {
        VkRecordCommands(
            [=](VkCommandBuffer cmd)
            {
                VkImageMemoryBarrier barrier = {};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.image = image;
                barrier.subresourceRange.aspectMask = aspect_mask;
                barrier.subresourceRange.baseMipLevel = 0;
                barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
                barrier.subresourceRange.baseArrayLayer = 0;
                barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
                barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                barrier.newLayout = params.InitialLayout;
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = access_mask;
                vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, stage_mask, 0, 0, NULL, 0, NULL, 1, &barrier);
            });
    }

	VkTexture texture;
    texture.Image = image;
    texture.ImageView = image_view;
    texture.ImageAllocation = image_allocation;
    texture.Width = params.Width;
    texture.Height = params.Height;
    texture.Depth = params.Depth;
	return texture;
}
VkTexture VkTextureLoad(const char* filepath, bool srgb)
{
    int width, height, component_count;
    stbi_uc* data = stbi_load(filepath, &width, &height, &component_count, STBI_rgb_alpha);
	assert(data != nullptr);

    VkDeviceSize data_size = width * height * 4;
    VkAllocation allocation = VkAllocateUploadBuffer(data_size);
    memcpy(allocation.Data, data, data_size);

    VkTextureCreateParams params;
    params.Type = VK_IMAGE_TYPE_2D;
    params.ViewType = VK_IMAGE_VIEW_TYPE_2D;
    params.Width = static_cast<uint32_t>(width);
    params.Height = static_cast<uint32_t>(height);
    params.Format = srgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
    params.Usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    params.InitialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    params.Data = data;
    params.DataSize = data_size;
    params.GenerateMipmaps = true;
	VkTexture texture = VkTextureCreate(params);

    stbi_image_free(data);

    return texture;
}
VkTexture VkTextureLoadEXR(const char* filepath)
{
	float* data;
	int width, height;
	const char* error = nullptr;
	if (LoadEXR(&data, &width, &height, filepath, &error) != TINYEXR_SUCCESS)
	{
		if (error != nullptr)
		{
			VkError(error);
			FreeEXRErrorMessage(error);
		}
		return {};
	}

	VkDeviceSize data_size = width * height * 16;
	VkAllocation allocation = VkAllocateUploadBuffer(data_size);
	memcpy(allocation.Data, data, data_size);

	VkTextureCreateParams params;
	params.Type = VK_IMAGE_TYPE_2D;
	params.ViewType = VK_IMAGE_VIEW_TYPE_2D;
	params.Width = static_cast<uint32_t>(width);
	params.Height = static_cast<uint32_t>(height);
	params.Format = VK_FORMAT_R32G32B32A32_SFLOAT;
	params.Usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
	params.InitialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	params.Data = data;
	params.DataSize = data_size;
	VkTexture texture = VkTextureCreate(params);

	free(data);

	return texture;
}
void VkTextureDestroy(const VkTexture& texture)
{
    vkDestroyImageView(Vk.Device, texture.ImageView, NULL);
    vmaDestroyImage(Vk.Allocator, texture.Image, texture.ImageAllocation);
}
