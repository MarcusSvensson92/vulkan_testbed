#include "VkUtil.h"

#include <fstream>

VkRenderPass VkUtilCreateRenderPass(const VkUtilCreateRenderPassParams& params)
{
    const uint32_t color_attachment_count = static_cast<uint32_t>(params.ColorAttachmentFormats.size());
    const uint32_t depth_attachment_count = params.DepthAttachmentFormat != VK_FORMAT_UNDEFINED ? 1 : 0;

    std::vector<VkAttachmentDescription> attachments(color_attachment_count + depth_attachment_count);
    for (uint32_t i = 0; i < color_attachment_count; ++i)
    {
        attachments[i] = {};
        attachments[i].format = params.ColorAttachmentFormats[i];
        attachments[i].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[i].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachments[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[i].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachments[i].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    if (depth_attachment_count)
    {
        attachments[color_attachment_count] = {};
        attachments[color_attachment_count].format = params.DepthAttachmentFormat;
        attachments[color_attachment_count].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[color_attachment_count].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachments[color_attachment_count].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[color_attachment_count].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachments[color_attachment_count].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[color_attachment_count].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachments[color_attachment_count].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    std::vector<VkAttachmentReference> color_attachments(color_attachment_count);
    for (uint32_t i = 0; i < color_attachment_count; ++i)
    {
        color_attachments[i].attachment = i;
        color_attachments[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    VkAttachmentReference depth_attachment;
    if (depth_attachment_count)
    {
        depth_attachment.attachment = color_attachment_count;
        depth_attachment.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    VkSubpassDescription subpass_description = {};
    subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_description.colorAttachmentCount = color_attachment_count;
    subpass_description.pColorAttachments = color_attachments.data();
    subpass_description.pDepthStencilAttachment = depth_attachment_count ? &depth_attachment : NULL;

    VkRenderPassCreateInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = static_cast<uint32_t>(attachments.size());
    render_pass_info.pAttachments = attachments.data();
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass_description;

    VkRenderPass render_pass = VK_NULL_HANDLE;
    VK(vkCreateRenderPass(Vk.Device, &render_pass_info, NULL, &render_pass));
    return render_pass;
}

VkFramebuffer VkUtilCreateFramebuffer(const VkUtilCreateFramebufferParams& params)
{
    const uint32_t color_attachment_count = static_cast<uint32_t>(params.ColorAttachments.size());
    const uint32_t depth_attachment_count = params.DepthAttachment != VK_NULL_HANDLE ? 1 : 0;

    std::vector<VkImageView> image_views(color_attachment_count + depth_attachment_count);
    for (uint32_t i = 0; i < color_attachment_count; ++i)
    {
        image_views[i] = params.ColorAttachments[i];
    }
    if (depth_attachment_count)
    {
        image_views[color_attachment_count] = params.DepthAttachment;
    }

    VkFramebufferCreateInfo framebuffer_info = {};
    framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_info.renderPass = params.RenderPass;
    framebuffer_info.attachmentCount = static_cast<uint32_t>(image_views.size());
    framebuffer_info.pAttachments = image_views.data();
    framebuffer_info.width = params.Width;
    framebuffer_info.height = params.Height;
    framebuffer_info.layers = 1;

    VkFramebuffer framebuffer = VK_NULL_HANDLE;
    VK(vkCreateFramebuffer(Vk.Device, &framebuffer_info, NULL, &framebuffer));
    return framebuffer;
}

VkPipelineInputAssemblyStateCreateInfo VkUtilGetDefaultInputAssemblyState()
{
    VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {};
    input_assembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_state.primitiveRestartEnable = VK_FALSE;
    return input_assembly_state;
}
VkPipelineRasterizationStateCreateInfo VkUtilGetDefaultRasterizationState()
{
    VkPipelineRasterizationStateCreateInfo rasterization_state = {};
    rasterization_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization_state.depthClampEnable = VK_FALSE;
    rasterization_state.rasterizerDiscardEnable = VK_FALSE;
    rasterization_state.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization_state.cullMode = VK_CULL_MODE_NONE;
    rasterization_state.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterization_state.depthBiasEnable = VK_FALSE;
    rasterization_state.depthBiasConstantFactor = 0.f;
    rasterization_state.depthBiasClamp = 0.f;
    rasterization_state.depthBiasSlopeFactor = 0.f;
    rasterization_state.lineWidth = 1.f;
    return rasterization_state;
}
VkPipelineDepthStencilStateCreateInfo VkUtilGetDefaultDepthStencilState()
{
    VkPipelineDepthStencilStateCreateInfo depth_stencil_state = {};
    depth_stencil_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_state.depthTestEnable = VK_FALSE;
    depth_stencil_state.depthWriteEnable = VK_FALSE;
    depth_stencil_state.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
    depth_stencil_state.depthBoundsTestEnable = VK_FALSE;
    depth_stencil_state.stencilTestEnable = VK_FALSE;
    depth_stencil_state.front = { VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_COMPARE_OP_ALWAYS, 0, 0, 0 };
    depth_stencil_state.back = { VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_COMPARE_OP_ALWAYS, 0, 0, 0 };
    depth_stencil_state.minDepthBounds = 0.f;
    depth_stencil_state.maxDepthBounds = 0.f;
    return depth_stencil_state;
}
VkPipelineColorBlendAttachmentState VkUtilGetDefaultBlendAttachmentState()
{
    VkPipelineColorBlendAttachmentState blend_attachment_state = {};
    blend_attachment_state.blendEnable = VK_FALSE;
    blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
    blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;
    blend_attachment_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    return blend_attachment_state;
}

VkShaderModule VkUtilLoadShaderModule(const std::string& filepath)
{
	std::ifstream file(filepath, std::ios::ate | std::ios::binary);
	if (!file.is_open())
	{
		VkError("Failed to load shader " + filepath);
		return VK_NULL_HANDLE;
	}

	std::vector<char> code(static_cast<size_t>(file.tellg()));
	file.seekg(0);
	file.read(code.data(), code.size());
	file.close();

	VkShaderModuleCreateInfo shader_module_create_info = {};
	shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shader_module_create_info.codeSize = code.size();
	shader_module_create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shader_module = VK_NULL_HANDLE;
	VK(vkCreateShaderModule(Vk.Device, &shader_module_create_info, NULL, &shader_module));
	return shader_module;
}

VkPipeline VkUtilCreateGraphicsPipeline(const VkUtilCreateGraphicsPipelineParams& params)
{
    VkShaderModule vert_shader = VkUtilLoadShaderModule(params.VertexShaderFilepath);
    VkShaderModule frag_shader = VkUtilLoadShaderModule(params.FragmentShaderFilepath);

    VkPipelineShaderStageCreateInfo vert_shader_stage = {};
    vert_shader_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_shader_stage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_shader_stage.module = vert_shader;
    vert_shader_stage.pName = "main";
    VkPipelineShaderStageCreateInfo frag_shader_stage = {};
    frag_shader_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_shader_stage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_shader_stage.module = frag_shader;
    frag_shader_stage.pName = "main";
    const VkPipelineShaderStageCreateInfo shader_stages[] =
    {
        vert_shader_stage,
        frag_shader_stage,
    };

    VkPipelineVertexInputStateCreateInfo vertex_input_state = {};
    vertex_input_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_state.vertexBindingDescriptionCount = static_cast<uint32_t>(params.VertexBindingDescriptions.size());
    vertex_input_state.pVertexBindingDescriptions = params.VertexBindingDescriptions.data();
    vertex_input_state.vertexAttributeDescriptionCount = static_cast<uint32_t>(params.VertexAttributeDescriptions.size());
    vertex_input_state.pVertexAttributeDescriptions = params.VertexAttributeDescriptions.data();

    VkPipelineColorBlendStateCreateInfo blend_state = {};
    blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend_state.logicOpEnable = VK_FALSE;
    blend_state.attachmentCount = static_cast<uint32_t>(params.BlendAttachmentStates.size());
    blend_state.pAttachments = params.BlendAttachmentStates.data();

    VkPipelineMultisampleStateCreateInfo multisample_state = {};
    multisample_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_state.sampleShadingEnable = VK_FALSE;
    multisample_state.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineViewportStateCreateInfo viewport_state = {};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.pViewports = NULL;
    viewport_state.scissorCount = 1;
    viewport_state.pScissors = NULL;

    const VkDynamicState dynamic_states[] =
    {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamic_state = {};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = static_cast<uint32_t>(sizeof(dynamic_states) / sizeof(VkDynamicState));
    dynamic_state.pDynamicStates = dynamic_states;

    VkGraphicsPipelineCreateInfo pipeline_info = {};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.layout = params.PipelineLayout;
    pipeline_info.renderPass = params.RenderPass;
    pipeline_info.stageCount = static_cast<uint32_t>(sizeof(shader_stages) / sizeof(VkPipelineShaderStageCreateInfo));
    pipeline_info.pStages = shader_stages;
    pipeline_info.pVertexInputState = &vertex_input_state;
    pipeline_info.pInputAssemblyState = &params.InputAssemblyState;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &params.RasterizationState;
    pipeline_info.pMultisampleState = &multisample_state;
    pipeline_info.pDepthStencilState = &params.DepthStencilState;
    pipeline_info.pColorBlendState = &blend_state;
    pipeline_info.pDynamicState = &dynamic_state;
    pipeline_info.subpass = 0;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;

    VkPipeline pipeline = VK_NULL_HANDLE;
    VK(vkCreateGraphicsPipelines(Vk.Device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &pipeline));

    vkDestroyShaderModule(Vk.Device, vert_shader, NULL);
    vkDestroyShaderModule(Vk.Device, frag_shader, NULL);

    return pipeline;
}

VkPipeline VkUtilCreateComputePipeline(const VkUtilCreateComputePipelineParams& params)
{
    VkShaderModule comp_shader = VkUtilLoadShaderModule(params.ComputeShaderFilepath);

    VkPipelineShaderStageCreateInfo comp_shader_stage = {};
    comp_shader_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    comp_shader_stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    comp_shader_stage.module = comp_shader;
    comp_shader_stage.pName = "main";

    VkComputePipelineCreateInfo pipeline_info = {};
    pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipeline_info.layout = params.PipelineLayout;
    pipeline_info.stage = comp_shader_stage;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;

    VkPipeline pipeline = VK_NULL_HANDLE;
    VK(vkCreateComputePipelines(Vk.Device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &pipeline));

    vkDestroyShaderModule(Vk.Device, comp_shader, NULL);

    return pipeline;
}

static inline VkAccessFlags ToVkAccessMask(VkImageLayout layout)
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
void VkUtilImageBarrier(VkCommandBuffer cmd, VkImage image, VkImageLayout old_layout, VkImageLayout new_layout, VkImageAspectFlags aspect_mask, uint32_t base_mip, uint32_t mip_count, uint32_t base_layer, uint32_t layer_count)
{
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = image;
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcAccessMask = ToVkAccessMask(old_layout);
    barrier.dstAccessMask = ToVkAccessMask(new_layout);
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = aspect_mask;
    barrier.subresourceRange.baseMipLevel = base_mip;
    barrier.subresourceRange.levelCount = mip_count;
    barrier.subresourceRange.baseArrayLayer = base_layer;
    barrier.subresourceRange.layerCount = layer_count;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);
}