#include "RenderSSAO.h"
#include "VkUtil.h"

void RenderSSAO::Create(const RenderContext& rc)
{
    // Generate
    {
        VkDescriptorSetLayoutBinding set_layout_bindings[] =
        {
            { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL },
            { 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL },
            { 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL },
			{ 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL },
			{ 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL },
        };
        VkDescriptorSetLayoutCreateInfo set_layout_info = {};
        set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        set_layout_info.bindingCount = static_cast<uint32_t>(sizeof(set_layout_bindings) / sizeof(*set_layout_bindings));
        set_layout_info.pBindings = set_layout_bindings;
        VK(vkCreateDescriptorSetLayout(Vk.Device, &set_layout_info, NULL, &m_GenerateDescriptorSetLayout));

        VkPipelineLayoutCreateInfo pipeline_layout_info = {};
        pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_info.setLayoutCount = 1;
        pipeline_layout_info.pSetLayouts = &m_GenerateDescriptorSetLayout;
        VK(vkCreatePipelineLayout(Vk.Device, &pipeline_layout_info, NULL, &m_GeneratePipelineLayout));
    }

	// Blur
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
		VK(vkCreateDescriptorSetLayout(Vk.Device, &set_layout_info, NULL, &m_BlurDescriptorSetLayout));

		VkPipelineLayoutCreateInfo pipeline_layout_info = {};
		pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_info.setLayoutCount = 1;
		pipeline_layout_info.pSetLayouts = &m_BlurDescriptorSetLayout;
		VK(vkCreatePipelineLayout(Vk.Device, &pipeline_layout_info, NULL, &m_BlurPipelineLayout));
	}

	CreatePipelines(rc);
	CreateResolutionDependentResources(rc);
}

void RenderSSAO::Destroy()
{
	DestroyResolutionDependentResources();
	DestroyPipelines();

    vkDestroyPipelineLayout(Vk.Device, m_GeneratePipelineLayout, NULL);
    vkDestroyDescriptorSetLayout(Vk.Device, m_GenerateDescriptorSetLayout, NULL);

	vkDestroyPipelineLayout(Vk.Device, m_BlurPipelineLayout, NULL);
	vkDestroyDescriptorSetLayout(Vk.Device, m_BlurDescriptorSetLayout, NULL);
}

void RenderSSAO::CreatePipelines(const RenderContext& rc)
{
	// Generate
	{
		VkUtilCreateComputePipelineParams pipeline_params;
		pipeline_params.PipelineLayout = m_GeneratePipelineLayout;
		pipeline_params.ComputeShaderFilepath = "../Assets/Shaders/SSAOGenerate.comp";
		m_GeneratePipeline = VkUtilCreateComputePipeline(pipeline_params);
	}

	// Blur
	{
		VkUtilCreateComputePipelineParams pipeline_params;
		pipeline_params.PipelineLayout = m_BlurPipelineLayout;
		pipeline_params.ComputeShaderFilepath = "../Assets/Shaders/SSAOBlur.comp";
		m_BlurPipeline = VkUtilCreateComputePipeline(pipeline_params);
	}
}

void RenderSSAO::DestroyPipelines()
{
	vkDestroyPipeline(Vk.Device, m_GeneratePipeline, NULL);
	vkDestroyPipeline(Vk.Device, m_BlurPipeline, NULL);
}

void RenderSSAO::CreateResolutionDependentResources(const RenderContext& rc)
{
	VkTextureCreateParams intermediate_texture_params;
	intermediate_texture_params.Type = VK_IMAGE_TYPE_2D;
	intermediate_texture_params.ViewType = VK_IMAGE_VIEW_TYPE_2D;
	intermediate_texture_params.Width = rc.Width;
	intermediate_texture_params.Height = rc.Height;
	intermediate_texture_params.Format = VK_FORMAT_R8_UNORM;
	intermediate_texture_params.Usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	intermediate_texture_params.InitialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	m_IntermediateTexture = VkTextureCreate(intermediate_texture_params);
}

void RenderSSAO::DestroyResolutionDependentResources()
{
	VkTextureDestroy(m_IntermediateTexture);
}

void RenderSSAO::RecreatePipelines(const RenderContext& rc)
{
	DestroyPipelines();
	CreatePipelines(rc);
}

void RenderSSAO::RecreateResolutionDependentResources(const RenderContext& rc)
{
	DestroyResolutionDependentResources();
	CreateResolutionDependentResources(rc);
}

