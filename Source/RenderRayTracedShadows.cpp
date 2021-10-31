#include "RenderRayTracedShadows.h"
#include "VkUtil.h"

#include <algorithm>

void RenderRayTracedShadows::Create(const RenderContext& rc)
{
    if (!Vk.IsRayTracingSupported)
    {
        return;
    }

	// Ray Trace
	{
		VkDescriptorSetLayoutBinding set_layout_bindings_0[] =
		{
			{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR, NULL },
			{ 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR, NULL },
			{ 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR, NULL },
			{ 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR, NULL },
			{ 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR, NULL },
			{ 5, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR, NULL },
		};
		VkDescriptorSetLayoutCreateInfo set_layout_info_0 = {};
		set_layout_info_0.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		set_layout_info_0.bindingCount = static_cast<uint32_t>(sizeof(set_layout_bindings_0) / sizeof(*set_layout_bindings_0));
		set_layout_info_0.pBindings = set_layout_bindings_0;
		VK(vkCreateDescriptorSetLayout(Vk.Device, &set_layout_info_0, NULL, &m_RayTraceDescriptorSetLayouts[0]));

		VkDescriptorBindingFlagsEXT set_layout_binding_flags_1[] =
		{
			0,
			VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT,
			VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT,
			VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT,
		};
		VkDescriptorSetLayoutBindingFlagsCreateInfoEXT set_layout_binding_flags_info_1 = {};
		set_layout_binding_flags_info_1.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
		set_layout_binding_flags_info_1.bindingCount = static_cast<uint32_t>(sizeof(set_layout_binding_flags_1) / sizeof(*set_layout_binding_flags_1));
		set_layout_binding_flags_info_1.pBindingFlags = set_layout_binding_flags_1;

		VkDescriptorSetLayoutBinding set_layout_bindings_1[] =
		{
			{ 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_ANY_HIT_BIT_KHR, NULL },
			{ 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1024, VK_SHADER_STAGE_ANY_HIT_BIT_KHR, NULL },
			{ 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1024, VK_SHADER_STAGE_ANY_HIT_BIT_KHR, NULL },
			{ 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1024, VK_SHADER_STAGE_ANY_HIT_BIT_KHR, NULL },
		};
		VkDescriptorSetLayoutCreateInfo set_layout_info_1 = {};
		set_layout_info_1.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		set_layout_info_1.pNext = &set_layout_binding_flags_info_1;
		set_layout_info_1.bindingCount = static_cast<uint32_t>(sizeof(set_layout_bindings_1) / sizeof(*set_layout_bindings_1));
		set_layout_info_1.pBindings = set_layout_bindings_1;
		VK(vkCreateDescriptorSetLayout(Vk.Device, &set_layout_info_1, NULL, &m_RayTraceDescriptorSetLayouts[1]));

		VkPipelineLayoutCreateInfo pipeline_layout_info = {};
		pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_info.setLayoutCount = sizeof(m_RayTraceDescriptorSetLayouts) / sizeof(*m_RayTraceDescriptorSetLayouts);
		pipeline_layout_info.pSetLayouts = m_RayTraceDescriptorSetLayouts;
		VK(vkCreatePipelineLayout(Vk.Device, &pipeline_layout_info, NULL, &m_RayTracePipelineLayout));
	}

	// Reproject
	{
		VkDescriptorSetLayoutBinding set_layout_bindings[] =
		{
			{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL },
			{ 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL },
			{ 2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL },
			{ 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL },
			{ 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL },
			{ 5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL },
			{ 6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL },
			{ 7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL },
		};
		VkDescriptorSetLayoutCreateInfo set_layout_info = {};
		set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		set_layout_info.bindingCount = static_cast<uint32_t>(sizeof(set_layout_bindings) / sizeof(*set_layout_bindings));
		set_layout_info.pBindings = set_layout_bindings;
		VK(vkCreateDescriptorSetLayout(Vk.Device, &set_layout_info, NULL, &m_ReprojectDescriptorSetLayout));

		VkPipelineLayoutCreateInfo pipeline_layout_info = {};
		pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_info.setLayoutCount = 1;
		pipeline_layout_info.pSetLayouts = &m_ReprojectDescriptorSetLayout;
		VK(vkCreatePipelineLayout(Vk.Device, &pipeline_layout_info, NULL, &m_ReprojectPipelineLayout));
	}

	// Filter
	{
		VkDescriptorSetLayoutBinding set_layout_bindings[] =
		{
			{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL },
			{ 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL },
			{ 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL },
			{ 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL },
		};
		VkDescriptorSetLayoutCreateInfo set_layout_info = {};
		set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		set_layout_info.bindingCount = static_cast<uint32_t>(sizeof(set_layout_bindings) / sizeof(*set_layout_bindings));
		set_layout_info.pBindings = set_layout_bindings;
		VK(vkCreateDescriptorSetLayout(Vk.Device, &set_layout_info, NULL, &m_FilterDescriptorSetLayout));

		VkPipelineLayoutCreateInfo pipeline_layout_info = {};
		pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_info.setLayoutCount = 1;
		pipeline_layout_info.pSetLayouts = &m_FilterDescriptorSetLayout;
		VK(vkCreatePipelineLayout(Vk.Device, &pipeline_layout_info, NULL, &m_FilterPipelineLayout));
	}

	// Resolve
	{
		VkDescriptorSetLayoutBinding set_layout_bindings[] =
		{
			{ 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL },
			{ 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL },
		};
		VkDescriptorSetLayoutCreateInfo set_layout_info = {};
		set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		set_layout_info.bindingCount = static_cast<uint32_t>(sizeof(set_layout_bindings) / sizeof(*set_layout_bindings));
		set_layout_info.pBindings = set_layout_bindings;
		VK(vkCreateDescriptorSetLayout(Vk.Device, &set_layout_info, NULL, &m_ResolveDescriptorSetLayout));

		VkPipelineLayoutCreateInfo pipeline_layout_info = {};
		pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_info.setLayoutCount = 1;
		pipeline_layout_info.pSetLayouts = &m_ResolveDescriptorSetLayout;
		VK(vkCreatePipelineLayout(Vk.Device, &pipeline_layout_info, NULL, &m_ResolvePipelineLayout));
	}

	VkPhysicalDeviceRayTracingPipelinePropertiesKHR ray_tracing_properties = {};
	ray_tracing_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;

	VkPhysicalDeviceProperties2KHR physical_device_properties = {};
	physical_device_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;
	physical_device_properties.pNext = &ray_tracing_properties;
	vkGetPhysicalDeviceProperties2(Vk.PhysicalDevice, &physical_device_properties);

	m_ShaderGroupHandleSize = ray_tracing_properties.shaderGroupHandleSize;
	m_ShaderGroupHandleAlignedSize = VkAlignUp(ray_tracing_properties.shaderGroupHandleSize, ray_tracing_properties.shaderGroupBaseAlignment);

	VkBufferCreateInfo buffer_create_info = {};
	buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_create_info.size = static_cast<VkDeviceSize>(m_ShaderGroupHandleAlignedSize) * 3;
	buffer_create_info.usage = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo buffer_allocation_create_info = {};
	buffer_allocation_create_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	VK(vmaCreateBuffer(Vk.Allocator, &buffer_create_info, &buffer_allocation_create_info, &m_ShaderBindingTableBuffer, &m_ShaderBindingTableBufferAllocation, NULL));
	m_ShaderBindingTableBufferDeviceAddress = VkUtilGetDeviceAddress(m_ShaderBindingTableBuffer);

	CreatePipelines(rc);
	CreateResolutionDependentResources(rc);
}

