#include "RenderPostProcess.h"
#include "VkUtil.h"

void RenderPostProcess::Create(const RenderContext& rc)
{
    // Temporal Blend
    {
        VkDescriptorSetLayoutBinding set_layout_bindings[] =
        {
            { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL },
            { 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL },
            { 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL },
            { 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL },
            { 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL },
			{ 5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL },
        };
        VkDescriptorSetLayoutCreateInfo set_layout_info = {};
        set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        set_layout_info.bindingCount = static_cast<uint32_t>(sizeof(set_layout_bindings) / sizeof(*set_layout_bindings));
        set_layout_info.pBindings = set_layout_bindings;
        VK(vkCreateDescriptorSetLayout(Vk.Device, &set_layout_info, NULL, &m_TemporalBlendDescriptorSetLayout));

        VkPipelineLayoutCreateInfo pipeline_layout_info = {};
        pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_info.setLayoutCount = 1;
        pipeline_layout_info.pSetLayouts = &m_TemporalBlendDescriptorSetLayout;
        VK(vkCreatePipelineLayout(Vk.Device, &pipeline_layout_info, NULL, &m_TemporalBlendPipelineLayout));
    }

    // Temporal Resolve
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
        VK(vkCreateDescriptorSetLayout(Vk.Device, &set_layout_info, NULL, &m_TemporalResolveDescriptorSetLayout));

        VkPipelineLayoutCreateInfo pipeline_layout_info = {};
        pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_info.setLayoutCount = 1;
        pipeline_layout_info.pSetLayouts = &m_TemporalResolveDescriptorSetLayout;
        VK(vkCreatePipelineLayout(Vk.Device, &pipeline_layout_info, NULL, &m_TemporalResolvePipelineLayout));
    }

    // Tone Mapping
    {
        VkDescriptorSetLayoutBinding set_layout_bindings[] =
        {
            { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, NULL },
            { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, NULL },
        };
        VkDescriptorSetLayoutCreateInfo set_layout_info = {};
        set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        set_layout_info.bindingCount = static_cast<uint32_t>(sizeof(set_layout_bindings) / sizeof(*set_layout_bindings));
        set_layout_info.pBindings = set_layout_bindings;
        VK(vkCreateDescriptorSetLayout(Vk.Device, &set_layout_info, NULL, &m_ToneMappingDescriptorSetLayout));

        VkPipelineLayoutCreateInfo pipeline_layout_info = {};
        pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_info.setLayoutCount = 1;
        pipeline_layout_info.pSetLayouts = &m_ToneMappingDescriptorSetLayout;
        VK(vkCreatePipelineLayout(Vk.Device, &pipeline_layout_info, NULL, &m_ToneMappingPipelineLayout));
    }

	// Debug
	{
		VkDescriptorSetLayoutBinding set_layout_bindings[] =
		{
			{ 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, NULL },
		};
		VkDescriptorSetLayoutCreateInfo set_layout_info = {};
		set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		set_layout_info.bindingCount = static_cast<uint32_t>(sizeof(set_layout_bindings) / sizeof(*set_layout_bindings));
		set_layout_info.pBindings = set_layout_bindings;
		VK(vkCreateDescriptorSetLayout(Vk.Device, &set_layout_info, NULL, &m_DebugDescriptorSetLayout));

		VkPipelineLayoutCreateInfo pipeline_layout_info = {};
		pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_info.setLayoutCount = 1;
		pipeline_layout_info.pSetLayouts = &m_DebugDescriptorSetLayout;
		VK(vkCreatePipelineLayout(Vk.Device, &pipeline_layout_info, NULL, &m_DebugPipelineLayout));
	}

	CreatePipelines(rc);
	CreateResolutionDependentResources(rc);
}