void RenderSSAO::Generate(const RenderContext& rc, VkCommandBuffer cmd)
{
	if (!rc.EnableScreenSpaceAmbientOcclusion)
	{
		return;
	}

	{
		VkPushLabel(cmd, "SSAO Generate");

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_GeneratePipeline);

		VkUtilImageBarrier(cmd, rc.DepthTexture.Image, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);
		VkUtilImageBarrier(cmd, rc.ScreenSpaceAmbientOcclusionTexture.Image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);

		struct Constants
		{
			glm::mat4	InvProj;
			glm::mat4	View;
			float		ScreenRadius;
			float		NegInvRadius;
			float		Bias;
			float		Multiplier;
			float		Intensity;
		};
		VkAllocation constants_allocation = VkAllocateUploadBuffer(sizeof(Constants));
		Constants* constants = reinterpret_cast<Constants*>(constants_allocation.Data);
		constants->InvProj = glm::inverse(rc.CameraCurr.m_Projection);
		constants->View = rc.CameraCurr.m_View;
		constants->ScreenRadius = -m_Radius * 0.25f * static_cast<float>(rc.Height) / tanf(0.5f * rc.CameraCurr.m_FovY);
		constants->NegInvRadius = -1.0f / m_Radius;
		constants->Bias = fmaxf(0.0f, fminf(0.99f, m_Bias));
		constants->Multiplier = 1.0f / (1.0f - constants->Bias);
		constants->Intensity = m_Intensity;

		VkDescriptorSet set = VkCreateDescriptorSetForCurrentFrame(m_GenerateDescriptorSetLayout,
			{
				{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, constants_allocation.Buffer, constants_allocation.Offset, sizeof(Constants) },
				{ 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0, rc.ScreenSpaceAmbientOcclusionTexture.ImageView, VK_IMAGE_LAYOUT_GENERAL, VK_NULL_HANDLE },
				{ 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, rc.DepthTexture.ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rc.NearestClamp },
				{ 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, rc.NormalTexture.ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rc.NearestClamp },
				{ 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, rc.BlueNoiseTextures[rc.FrameCounter % 8].ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rc.NearestClamp },
			});
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_GeneratePipelineLayout, 0, 1, &set, 0, NULL);

		vkCmdDispatch(cmd, (rc.Width + 7) / 8, (rc.Height + 7) / 8, 1);

		VkUtilImageBarrier(cmd, rc.DepthTexture.Image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);
		VkUtilImageBarrier(cmd, rc.ScreenSpaceAmbientOcclusionTexture.Image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);

		VkPopLabel(cmd);
	}

	if (m_Blur)
	{
		VkPushLabel(cmd, "SSAO Blur");

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_BlurPipeline);

		VkUtilImageBarrier(cmd, m_IntermediateTexture.Image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);

		{
			struct Constants
			{
				glm::ivec2 Direction;
			};
			VkAllocation constants_allocation = VkAllocateUploadBuffer(sizeof(Constants));
			Constants* constants = reinterpret_cast<Constants*>(constants_allocation.Data);
			constants->Direction = glm::ivec2(2, 0);

			VkDescriptorSet set = VkCreateDescriptorSetForCurrentFrame(m_BlurDescriptorSetLayout,
				{
					{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, constants_allocation.Buffer, constants_allocation.Offset, sizeof(Constants) },
					{ 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0, m_IntermediateTexture.ImageView, VK_IMAGE_LAYOUT_GENERAL, VK_NULL_HANDLE },
					{ 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, rc.ScreenSpaceAmbientOcclusionTexture.ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rc.NearestClamp },
					{ 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, rc.LinearDepthTextures[rc.FrameCounter & 1].ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rc.NearestClamp },
				});
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_BlurPipelineLayout, 0, 1, &set, 0, NULL);

			vkCmdDispatch(cmd, (rc.Width + 7) / 8, (rc.Height + 7) / 8, 1);
		}

		VkUtilImageBarrier(cmd, m_IntermediateTexture.Image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
		VkUtilImageBarrier(cmd, rc.ScreenSpaceAmbientOcclusionTexture.Image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);

		{
			struct Constants
			{
				glm::ivec2 Direction;
			};
			VkAllocation constants_allocation = VkAllocateUploadBuffer(sizeof(Constants));
			Constants* constants = reinterpret_cast<Constants*>(constants_allocation.Data);
			constants->Direction = glm::ivec2(0, 2);

			VkDescriptorSet set = VkCreateDescriptorSetForCurrentFrame(m_BlurDescriptorSetLayout,
				{
					{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, constants_allocation.Buffer, constants_allocation.Offset, sizeof(Constants) },
					{ 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0, rc.ScreenSpaceAmbientOcclusionTexture.ImageView, VK_IMAGE_LAYOUT_GENERAL, VK_NULL_HANDLE },
					{ 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, m_IntermediateTexture.ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rc.NearestClamp },
					{ 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, rc.LinearDepthTextures[rc.FrameCounter & 1].ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rc.NearestClamp },
				});
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_BlurPipelineLayout, 0, 1, &set, 0, NULL);

			vkCmdDispatch(cmd, (rc.Width + 7) / 8, (rc.Height + 7) / 8, 1);
		}

		VkUtilImageBarrier(cmd, rc.ScreenSpaceAmbientOcclusionTexture.Image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);

		VkPopLabel(cmd);
	}
}