void RenderRayTracedShadows::Destroy()
{
    if (!Vk.IsRayTracingSupported)
    {
        return;
    }

	vmaDestroyBuffer(Vk.Allocator, m_ShaderBindingTableBuffer, m_ShaderBindingTableBufferAllocation);

	DestroyPipelines();
	DestroyResolutionDependentResources();

	vkDestroyPipelineLayout(Vk.Device, m_RayTracePipelineLayout, NULL);
	vkDestroyDescriptorSetLayout(Vk.Device, m_RayTraceDescriptorSetLayouts[0], NULL);
	vkDestroyDescriptorSetLayout(Vk.Device, m_RayTraceDescriptorSetLayouts[1], NULL);

	vkDestroyPipelineLayout(Vk.Device, m_ReprojectPipelineLayout, NULL);
	vkDestroyDescriptorSetLayout(Vk.Device, m_ReprojectDescriptorSetLayout, NULL);

	vkDestroyPipelineLayout(Vk.Device, m_FilterPipelineLayout, NULL);
	vkDestroyDescriptorSetLayout(Vk.Device, m_FilterDescriptorSetLayout, NULL);

	vkDestroyPipelineLayout(Vk.Device, m_ResolvePipelineLayout, NULL);
	vkDestroyDescriptorSetLayout(Vk.Device, m_ResolveDescriptorSetLayout, NULL);
}

