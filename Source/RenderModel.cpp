#include "RenderModel.h"
#include "VkUtil.h"

void RenderModel::Create(const RenderContext& rc)
{
    VkDescriptorSetLayoutBinding set_layout_bindings[] =
    {
        { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, NULL },
        { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, NULL },
        { 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, NULL },
        { 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, NULL },
        { 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, NULL },
        { 5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, NULL },
        { 6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, NULL },
		{ 7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, NULL },
    };
    VkDescriptorSetLayoutCreateInfo set_layout_info = {};
    set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    set_layout_info.bindingCount = static_cast<uint32_t>(sizeof(set_layout_bindings) / sizeof(*set_layout_bindings));
    set_layout_info.pBindings = set_layout_bindings;
    VK(vkCreateDescriptorSetLayout(Vk.Device, &set_layout_info, NULL, &m_DescriptorSetLayout));

    VkPipelineLayoutCreateInfo pipeline_layout_info = {};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &m_DescriptorSetLayout;
    VK(vkCreatePipelineLayout(Vk.Device, &pipeline_layout_info, NULL, &m_PipelineLayout));

	CreatePipelines(rc);
}

void RenderModel::Destroy()
{
	DestroyPipelines();

    vkDestroyPipelineLayout(Vk.Device, m_PipelineLayout, NULL);
    vkDestroyDescriptorSetLayout(Vk.Device, m_DescriptorSetLayout, NULL);
}

void RenderModel::CreatePipelines(const RenderContext& rc)
{
	VkUtilCreateGraphicsPipelineParams depth_pipeline_params;
	depth_pipeline_params.PipelineLayout = m_PipelineLayout;
	depth_pipeline_params.RenderPass = rc.DepthRenderPass;
	depth_pipeline_params.VertexShaderFilepath = "../Assets/Shaders/ModelDepth.vert";
	depth_pipeline_params.FragmentShaderFilepath = "../Assets/Shaders/ModelDepth.frag";
	depth_pipeline_params.VertexBindingDescriptions =
	{
		{ 0, 12, VK_VERTEX_INPUT_RATE_VERTEX },
		{ 1, 8,  VK_VERTEX_INPUT_RATE_VERTEX },
		{ 2, 12, VK_VERTEX_INPUT_RATE_VERTEX },
	};
	depth_pipeline_params.VertexAttributeDescriptions =
	{
		{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },
		{ 1, 1, VK_FORMAT_R32G32_SFLOAT, 0 },
		{ 2, 2, VK_FORMAT_R32G32B32_SFLOAT, 0 },
	};
	depth_pipeline_params.RasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
	depth_pipeline_params.DepthStencilState.depthTestEnable = VK_TRUE;
	depth_pipeline_params.DepthStencilState.depthWriteEnable = VK_TRUE;
	depth_pipeline_params.BlendAttachmentStates = { VkUtilGetDefaultBlendAttachmentState(), VkUtilGetDefaultBlendAttachmentState() };
	m_PipelineDepth = VkUtilCreateGraphicsPipeline(depth_pipeline_params);

	VkUtilCreateGraphicsPipelineParams color_pipeline_params;
	color_pipeline_params.PipelineLayout = m_PipelineLayout;
	color_pipeline_params.RenderPass = rc.ColorRenderPass;
	color_pipeline_params.VertexShaderFilepath = "../Assets/Shaders/ModelColor.vert";
	color_pipeline_params.FragmentShaderFilepath = "../Assets/Shaders/ModelColor.frag";
	color_pipeline_params.VertexBindingDescriptions =
	{
		{ 0, 12, VK_VERTEX_INPUT_RATE_VERTEX },
		{ 1, 8,  VK_VERTEX_INPUT_RATE_VERTEX },
		{ 2, 12, VK_VERTEX_INPUT_RATE_VERTEX },
		{ 3, 16, VK_VERTEX_INPUT_RATE_VERTEX },
	};
	color_pipeline_params.VertexAttributeDescriptions =
	{
		{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },
		{ 1, 1, VK_FORMAT_R32G32_SFLOAT, 0 },
		{ 2, 2, VK_FORMAT_R32G32B32_SFLOAT, 0 },
		{ 3, 3, VK_FORMAT_R32G32B32A32_SFLOAT, 0 },
	};
	color_pipeline_params.RasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
	color_pipeline_params.DepthStencilState.depthTestEnable = VK_TRUE;
	color_pipeline_params.BlendAttachmentStates = { VkUtilGetDefaultBlendAttachmentState() };
	m_PipelineColor = VkUtilCreateGraphicsPipeline(color_pipeline_params);
}

