#pragma once

#include "Vk.h"

struct VkTexture
{
	VkImage			    Image			= VK_NULL_HANDLE;
    VkImageView		    ImageView		= VK_NULL_HANDLE;
    VmaAllocation	    ImageAllocation	= VK_NULL_HANDLE;

    uint32_t            Width			= 0;
    uint32_t            Height			= 0;
    uint32_t            Depth			= 0;
};

struct VkTextureCreateParams
{
    VkImageType         Type			= VK_IMAGE_TYPE_2D;
    VkImageViewType     ViewType		= VK_IMAGE_VIEW_TYPE_2D;
    uint32_t		    Width			= 0;
    uint32_t		    Height			= 1;
    uint32_t            Depth			= 1;
    VkFormat		    Format			= VK_FORMAT_UNDEFINED;
    VkImageUsageFlags	Usage			= VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    VkImageLayout	    InitialLayout	= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    const void*		    Data			= nullptr;
    size_t			    DataSize		= 0;
    bool                GenerateMipmaps	= false;
};
VkTexture				VkTextureCreate(const VkTextureCreateParams& params);
VkTexture				VkTextureLoad(const char* filepath, bool srgb);
void					VkTextureDestroy(const VkTexture& texture);