void RenderPostProcess::Destroy()
{
	DestroyResolutionDependentResources();
	DestroyPipelines();

    vkDestroyPipelineLayout(Vk.Device, m_TemporalBlendPipelineLayout, NULL);
    vkDestroyDescriptorSetLayout(Vk.Device, m_TemporalBlendDescriptorSetLayout, NULL);

    vkDestroyPipelineLayout(Vk.Device, m_TemporalResolvePipelineLayout, NULL);
    vkDestroyDescriptorSetLayout(Vk.Device, m_TemporalResolveDescriptorSetLayout, NULL);

    vkDestroyPipelineLayout(Vk.Device, m_ToneMappingPipelineLayout, NULL);
    vkDestroyDescriptorSetLayout(Vk.Device, m_ToneMappingDescriptorSetLayout, NULL);

	vkDestroyPipelineLayout(Vk.Device, m_DebugPipelineLayout, NULL);
	vkDestroyDescriptorSetLayout(Vk.Device, m_DebugDescriptorSetLayout, NULL);
}

void RenderPostProcess::CreatePipelines(const RenderContext& rc)
{
	// Temporal Blend
	{
		VkUtilCreateComputePipelineParams pipeline_params;
		pipeline_params.PipelineLayout = m_TemporalBlendPipelineLayout;
		pipeline_params.ComputeShaderFilepath = "../Assets/Shaders/PostProcessTemporalBlend.comp";
		m_TemporalBlendPipeline = VkUtilCreateComputePipeline(pipeline_params);
	}

	// Temporal Resolve
	{
		VkUtilCreateComputePipelineParams pipeline_params;
		pipeline_params.PipelineLayout = m_TemporalResolvePipelineLayout;
		pipeline_params.ComputeShaderFilepath = "../Assets/Shaders/PostProcessTemporalResolve.comp";
		m_TemporalResolvePipeline = VkUtilCreateComputePipeline(pipeline_params);
	}

	// Tone Mapping
	{
		VkUtilCreateGraphicsPipelineParams pipeline_params;
		pipeline_params.PipelineLayout = m_ToneMappingPipelineLayout;
		pipeline_params.RenderPass = rc.BackBufferRenderPass;
		pipeline_params.VertexShaderFilepath = "../Assets/Shaders/PostProcessToneMapping.vert";
		pipeline_params.FragmentShaderFilepath = "../Assets/Shaders/PostProcessToneMapping.frag";
		pipeline_params.BlendAttachmentStates = { VkUtilGetDefaultBlendAttachmentState() };
		m_ToneMappingPipeline = VkUtilCreateGraphicsPipeline(pipeline_params);
	}

	// Debug
	{
		VkUtilCreateGraphicsPipelineParams pipeline_params;
		pipeline_params.PipelineLayout = m_DebugPipelineLayout;
		pipeline_params.RenderPass = rc.BackBufferRenderPass;
		pipeline_params.VertexShaderFilepath = "../Assets/Shaders/PostProcessDebug.vert";
		pipeline_params.FragmentShaderFilepath = "../Assets/Shaders/PostProcessDebug.frag";
		pipeline_params.BlendAttachmentStates = { VkUtilGetDefaultBlendAttachmentState() };
		m_DebugPipeline = VkUtilCreateGraphicsPipeline(pipeline_params);
	}
}

void RenderPostProcess::DestroyPipelines()
{
	vkDestroyPipeline(Vk.Device, m_TemporalBlendPipeline, NULL);
	vkDestroyPipeline(Vk.Device, m_TemporalResolvePipeline, NULL);
	vkDestroyPipeline(Vk.Device, m_ToneMappingPipeline, NULL);
	vkDestroyPipeline(Vk.Device, m_DebugPipeline, NULL);
}

void RenderPostProcess::CreateResolutionDependentResources(const RenderContext& rc)
{
	VkTextureCreateParams temporal_texture_params;
	temporal_texture_params.Type = VK_IMAGE_TYPE_2D;
	temporal_texture_params.ViewType = VK_IMAGE_VIEW_TYPE_2D;
	temporal_texture_params.Width = rc.Width;
	temporal_texture_params.Height = rc.Height;
	temporal_texture_params.Format = VK_FORMAT_R16G16B16A16_SFLOAT;
	temporal_texture_params.Usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	temporal_texture_params.InitialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	m_TemporalTextures[0] = VkTextureCreate(temporal_texture_params);
	m_TemporalTextures[1] = VkTextureCreate(temporal_texture_params);
}

