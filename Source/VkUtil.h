#pragma once

#include "Vk.h"

struct VkUtilCreateRenderPassParams
{
    std::vector<VkFormat>                               ColorAttachmentFormats		= {};
    VkFormat                                            DepthAttachmentFormat		= VK_FORMAT_UNDEFINED;
};
VkRenderPass                                            VkUtilCreateRenderPass(const VkUtilCreateRenderPassParams& params);

struct VkUtilCreateFramebufferParams
{
    VkRenderPass                                        RenderPass					= VK_NULL_HANDLE;
    std::vector<VkImageView>                            ColorAttachments			= {};
    VkImageView                                         DepthAttachment				= VK_NULL_HANDLE;
    uint32_t                                            Width						= 0;
    uint32_t                                            Height						= 0;
};
VkFramebuffer                                           VkUtilCreateFramebuffer(const VkUtilCreateFramebufferParams& params);

VkPipelineInputAssemblyStateCreateInfo                  VkUtilGetDefaultInputAssemblyState();
VkPipelineRasterizationStateCreateInfo                  VkUtilGetDefaultRasterizationState();
VkPipelineDepthStencilStateCreateInfo                   VkUtilGetDefaultDepthStencilState();
VkPipelineColorBlendAttachmentState                     VkUtilGetDefaultBlendAttachmentState();

VkShaderModule                                          VkUtilLoadShaderModule(const std::string& filepath);

struct VkUtilCreateGraphicsPipelineParams
{
    VkPipelineLayout                                    PipelineLayout				= VK_NULL_HANDLE;
    VkRenderPass                                        RenderPass					= VK_NULL_HANDLE;
    std::string                                         VertexShaderFilepath		= {};
    std::string                                         FragmentShaderFilepath		= {};
    std::vector<VkVertexInputBindingDescription>	    VertexBindingDescriptions	= {};
    std::vector<VkVertexInputAttributeDescription>	    VertexAttributeDescriptions	= {};
    VkPipelineInputAssemblyStateCreateInfo              InputAssemblyState			= VkUtilGetDefaultInputAssemblyState();
    VkPipelineRasterizationStateCreateInfo              RasterizationState			= VkUtilGetDefaultRasterizationState();
    VkPipelineDepthStencilStateCreateInfo               DepthStencilState			= VkUtilGetDefaultDepthStencilState();
    std::vector<VkPipelineColorBlendAttachmentState>    BlendAttachmentStates		= {};
};
VkPipeline                                              VkUtilCreateGraphicsPipeline(const VkUtilCreateGraphicsPipelineParams& params);

struct VkUtilCreateComputePipelineParams
{
    VkPipelineLayout                                    PipelineLayout				= VK_NULL_HANDLE;
    std::string                                         ComputeShaderFilepath		= {};
};
VkPipeline                                              VkUtilCreateComputePipeline(const VkUtilCreateComputePipelineParams& params);

void                                                    VkUtilImageBarrier(VkCommandBuffer cmd, VkImage image, VkImageLayout old_layout, VkImageLayout new_layout, VkImageAspectFlags aspect_mask, uint32_t base_mip = 0, uint32_t mip_count = VK_REMAINING_MIP_LEVELS, uint32_t base_layer = 0, uint32_t layer_count = VK_REMAINING_ARRAY_LAYERS);