void RenderRayTracedShadows::CreatePipelines(const RenderContext& rc)
{
	// Ray Trace
	{
		VkShaderModule rgen_shader = VkUtilLoadShaderModule("../Assets/Shaders/Shadows.rgen");
		VkShaderModule rmiss_shader = VkUtilLoadShaderModule("../Assets/Shaders/Shadows.rmiss");
		VkShaderModule rahit_shader = VkUtilLoadShaderModule("../Assets/Shaders/Shadows.rahit");

		VkPipelineShaderStageCreateInfo rgen_shader_stage = {};
		rgen_shader_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		rgen_shader_stage.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
		rgen_shader_stage.module = rgen_shader;
		rgen_shader_stage.pName = "main";
		VkPipelineShaderStageCreateInfo rmiss_shader_stage = {};
		rmiss_shader_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		rmiss_shader_stage.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
		rmiss_shader_stage.module = rmiss_shader;
		rmiss_shader_stage.pName = "main";
		VkPipelineShaderStageCreateInfo rahit_shader_stage = {};
		rahit_shader_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		rahit_shader_stage.stage = VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
		rahit_shader_stage.module = rahit_shader;
		rahit_shader_stage.pName = "main";
		const VkPipelineShaderStageCreateInfo shader_stages[] =
		{
			rgen_shader_stage,
			rmiss_shader_stage,
			rahit_shader_stage,
		};

		VkRayTracingShaderGroupCreateInfoKHR rgen_shader_group = {};
		rgen_shader_group.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		rgen_shader_group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		rgen_shader_group.generalShader = 0;
		rgen_shader_group.closestHitShader = VK_SHADER_UNUSED_KHR;
		rgen_shader_group.anyHitShader = VK_SHADER_UNUSED_KHR;
		rgen_shader_group.intersectionShader = VK_SHADER_UNUSED_KHR;
		VkRayTracingShaderGroupCreateInfoKHR rmiss_shader_group = {};
		rmiss_shader_group.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		rmiss_shader_group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		rmiss_shader_group.generalShader = 1;
		rmiss_shader_group.closestHitShader = VK_SHADER_UNUSED_KHR;
		rmiss_shader_group.anyHitShader = VK_SHADER_UNUSED_KHR;
		rmiss_shader_group.intersectionShader = VK_SHADER_UNUSED_KHR;
		VkRayTracingShaderGroupCreateInfoKHR rahit_shader_group = {};
		rahit_shader_group.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		rahit_shader_group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
		rahit_shader_group.generalShader = VK_SHADER_UNUSED_KHR;
		rahit_shader_group.closestHitShader = VK_SHADER_UNUSED_KHR;
		rahit_shader_group.anyHitShader = 2;
		rahit_shader_group.intersectionShader = VK_SHADER_UNUSED_KHR;
		const VkRayTracingShaderGroupCreateInfoKHR shader_groups[] =
		{
			rgen_shader_group,
			rmiss_shader_group,
			rahit_shader_group,
		};

		VkRayTracingPipelineCreateInfoKHR pipeline_info = {};
		pipeline_info.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
		pipeline_info.layout = m_RayTracePipelineLayout;
		pipeline_info.stageCount = static_cast<uint32_t>(sizeof(shader_stages) / sizeof(VkPipelineShaderStageCreateInfo));
		pipeline_info.pStages = shader_stages;
		pipeline_info.groupCount = static_cast<uint32_t>(sizeof(shader_groups) / sizeof(VkRayTracingShaderGroupCreateInfoKHR));
		pipeline_info.pGroups = shader_groups;
		VK(vkCreateRayTracingPipelinesKHR(Vk.Device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &m_RayTracePipeline));

		vkDestroyShaderModule(Vk.Device, rgen_shader, NULL);
		vkDestroyShaderModule(Vk.Device, rmiss_shader, NULL);
		vkDestroyShaderModule(Vk.Device, rahit_shader, NULL);

		VkDeviceSize buffer_size = static_cast<VkDeviceSize>(m_ShaderGroupHandleAlignedSize) * 3;
		VkAllocation buffer_allocation = VkAllocateUploadBuffer(buffer_size);

		vkGetRayTracingShaderGroupHandlesKHR(Vk.Device, m_RayTracePipeline, 0, 1, m_ShaderGroupHandleSize, buffer_allocation.Data + 0 * m_ShaderGroupHandleAlignedSize);
		vkGetRayTracingShaderGroupHandlesKHR(Vk.Device, m_RayTracePipeline, 1, 1, m_ShaderGroupHandleSize, buffer_allocation.Data + 1 * m_ShaderGroupHandleAlignedSize);
		vkGetRayTracingShaderGroupHandlesKHR(Vk.Device, m_RayTracePipeline, 2, 1, m_ShaderGroupHandleSize, buffer_allocation.Data + 2 * m_ShaderGroupHandleAlignedSize);

		VkRecordCommands(
			[=](VkCommandBuffer cmd)
			{
				VkBufferMemoryBarrier pre_transfer_barrier = {};
				pre_transfer_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
				pre_transfer_barrier.srcAccessMask = 0;
				pre_transfer_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				pre_transfer_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				pre_transfer_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				pre_transfer_barrier.buffer = m_ShaderBindingTableBuffer;
				pre_transfer_barrier.offset = 0;
				pre_transfer_barrier.size = buffer_size;
				vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 1, &pre_transfer_barrier, 0, NULL);

				VkBufferCopy buffer_copy_region;
				buffer_copy_region.srcOffset = buffer_allocation.Offset;
				buffer_copy_region.dstOffset = 0;
				buffer_copy_region.size = buffer_size;
				vkCmdCopyBuffer(cmd, buffer_allocation.Buffer, m_ShaderBindingTableBuffer, 1, &buffer_copy_region);

				VkBufferMemoryBarrier post_transfer_barrier = {};
				post_transfer_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
				post_transfer_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				post_transfer_barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
				post_transfer_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				post_transfer_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				post_transfer_barrier.buffer = m_ShaderBindingTableBuffer;
				post_transfer_barrier.offset = 0;
				post_transfer_barrier.size = buffer_size;
				vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, 0, 0, NULL, 1, &post_transfer_barrier, 0, NULL);
			});
	}

	// Reproject
	{
		VkUtilCreateComputePipelineParams pipeline_params;
		pipeline_params.PipelineLayout = m_ReprojectPipelineLayout;
		pipeline_params.ComputeShaderFilepath = "../Assets/Shaders/ShadowsReproject.comp";
		m_ReprojectPipeline = VkUtilCreateComputePipeline(pipeline_params);
	}

	// Filter
	{
		VkUtilCreateComputePipelineParams pipeline_params;
		pipeline_params.PipelineLayout = m_FilterPipelineLayout;
		pipeline_params.ComputeShaderFilepath = "../Assets/Shaders/ShadowsFilter.comp";
		m_FilterPipeline = VkUtilCreateComputePipeline(pipeline_params);
	}

	// Resolve
	{
		VkUtilCreateComputePipelineParams pipeline_params;
		pipeline_params.PipelineLayout = m_ResolvePipelineLayout;
		pipeline_params.ComputeShaderFilepath = "../Assets/Shaders/ShadowsResolve.comp";
		m_ResolvePipeline = VkUtilCreateComputePipeline(pipeline_params);
	}
}

