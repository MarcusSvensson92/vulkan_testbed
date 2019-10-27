#include "RenderMotion.h"
#include "VkUtil.h"


void RenderMotion::Create(const RenderContext& rc)
{
    // Generate
    {
        VkDescriptorSetLayoutBinding set_layout_bindings[] =
        {
            { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL },
            { 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL },
            { 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL },
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

	CreatePipelines(rc);
}

void RenderMotion::Destroy()
{
	DestroyPipelines();

    vkDestroyPipelineLayout(Vk.Device, m_GeneratePipelineLayout, NULL);
    vkDestroyDescriptorSetLayout(Vk.Device, m_GenerateDescriptorSetLayout, NULL);
}

void RenderMotion::CreatePipelines(const RenderContext& rc)
{
	// Generate
	{
		VkUtilCreateComputePipelineParams pipeline_params;
		pipeline_params.PipelineLayout = m_GeneratePipelineLayout;
		pipeline_params.ComputeShaderFilepath = "../Assets/Shaders/MotionGenerate.comp";
		m_GeneratePipeline = VkUtilCreateComputePipeline(pipeline_params);
	}
}

void RenderMotion::DestroyPipelines()
{
	vkDestroyPipeline(Vk.Device, m_GeneratePipeline, NULL);
}

void RenderMotion::RecreatePipelines(const RenderContext& rc)
{
	DestroyPipelines();
	CreatePipelines(rc);
}

void RenderMotion::Generate(const RenderContext& rc, VkCommandBuffer cmd)
{
	VkPushLabel(cmd, "Motion Generate");

	VkUtilImageBarrier(cmd, rc.MotionTexture.Image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_GeneratePipeline);

	const glm::mat4 curr = rc.CameraCurr.m_Projection * rc.CameraCurr.m_View;
	const glm::mat4 prev = rc.CameraCurr.m_Projection * rc.CameraPrev.m_View;
	const glm::mat4 curr_to_prev = prev * glm::inverse(curr);

	const float depth_param = (rc.CameraCurr.m_FarZ - rc.CameraCurr.m_NearZ) / rc.CameraCurr.m_NearZ;

	const glm::mat4 pre =
	{
		2.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 2.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f / depth_param, 0.0f,
		-1.0f, -1.0f, -1.0f / depth_param, 1.0f
	};
	const glm::mat4 post =
	{
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f
	};

	struct Constants
	{
		glm::mat4	m_CurrToPrev;
	};
	VkAllocation constants_allocation = VkAllocateUploadBuffer(sizeof(Constants));
	Constants* constants = reinterpret_cast<Constants*>(constants_allocation.Data);
	constants->m_CurrToPrev = post * curr_to_prev * pre;

	VkDescriptorSet set = VkCreateDescriptorSetForCurrentFrame(m_GenerateDescriptorSetLayout,
		{
			{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, constants_allocation.Buffer, constants_allocation.Offset, sizeof(Constants) },
			{ 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0, rc.MotionTexture.ImageView, VK_IMAGE_LAYOUT_GENERAL, VK_NULL_HANDLE },
			{ 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, rc.LinearDepthTextures[rc.FrameCounter & 1].ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rc.NearestClamp },
		});
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_GeneratePipelineLayout, 0, 1, &set, 0, NULL);

	vkCmdDispatch(cmd, (rc.Width + 7) / 8, (rc.Height + 7) / 8, 1);

	VkUtilImageBarrier(cmd, rc.MotionTexture.Image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);

	VkPopLabel(cmd);
}