void RenderPostProcess::DestroyResolutionDependentResources()
{
	VkTextureDestroy(m_TemporalTextures[0]);
	VkTextureDestroy(m_TemporalTextures[1]);
}

void RenderPostProcess::RecreatePipelines(const RenderContext& rc)
{
	DestroyPipelines();
	CreatePipelines(rc);
}

void RenderPostProcess::RecreateResolutionDependentResources(const RenderContext& rc)
{
	DestroyResolutionDependentResources();
	CreateResolutionDependentResources(rc);
}

void RenderPostProcess::Jitter(RenderContext& rc)
{
	glm::vec2 jitter = glm::vec2(0.0f, 0.0f);
    if (m_TemporalAAEnable && !rc.DebugEnable)
    {
        const float halton_23[8][2] =
        {
            { 0.0f / 8.0f, 0.0f / 9.0f }, { 4.0f / 8.0f, 3.0f / 9.0f },
            { 2.0f / 8.0f, 6.0f / 9.0f }, { 6.0f / 8.0f, 1.0f / 9.0f },
            { 1.0f / 8.0f, 4.0f / 9.0f }, { 5.0f / 8.0f, 7.0f / 9.0f },
            { 3.0f / 8.0f, 2.0f / 9.0f }, { 7.0f / 8.0f, 5.0f / 9.0f },
        };
		jitter.x = halton_23[rc.FrameCounter % 8][0] / static_cast<float>(rc.Width);
		jitter.y = halton_23[rc.FrameCounter % 8][1] / static_cast<float>(rc.Height);
    }
	rc.CameraCurr.Jitter(jitter);
}