void RenderRayTracedShadows::DestroyPipelines()
{
	vkDestroyPipeline(Vk.Device, m_RayTracePipeline, NULL);
	vkDestroyPipeline(Vk.Device, m_ReprojectPipeline, NULL);
	vkDestroyPipeline(Vk.Device, m_FilterPipeline, NULL);
	vkDestroyPipeline(Vk.Device, m_ResolvePipeline, NULL);
}

void RenderRayTracedShadows::CreateResolutionDependentResources(const RenderContext& rc)
{
	VkTextureCreateParams temporal_texture_params;
	temporal_texture_params.Type = VK_IMAGE_TYPE_2D;
	temporal_texture_params.ViewType = VK_IMAGE_VIEW_TYPE_2D;
	temporal_texture_params.Width = rc.Width;
	temporal_texture_params.Height = rc.Height;
	temporal_texture_params.Format = VK_FORMAT_R16G16B16A16_UNORM;
	temporal_texture_params.Usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	temporal_texture_params.InitialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	m_TemporalTextures[0] = VkTextureCreate(temporal_texture_params);
	m_TemporalTextures[1] = VkTextureCreate(temporal_texture_params);

	VkTextureCreateParams variance_texture_params;
	variance_texture_params.Type = VK_IMAGE_TYPE_2D;
	variance_texture_params.ViewType = VK_IMAGE_VIEW_TYPE_2D;
	variance_texture_params.Width = rc.Width;
	variance_texture_params.Height = rc.Height;
	variance_texture_params.Format = VK_FORMAT_R16G16_UNORM;
	variance_texture_params.Usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	variance_texture_params.InitialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	m_VarianceTextures[0] = VkTextureCreate(variance_texture_params);
	m_VarianceTextures[1] = VkTextureCreate(variance_texture_params);
}