void RenderModel::DestroyPipelines()
{
	vkDestroyPipeline(Vk.Device, m_PipelineDepth, NULL);
	vkDestroyPipeline(Vk.Device, m_PipelineColor, NULL);
}

void RenderModel::RecreatePipelines(const RenderContext& rc)
{
	DestroyPipelines();
	CreatePipelines(rc);
}

void RenderModel::SetAmbientLightLUT(VkImageView lut)
{
	m_AmbientLightLUT = lut;
}

void RenderModel::SetDirectionalLightLUT(VkImageView lut)
{
	m_DirectionalLightLUT = lut;
}

void RenderModel::DrawDepth(const RenderContext& rc, VkCommandBuffer cmd, uint32_t model_count, const GltfModel* models)
{
	VkPushLabel(cmd, "Models Depth");

	VkUtilImageBarrier(cmd, rc.NormalTexture.Image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
	VkUtilImageBarrier(cmd, rc.LinearDepthTextures[rc.FrameCounter & 1].Image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
	
    VkRenderPassBeginInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = rc.DepthRenderPass;
    render_pass_info.framebuffer = rc.DepthFramebuffers[rc.FrameCounter & 1];
    render_pass_info.renderArea.offset = { 0, 0 };
    render_pass_info.renderArea.extent = { rc.Width, rc.Height };
    vkCmdBeginRenderPass(cmd, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = { 0.0f, 0.0f, static_cast<float>(rc.Width), static_cast<float>(rc.Height), 0.0f, 1.0f };
    VkRect2D scissor = { { 0, 0 }, { rc.Width, rc.Height } };
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    VkClearRect clear_rect = {};
    clear_rect.baseArrayLayer = 0;
    clear_rect.layerCount = 1;
    clear_rect.rect.offset.x = 0;
    clear_rect.rect.offset.y = 0;
    clear_rect.rect.extent.width = rc.Width;
    clear_rect.rect.extent.height = rc.Height;

    VkClearAttachment clear_attachments[] =
    {
		{ VK_IMAGE_ASPECT_COLOR_BIT, 0, { 0.5f, 0.5f, 0.5f, 0.0f } },
		{ VK_IMAGE_ASPECT_COLOR_BIT, 1, { 1.0f, 0.0f, 0.0f, 0.0f } },
        { VK_IMAGE_ASPECT_DEPTH_BIT, 0, { 0.0f, 0 } },
    };
    vkCmdClearAttachments(cmd, static_cast<uint32_t>(sizeof(clear_attachments) / sizeof(*clear_attachments)), clear_attachments, 1, &clear_rect);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineDepth);

	const glm::mat4 view_projection = rc.CameraCurr.m_Projection * rc.CameraCurr.m_View;

    for (uint32_t i = 0; i < model_count; ++i)
    {
        const GltfModel& model = models[i];

        model.BindVertexBuffer(cmd, 0, VERTEX_ATTRIBUTE_POSITION);
        model.BindVertexBuffer(cmd, 1, VERTEX_ATTRIBUTE_TEXCOORD);
		model.BindVertexBuffer(cmd, 2, VERTEX_ATTRIBUTE_NORMAL);
        model.BindIndexBuffer(cmd);

		const uint32_t instance_count = static_cast<uint32_t>(model.m_Instances.size());
		for (uint32_t j = 0; j < instance_count; ++j)
		{
			const GltfInstance& instance = model.m_Instances[j];

			for (uint32_t k = instance.MeshOffset; k < (instance.MeshOffset + instance.MeshCount); ++k)
			{
				const uint32_t material_index = model.m_Meshes[k].MaterialIndex;
				const GltfMaterial& material = model.m_Materials[material_index];

				const VkTexture& base_color_texture = model.m_Textures[material.BaseColorTextureIndex];
				const VkTexture& normal_texture = model.m_Textures[material.NormalTextureIndex];
				const VkTexture& metallic_roughness_texture = model.m_Textures[material.MetallicRoughnessTextureIndex];

				struct Constants
				{
					glm::mat4   World;
					glm::mat4	WorldViewProjection;
					float		DepthParam;
				};
				VkAllocation constants_allocation = VkAllocateUploadBuffer(sizeof(Constants));
				Constants* constants = reinterpret_cast<Constants*>(constants_allocation.Data);
				constants->World = instance.Transform;
				constants->WorldViewProjection = view_projection * instance.Transform;
				constants->DepthParam = (rc.CameraCurr.m_FarZ - rc.CameraCurr.m_NearZ) / rc.CameraCurr.m_NearZ;

				VkDescriptorSet set = VkCreateDescriptorSetForCurrentFrame(m_DescriptorSetLayout,
					{
						{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, constants_allocation.Buffer, constants_allocation.Offset, sizeof(Constants) },
						{ 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, base_color_texture.ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rc.AnisoWrap },
						{ 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, normal_texture.ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rc.AnisoWrap },
						{ 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, metallic_roughness_texture.ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rc.AnisoWrap },
						{ 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, m_AmbientLightLUT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rc.LinearClamp },
						{ 5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, m_DirectionalLightLUT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rc.LinearClamp },
						{ 6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, rc.AmbientOcclusionTexture.ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rc.NearestClamp },
						{ 7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, rc.ShadowTexture.ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rc.NearestClamp }
					});
				vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &set, 0, NULL);

				model.Draw(cmd, k);
			}
		}
    }

    vkCmdEndRenderPass(cmd);

	VkUtilImageBarrier(cmd, rc.NormalTexture.Image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
	VkUtilImageBarrier(cmd, rc.LinearDepthTextures[rc.FrameCounter & 1].Image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
	
	VkPopLabel(cmd);
}

void RenderModel::DrawColor(const RenderContext& rc, VkCommandBuffer cmd, uint32_t model_count, const GltfModel* models)
{
	VkPushLabel(cmd, "Models Color");

    VkRenderPassBeginInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = rc.ColorRenderPass;
    render_pass_info.framebuffer = rc.ColorFramebuffer;
    render_pass_info.renderArea.offset = { 0, 0 };
    render_pass_info.renderArea.extent = { rc.Width, rc.Height };
    vkCmdBeginRenderPass(cmd, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = { 0.0f, 0.0f, static_cast<float>(rc.Width), static_cast<float>(rc.Height), 0.0f, 1.0f };
    VkRect2D scissor = { { 0, 0 }, { rc.Width, rc.Height } };
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    VkClearRect clear_rect = {};
    clear_rect.baseArrayLayer = 0;
    clear_rect.layerCount = 1;
    clear_rect.rect.offset.x = 0;
    clear_rect.rect.offset.y = 0;
    clear_rect.rect.extent.width = rc.Width;
    clear_rect.rect.extent.height = rc.Height;

    VkClearAttachment clear_attachments[] =
    {
        { VK_IMAGE_ASPECT_COLOR_BIT, 0, { 0.0f, 0.0f, 0.0f, 0.0f } },
    };
    vkCmdClearAttachments(cmd, static_cast<uint32_t>(sizeof(clear_attachments) / sizeof(*clear_attachments)), clear_attachments, 1, &clear_rect);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineColor);

	const glm::mat4 view_projection = rc.CameraCurr.m_Projection * rc.CameraCurr.m_View;

    for (uint32_t i = 0; i < model_count; ++i)
    {
        const GltfModel& model = models[i];

        model.BindVertexBuffer(cmd, 0, VERTEX_ATTRIBUTE_POSITION);
        model.BindVertexBuffer(cmd, 1, VERTEX_ATTRIBUTE_TEXCOORD);
		model.BindVertexBuffer(cmd, 2, VERTEX_ATTRIBUTE_NORMAL);
		model.BindVertexBuffer(cmd, 3, VERTEX_ATTRIBUTE_TANGENT);
        model.BindIndexBuffer(cmd);

		const uint32_t instance_count = static_cast<uint32_t>(model.m_Instances.size());
		for (uint32_t j = 0; j < instance_count; ++j)
		{
			const GltfInstance& instance = model.m_Instances[j];

			for (uint32_t k = instance.MeshOffset; k < (instance.MeshOffset + instance.MeshCount); ++k)
			{
				const uint32_t material_index = model.m_Meshes[k].MaterialIndex;
				const GltfMaterial& material = model.m_Materials[material_index];

				const VkTexture& base_color_texture = model.m_Textures[material.BaseColorTextureIndex];
				const VkTexture& normal_texture = model.m_Textures[material.NormalTextureIndex];
				const VkTexture& metallic_roughness_texture = model.m_Textures[material.MetallicRoughnessTextureIndex];

				struct Constants
				{
					glm::mat4   World;
					glm::mat4	WorldViewProjection;
					glm::vec3	ViewPosition;
					float	    AmbientLightIntensity;
					glm::vec3	LightDirection;
					float	    DirectionalLightIntensity;
					glm::vec4	BaseColorFactor;
					glm::vec2	MetallicRoughnessFactor;
					uint32_t	HasBaseColorTexture;
					uint32_t	HasNormalTexture;
					uint32_t	HasMetallicRoughnessTexture;
					uint32_t	DebugEnable;
					uint32_t	DebugIndex;
				};
				VkAllocation constants_allocation = VkAllocateUploadBuffer(sizeof(Constants));
				Constants* constants = reinterpret_cast<Constants*>(constants_allocation.Data);
				constants->World = instance.Transform;
				constants->WorldViewProjection = view_projection * instance.Transform;
				constants->ViewPosition = rc.CameraCurr.m_Position;
				constants->AmbientLightIntensity = m_AmbientLightIntensity;
				constants->LightDirection = glm::normalize(rc.SunDirection);
				constants->DirectionalLightIntensity = m_DirectionalLightIntensity;
				constants->BaseColorFactor = material.BaseColorFactor;
				constants->MetallicRoughnessFactor = material.MetallicRoughnessFactor;
				constants->HasBaseColorTexture = material.HasBaseColorTexture;
				constants->HasNormalTexture = material.HasNormalTexture;
				constants->HasMetallicRoughnessTexture = material.HasMetallicRoughnessTexture;
				constants->DebugEnable = rc.DebugEnable;
				constants->DebugIndex = rc.DebugIndex;

				VkDescriptorSet set = VkCreateDescriptorSetForCurrentFrame(m_DescriptorSetLayout,
					{
						{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, constants_allocation.Buffer, constants_allocation.Offset, sizeof(Constants) },
						{ 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, base_color_texture.ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rc.AnisoWrap },
						{ 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, normal_texture.ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rc.AnisoWrap },
						{ 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, metallic_roughness_texture.ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rc.AnisoWrap },
						{ 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, m_AmbientLightLUT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rc.LinearClamp },
						{ 5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, m_DirectionalLightLUT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rc.LinearClamp },
						{ 6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, rc.AmbientOcclusionTexture.ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rc.NearestClamp },
						{ 7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, rc.ShadowTexture.ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rc.NearestClamp }
					});
				vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &set, 0, NULL);

				model.Draw(cmd, k);
			}
		}
    }

    vkCmdEndRenderPass(cmd);

	VkPopLabel(cmd);
}