void RenderPostProcess::Draw(const RenderContext& rc, VkCommandBuffer cmd)
{
    if (m_TemporalAAEnable && !rc.DebugEnable)
    {
		VkPushLabel(cmd, "Post Process TAA");

        VkUtilImageBarrier(cmd, rc.ColorTexture.Image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
		VkUtilImageBarrier(cmd, rc.DepthTexture.Image, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);
        VkUtilImageBarrier(cmd, m_TemporalTextures[rc.FrameCounter & 1].Image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);

        // Temporal Blend
        {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_TemporalBlendPipeline);

			static uint32_t last_frame = ~0U;
			const bool is_hist_valid = last_frame == rc.FrameCounter;
			last_frame = rc.FrameCounter + 1;

            struct Constants
            {
				uint32_t	IsHistValid;
				float		Exposure;
            };
            VkAllocation constants_allocation = VkAllocateUploadBuffer(sizeof(Constants));
            Constants* constants = reinterpret_cast<Constants*>(constants_allocation.Data);
			constants->IsHistValid = is_hist_valid;
			constants->Exposure = m_Exposure;

            VkDescriptorSet set = VkCreateDescriptorSetForCurrentFrame(m_TemporalBlendDescriptorSetLayout,
                {
                    { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, constants_allocation.Buffer, constants_allocation.Offset, sizeof(Constants) },
                    { 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0, m_TemporalTextures[rc.FrameCounter & 1].ImageView, VK_IMAGE_LAYOUT_GENERAL, VK_NULL_HANDLE },
                    { 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, rc.ColorTexture.ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rc.NearestClamp },
					{ 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, rc.DepthTexture.ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rc.NearestClamp },
                    { 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, rc.MotionTexture.ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rc.NearestClamp },
                    { 5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, m_TemporalTextures[(rc.FrameCounter + 1) & 1].ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rc.LinearClamp },
                });
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_TemporalBlendPipelineLayout, 0, 1, &set, 0, NULL);

            vkCmdDispatch(cmd, (rc.Width + 7) / 8, (rc.Height + 7) / 8, 1);
        }

        VkUtilImageBarrier(cmd, rc.ColorTexture.Image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);
		VkUtilImageBarrier(cmd, rc.DepthTexture.Image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);
        VkUtilImageBarrier(cmd, m_TemporalTextures[rc.FrameCounter & 1].Image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);

        // Temporal Resolve
        {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_TemporalResolvePipeline);

            VkDescriptorSet set = VkCreateDescriptorSetForCurrentFrame(m_TemporalResolveDescriptorSetLayout,
                {
                    { 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0, rc.ColorTexture.ImageView, VK_IMAGE_LAYOUT_GENERAL, VK_NULL_HANDLE },
                    { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, m_TemporalTextures[rc.FrameCounter & 1].ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rc.NearestClamp },
                });
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_TemporalResolvePipelineLayout, 0, 1, &set, 0, NULL);

            vkCmdDispatch(cmd, (rc.Width + 7) / 8, (rc.Height + 7) / 8, 1);
        }

        VkUtilImageBarrier(cmd, rc.ColorTexture.Image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);

		VkPopLabel(cmd);
    }
    else
    {
        VkUtilImageBarrier(cmd, rc.ColorTexture.Image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
    }

	if (!rc.DebugEnable)
    {
		VkPushLabel(cmd, "Post Process Tone Mapping");

        VkRenderPassBeginInfo render_pass_info = {};
        render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_info.renderPass = rc.BackBufferRenderPass;
        render_pass_info.framebuffer = rc.BackBufferFramebuffers[Vk.SwapchainImageIndex];
        render_pass_info.renderArea.offset = { 0, 0 };
        render_pass_info.renderArea.extent = { rc.Width, rc.Height };
        vkCmdBeginRenderPass(cmd, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport = { 0.0f, 0.0f, static_cast<float>(rc.Width), static_cast<float>(rc.Height), 0.0f, 1.0f };
        VkRect2D scissor = { { 0, 0 }, { rc.Width, rc.Height } };
        vkCmdSetViewport(cmd, 0, 1, &viewport);
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ToneMappingPipeline);

        struct Constants
        {
            float   Exposure;
        };
        VkAllocation constants_allocation = VkAllocateUploadBuffer(sizeof(Constants));
        Constants* constants = reinterpret_cast<Constants*>(constants_allocation.Data);
        constants->Exposure = m_Exposure;

        VkDescriptorSet set = VkCreateDescriptorSetForCurrentFrame(m_ToneMappingDescriptorSetLayout,
            {
                { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, constants_allocation.Buffer, constants_allocation.Offset, sizeof(Constants) },
                { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, rc.ColorTexture.ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rc.NearestClamp }
            });
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ToneMappingPipelineLayout, 0, 1, &set, 0, NULL);

        vkCmdDraw(cmd, 3, 1, 0, 0);

        vkCmdEndRenderPass(cmd);

		VkPopLabel(cmd);
    }

	if (rc.DebugEnable)
	{
		VkRenderPassBeginInfo render_pass_info = {};
		render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		render_pass_info.renderPass = rc.BackBufferRenderPass;
		render_pass_info.framebuffer = rc.BackBufferFramebuffers[Vk.SwapchainImageIndex];
		render_pass_info.renderArea.offset = { 0, 0 };
		render_pass_info.renderArea.extent = { rc.Width, rc.Height };
		vkCmdBeginRenderPass(cmd, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = { 0.0f, 0.0f, static_cast<float>(rc.Width), static_cast<float>(rc.Height), 0.0f, 1.0f };
		VkRect2D scissor = { { 0, 0 }, { rc.Width, rc.Height } };
		vkCmdSetViewport(cmd, 0, 1, &viewport);
		vkCmdSetScissor(cmd, 0, 1, &scissor);

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_DebugPipeline);

		VkDescriptorSet set = VkCreateDescriptorSetForCurrentFrame(m_DebugDescriptorSetLayout,
			{
				{ 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, rc.ColorTexture.ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rc.NearestClamp }
			});
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_DebugPipelineLayout, 0, 1, &set, 0, NULL);

		vkCmdDraw(cmd, 3, 1, 0, 0);

		vkCmdEndRenderPass(cmd);
	}

	VkUtilImageBarrier(cmd, rc.ColorTexture.Image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
}