void RenderRayTracedShadows::DestroyResolutionDependentResources()
{
	VkTextureDestroy(m_TemporalTextures[0]);
	VkTextureDestroy(m_TemporalTextures[1]);
	VkTextureDestroy(m_VarianceTextures[0]);
	VkTextureDestroy(m_VarianceTextures[1]);
}

void RenderRayTracedShadows::RecreatePipelines(const RenderContext& rc)
{
	if (!Vk.IsRayTracingSupported)
	{
		return;
	}

	DestroyPipelines();
	CreatePipelines(rc);
}

void RenderRayTracedShadows::RecreateResolutionDependentResources(const RenderContext& rc)
{
	if (!Vk.IsRayTracingSupported)
	{
		return;
	}

	DestroyResolutionDependentResources();
	CreateResolutionDependentResources(rc);
}

void RenderRayTracedShadows::RayTrace(const RenderContext& rc, VkCommandBuffer cmd, const AccelerationStructure& as)
{
	if (!Vk.IsRayTracingSupported || !rc.EnableRayTracedShadows)
	{
		return;
	}

	VkUtilImageBarrier(cmd, rc.DepthTexture.Image, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);
	VkUtilImageBarrier(cmd, rc.ShadowTexture.Image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);

	{
		VkPushLabel(cmd, "Shadows Trace Rays");

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_RayTracePipeline);

		glm::mat4 view = rc.CameraCurr.m_View;
		view[3][0] = view[3][1] = view[3][2] = 0.0f; // Set translation to zero

		struct Constants
		{
			glm::mat4	InvViewProjZeroTranslation;
			glm::vec3	ViewPosition;
			float		CosAngle;
			glm::vec3	LightDirection;
			uint32_t	AlphaTest;
		};
		VkAllocation constants_allocation = VkAllocateUploadBuffer(sizeof(Constants));
		Constants* constants = reinterpret_cast<Constants*>(constants_allocation.Data);
		constants->InvViewProjZeroTranslation = glm::inverse(rc.CameraCurr.m_Projection * view);
		constants->ViewPosition = rc.CameraCurr.m_Position;
		constants->CosAngle = std::cos(glm::radians(m_ConeAngle) * 0.5f);
		constants->LightDirection = glm::normalize(rc.SunDirection);
		constants->AlphaTest = m_AlphaTest;

		VkDescriptorSet sets[] =
		{
			VkCreateDescriptorSetForCurrentFrame(m_RayTraceDescriptorSetLayouts[0],
			{
				{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, constants_allocation.Buffer, constants_allocation.Offset, sizeof(Constants) },
				{ 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0, rc.ShadowTexture.ImageView, VK_IMAGE_LAYOUT_GENERAL, VK_NULL_HANDLE },
				{ 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, rc.DepthTexture.ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rc.NearestClamp },
				{ 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, rc.NormalTexture.ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rc.NearestClamp },
				{ 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, rc.BlueNoiseTextures[m_Reproject ? (rc.FrameCounter % static_cast<uint32_t>(rc.BlueNoiseTextures.size())) : 0].ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rc.NearestWrap },
				{ 5, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 0, as.m_TopLevel.AccelerationStructure },
			}),
			VkCreateDescriptorSetForCurrentFrame(m_RayTraceDescriptorSetLayouts[1],
			{
				{ 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, as.m_TransparentInstanceBuffer },
				{ 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, static_cast<uint32_t>(as.m_IndexBufferInfo.size()), as.m_IndexBufferInfo.data() },
				{ 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, static_cast<uint32_t>(as.m_VertexBufferInfo.size()), as.m_VertexBufferInfo.data() },
				{ 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, static_cast<uint32_t>(as.m_BaseColorImageInfo.size()), as.m_BaseColorImageInfo.data() },
			})
		};
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_RayTracePipelineLayout, 0, sizeof(sets) / sizeof(*sets), sets, 0, NULL);

		VkStridedDeviceAddressRegionKHR raygen_shader_binding_table;
		raygen_shader_binding_table.deviceAddress = m_ShaderBindingTableBufferDeviceAddress;
		raygen_shader_binding_table.stride = m_ShaderGroupHandleSize;
		raygen_shader_binding_table.size = m_ShaderGroupHandleSize;
		VkStridedDeviceAddressRegionKHR miss_shader_binding_table;
		miss_shader_binding_table.deviceAddress = m_ShaderBindingTableBufferDeviceAddress + m_ShaderGroupHandleAlignedSize;
		miss_shader_binding_table.stride = m_ShaderGroupHandleSize;
		miss_shader_binding_table.size = m_ShaderGroupHandleSize;
		VkStridedDeviceAddressRegionKHR hit_shader_binding_table;
		hit_shader_binding_table.deviceAddress = m_ShaderBindingTableBufferDeviceAddress + m_ShaderGroupHandleAlignedSize * 2;
		hit_shader_binding_table.stride = m_ShaderGroupHandleSize;
		hit_shader_binding_table.size = m_ShaderGroupHandleSize;
		VkStridedDeviceAddressRegionKHR callable_shader_binding_table;
		callable_shader_binding_table.deviceAddress = 0;
		callable_shader_binding_table.stride = 0;
		callable_shader_binding_table.size = 0;
		vkCmdTraceRaysKHR(cmd, &raygen_shader_binding_table, &miss_shader_binding_table, &hit_shader_binding_table, &callable_shader_binding_table, rc.Width, rc.Height, 1);

		VkPopLabel(cmd);
	}

	VkUtilImageBarrier(cmd, rc.DepthTexture.Image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);
	VkUtilImageBarrier(cmd, rc.ShadowTexture.Image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
	
	if (m_Reproject)
	{
		VkUtilImageBarrier(cmd, m_TemporalTextures[rc.FrameCounter & 1].Image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);
		VkUtilImageBarrier(cmd, m_VarianceTextures[0].Image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);

		{
			VkPushLabel(cmd, "Shadows Reproject");

			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_ReprojectPipeline);

			static uint32_t last_frame= ~0U;
			const bool is_hist_valid = last_frame == rc.FrameCounter;
			last_frame = rc.FrameCounter + 1;

			struct Constants
			{
				float		AlphaShadow;
				float		AlphaMoments;
				uint32_t	IsHistValid;
			};
			VkAllocation constants_allocation = VkAllocateUploadBuffer(sizeof(Constants));
			Constants* constants = reinterpret_cast<Constants*>(constants_allocation.Data);
			constants->AlphaShadow = m_ReprojectAlphaShadow;
			constants->AlphaMoments = m_ReprojectAlphaMoments;
			constants->IsHistValid = is_hist_valid;

			VkDescriptorSet set = VkCreateDescriptorSetForCurrentFrame(m_ReprojectDescriptorSetLayout,
				{
					{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, constants_allocation.Buffer, constants_allocation.Offset, sizeof(Constants) },
					{ 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0, m_VarianceTextures[0].ImageView, VK_IMAGE_LAYOUT_GENERAL, VK_NULL_HANDLE },
					{ 2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0, m_TemporalTextures[rc.FrameCounter & 1].ImageView, VK_IMAGE_LAYOUT_GENERAL, VK_NULL_HANDLE },
					{ 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, m_TemporalTextures[(rc.FrameCounter + 1) & 1].ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rc.NearestClamp },
					{ 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, rc.ShadowTexture.ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rc.NearestClamp },
					{ 5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, rc.MotionTexture.ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rc.NearestClamp },
					{ 6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, rc.LinearDepthTextures[rc.FrameCounter & 1].ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rc.NearestClamp },
					{ 7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, rc.LinearDepthTextures[(rc.FrameCounter + 1) & 1].ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rc.NearestClamp },
				});
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_ReprojectPipelineLayout, 0, 1, &set, 0, NULL);

			vkCmdDispatch(cmd, (rc.Width + 7) / 8, (rc.Height + 7) / 8, 1);

			VkPopLabel(cmd);
		}

		VkUtilImageBarrier(cmd, m_TemporalTextures[rc.FrameCounter & 1].Image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
		VkUtilImageBarrier(cmd, m_VarianceTextures[0].Image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);

		if (m_Filter)
		{
			VkPushLabel(cmd, "Shadows Filter");

			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_FilterPipeline);

			for (int32_t i = 0; i < m_FilterIterations; ++i)
			{
				uint32_t src_index = i & 1;
				uint32_t dst_index = 1 - src_index;

				VkUtilImageBarrier(cmd, m_VarianceTextures[dst_index].Image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);

				struct Constants
				{
					int32_t	StepSize;
					float	PhiVariance;
				};
				VkAllocation constants_allocation = VkAllocateUploadBuffer(sizeof(Constants));
				Constants* constants = reinterpret_cast<Constants*>(constants_allocation.Data);
				constants->StepSize = 1 << (m_FilterIterations - i - 1);
				constants->PhiVariance = m_FilterPhiVariance;

				VkDescriptorSet set = VkCreateDescriptorSetForCurrentFrame(m_FilterDescriptorSetLayout,
					{
						{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, constants_allocation.Buffer, constants_allocation.Offset, sizeof(Constants) },
						{ 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0, m_VarianceTextures[dst_index].ImageView, VK_IMAGE_LAYOUT_GENERAL, VK_NULL_HANDLE },
						{ 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, m_VarianceTextures[src_index].ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rc.NearestClamp },
						{ 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, rc.LinearDepthTextures[rc.FrameCounter & 1].ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rc.NearestClamp },
					});
				vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_FilterPipelineLayout, 0, 1, &set, 0, NULL);

				vkCmdDispatch(cmd, (rc.Width + 7) / 8, (rc.Height + 7) / 8, 1);

				VkUtilImageBarrier(cmd, m_VarianceTextures[dst_index].Image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
			}

			VkPopLabel(cmd);
		}

		VkUtilImageBarrier(cmd, rc.ShadowTexture.Image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);

		{
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_ResolvePipeline);

			uint32_t variance_texture_index = m_Filter ? (m_FilterIterations & 1) : 0;
			VkImageView variance_image_view = m_VarianceTextures[variance_texture_index].ImageView;

			VkDescriptorSet set = VkCreateDescriptorSetForCurrentFrame(m_ResolveDescriptorSetLayout,
				{
					{ 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0, rc.ShadowTexture.ImageView, VK_IMAGE_LAYOUT_GENERAL, VK_NULL_HANDLE },
					{ 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, variance_image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rc.NearestClamp },
				});
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_ResolvePipelineLayout, 0, 1, &set, 0, NULL);

			vkCmdDispatch(cmd, (rc.Width + 7) / 8, (rc.Height + 7) / 8, 1);
		}

		VkUtilImageBarrier(cmd, rc.ShadowTexture.Image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
	}
}