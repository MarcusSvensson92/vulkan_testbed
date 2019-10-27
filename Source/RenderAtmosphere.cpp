#include "RenderAtmosphere.h"
#include "VkUtil.h"

void RenderAtmosphere::Create(const RenderContext& rc)
{
    VkTextureCreateParams ambient_light_lut_params;
    ambient_light_lut_params.Type = VK_IMAGE_TYPE_1D;
    ambient_light_lut_params.ViewType = VK_IMAGE_VIEW_TYPE_1D;
    ambient_light_lut_params.Width = 256;
    ambient_light_lut_params.Format = VK_FORMAT_R16G16B16A16_SFLOAT;
    ambient_light_lut_params.Usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    ambient_light_lut_params.InitialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    m_AmbientLightLUT = VkTextureCreate(ambient_light_lut_params);

    VkTextureCreateParams directional_light_lut_params;
    directional_light_lut_params.Type = VK_IMAGE_TYPE_1D;
    directional_light_lut_params.ViewType = VK_IMAGE_VIEW_TYPE_1D;
    directional_light_lut_params.Width = 256;
    directional_light_lut_params.Format = VK_FORMAT_R16G16B16A16_SFLOAT;
    directional_light_lut_params.Usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    directional_light_lut_params.InitialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    m_DirectionalLightLUT = VkTextureCreate(directional_light_lut_params);

    VkTextureCreateParams sky_lut_params;
    sky_lut_params.Type = VK_IMAGE_TYPE_2D;
    sky_lut_params.ViewType = VK_IMAGE_VIEW_TYPE_2D;
    sky_lut_params.Width = 128;
    sky_lut_params.Height = 32;
    sky_lut_params.Format = VK_FORMAT_R16G16B16A16_SFLOAT;
    sky_lut_params.Usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    sky_lut_params.InitialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    m_SkyLUTR = VkTextureCreate(sky_lut_params);
    m_SkyLUTM = VkTextureCreate(sky_lut_params);

	// Precompute Ambient Light LUT
	{
		VkDescriptorSetLayoutBinding set_layout_bindings[] =
		{
			{ 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL },
		};
		VkDescriptorSetLayoutCreateInfo set_layout_info = {};
		set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		set_layout_info.bindingCount = static_cast<uint32_t>(sizeof(set_layout_bindings) / sizeof(*set_layout_bindings));
		set_layout_info.pBindings = set_layout_bindings;
		VK(vkCreateDescriptorSetLayout(Vk.Device, &set_layout_info, NULL, &m_PrecomputeAmbientLightLUTDescriptorSetLayout));

		VkPipelineLayoutCreateInfo pipeline_layout_info = {};
		pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_info.setLayoutCount = 1;
		pipeline_layout_info.pSetLayouts = &m_PrecomputeAmbientLightLUTDescriptorSetLayout;
		VK(vkCreatePipelineLayout(Vk.Device, &pipeline_layout_info, NULL, &m_PrecomputeAmbientLightLUTPipelineLayout));
	}

	// Precompute Directional Light LUT
	{
		VkDescriptorSetLayoutBinding set_layout_bindings[] =
		{
			{ 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL },
		};
		VkDescriptorSetLayoutCreateInfo set_layout_info = {};
		set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		set_layout_info.bindingCount = static_cast<uint32_t>(sizeof(set_layout_bindings) / sizeof(*set_layout_bindings));
		set_layout_info.pBindings = set_layout_bindings;
		VK(vkCreateDescriptorSetLayout(Vk.Device, &set_layout_info, NULL, &m_PrecomputeDirectionalLightLUTDescriptorSetLayout));

		VkPipelineLayoutCreateInfo pipeline_layout_info = {};
		pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_info.setLayoutCount = 1;
		pipeline_layout_info.pSetLayouts = &m_PrecomputeDirectionalLightLUTDescriptorSetLayout;
		VK(vkCreatePipelineLayout(Vk.Device, &pipeline_layout_info, NULL, &m_PrecomputeDirectionalLightLUTPipelineLayout));
	}

	// Precompute Sky LUT
	{
		VkDescriptorSetLayoutBinding set_layout_bindings[] =
		{
			{ 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL },
			{ 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL },
		};
		VkDescriptorSetLayoutCreateInfo set_layout_info = {};
		set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		set_layout_info.bindingCount = static_cast<uint32_t>(sizeof(set_layout_bindings) / sizeof(*set_layout_bindings));
		set_layout_info.pBindings = set_layout_bindings;
		VK(vkCreateDescriptorSetLayout(Vk.Device, &set_layout_info, NULL, &m_PrecomputeSkyLUTDescriptorSetLayout));

		VkPipelineLayoutCreateInfo pipeline_layout_info = {};
		pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_info.setLayoutCount = 1;
		pipeline_layout_info.pSetLayouts = &m_PrecomputeSkyLUTDescriptorSetLayout;
		VK(vkCreatePipelineLayout(Vk.Device, &pipeline_layout_info, NULL, &m_PrecomputeSkyLUTPipelineLayout));
	}

	// Sky
	{
		VkDescriptorSetLayoutBinding set_layout_bindings[] =
		{
			{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, NULL },
			{ 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, NULL },
			{ 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, NULL },
		};
		VkDescriptorSetLayoutCreateInfo set_layout_info = {};
		set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		set_layout_info.bindingCount = static_cast<uint32_t>(sizeof(set_layout_bindings) / sizeof(*set_layout_bindings));
		set_layout_info.pBindings = set_layout_bindings;
		VK(vkCreateDescriptorSetLayout(Vk.Device, &set_layout_info, NULL, &m_SkyDescriptorSetLayout));

		VkPipelineLayoutCreateInfo pipeline_layout_info = {};
		pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_info.setLayoutCount = 1;
		pipeline_layout_info.pSetLayouts = &m_SkyDescriptorSetLayout;
		VK(vkCreatePipelineLayout(Vk.Device, &pipeline_layout_info, NULL, &m_SkyPipelineLayout));
	}

	CreatePipelines(rc);
}

void RenderAtmosphere::Destroy()
{
	DestroyPipelines();

	vkDestroyPipelineLayout(Vk.Device, m_SkyPipelineLayout, NULL);
	vkDestroyDescriptorSetLayout(Vk.Device, m_SkyDescriptorSetLayout, NULL);

	vkDestroyPipelineLayout(Vk.Device, m_PrecomputeSkyLUTPipelineLayout, NULL);
	vkDestroyDescriptorSetLayout(Vk.Device, m_PrecomputeSkyLUTDescriptorSetLayout, NULL);

	vkDestroyPipelineLayout(Vk.Device, m_PrecomputeDirectionalLightLUTPipelineLayout, NULL);
	vkDestroyDescriptorSetLayout(Vk.Device, m_PrecomputeDirectionalLightLUTDescriptorSetLayout, NULL);

	vkDestroyPipelineLayout(Vk.Device, m_PrecomputeAmbientLightLUTPipelineLayout, NULL);
	vkDestroyDescriptorSetLayout(Vk.Device, m_PrecomputeAmbientLightLUTDescriptorSetLayout, NULL);

	VkTextureDestroy(m_AmbientLightLUT);
	VkTextureDestroy(m_DirectionalLightLUT);
	VkTextureDestroy(m_SkyLUTR);
	VkTextureDestroy(m_SkyLUTM);
}

void RenderAtmosphere::CreatePipelines(const RenderContext& rc)
{
	// Precompute Ambient Light LUT
	{
		VkUtilCreateComputePipelineParams pipeline_params;
		pipeline_params.PipelineLayout = m_PrecomputeAmbientLightLUTPipelineLayout;
		pipeline_params.ComputeShaderFilepath = "../Assets/Shaders/AtmospherePrecomputeAmbientLightLUT.comp";
		m_PrecomputeAmbientLightLUTPipeline = VkUtilCreateComputePipeline(pipeline_params);
	}

	// Precompute Directional Light LUT
	{
		VkUtilCreateComputePipelineParams pipeline_params;
		pipeline_params.PipelineLayout = m_PrecomputeDirectionalLightLUTPipelineLayout;
		pipeline_params.ComputeShaderFilepath = "../Assets/Shaders/AtmospherePrecomputeDirectionalLightLUT.comp";
		m_PrecomputeDirectionalLightLUTPipeline = VkUtilCreateComputePipeline(pipeline_params);
	}

	// Precompute Sky LUT
	{
		VkUtilCreateComputePipelineParams pipeline_params;
		pipeline_params.PipelineLayout = m_PrecomputeSkyLUTPipelineLayout;
		pipeline_params.ComputeShaderFilepath = "../Assets/Shaders/AtmospherePrecomputeSkyLUT.comp";
		m_PrecomputeSkyLUTPipeline = VkUtilCreateComputePipeline(pipeline_params);
	}

	// Sky
	{
		VkUtilCreateGraphicsPipelineParams pipeline_params;
		pipeline_params.PipelineLayout = m_SkyPipelineLayout;
		pipeline_params.RenderPass = rc.ColorRenderPass;
		pipeline_params.VertexShaderFilepath = "../Assets/Shaders/AtmosphereSky.vert";
		pipeline_params.FragmentShaderFilepath = "../Assets/Shaders/AtmosphereSky.frag";
		pipeline_params.DepthStencilState.depthTestEnable = VK_TRUE;
		pipeline_params.BlendAttachmentStates = { VkUtilGetDefaultBlendAttachmentState() };
		m_SkyPipeline = VkUtilCreateGraphicsPipeline(pipeline_params);
	}

	VkRecordCommands(
        [=](VkCommandBuffer cmd) mutable
        {
			VkUtilImageBarrier(cmd, m_AmbientLightLUT.Image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);
			VkUtilImageBarrier(cmd, m_DirectionalLightLUT.Image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);
			VkUtilImageBarrier(cmd, m_SkyLUTR.Image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);
			VkUtilImageBarrier(cmd, m_SkyLUTM.Image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);

            // Precompute Ambient Light LUT
            {
                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_PrecomputeAmbientLightLUTPipeline);

                VkDescriptorSet set = VkCreateDescriptorSetForCurrentFrame(m_PrecomputeAmbientLightLUTDescriptorSetLayout,
                    {
                        { 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0, m_AmbientLightLUT.ImageView, VK_IMAGE_LAYOUT_GENERAL }
                    });
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_PrecomputeAmbientLightLUTPipelineLayout, 0, 1, &set, 0, NULL);

                vkCmdDispatch(cmd, (m_AmbientLightLUT.Width + 63) / 64, 1, 1);
            }

            // Precompute Directional Light LUT
            {
                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_PrecomputeDirectionalLightLUTPipeline);

                VkDescriptorSet set = VkCreateDescriptorSetForCurrentFrame(m_PrecomputeDirectionalLightLUTDescriptorSetLayout,
                    {
                        { 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0, m_DirectionalLightLUT.ImageView, VK_IMAGE_LAYOUT_GENERAL }
                    });
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_PrecomputeDirectionalLightLUTPipelineLayout, 0, 1, &set, 0, NULL);

                vkCmdDispatch(cmd, (m_DirectionalLightLUT.Width + 63) / 64, 1, 1);
            }

            // Precompute Sky LUT
            {
                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_PrecomputeSkyLUTPipeline);

                VkDescriptorSet set = VkCreateDescriptorSetForCurrentFrame(m_PrecomputeSkyLUTDescriptorSetLayout,
                    {
                        { 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0, m_SkyLUTR.ImageView, VK_IMAGE_LAYOUT_GENERAL },
						{ 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0, m_SkyLUTM.ImageView, VK_IMAGE_LAYOUT_GENERAL },
                    });
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_PrecomputeSkyLUTPipelineLayout, 0, 1, &set, 0, NULL);

                vkCmdDispatch(cmd, (m_SkyLUTR.Width + 7) / 8, (m_SkyLUTR.Height + 7) / 8, 1);
            }

            VkUtilImageBarrier(cmd, m_AmbientLightLUT.Image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
            VkUtilImageBarrier(cmd, m_DirectionalLightLUT.Image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
            VkUtilImageBarrier(cmd, m_SkyLUTR.Image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
            VkUtilImageBarrier(cmd, m_SkyLUTM.Image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
        });
}
void RenderAtmosphere::DestroyPipelines()
{
	vkDestroyPipeline(Vk.Device, m_PrecomputeAmbientLightLUTPipeline, NULL);
	vkDestroyPipeline(Vk.Device, m_PrecomputeDirectionalLightLUTPipeline, NULL);
	vkDestroyPipeline(Vk.Device, m_PrecomputeSkyLUTPipeline, NULL);
	vkDestroyPipeline(Vk.Device, m_SkyPipeline, NULL);
}

void RenderAtmosphere::RecreatePipelines(const RenderContext& rc)
{
	DestroyPipelines();
	CreatePipelines(rc);
}

void RenderAtmosphere::DrawSky(const RenderContext& rc, VkCommandBuffer cmd)
{
	if (rc.DebugEnable)
	{
		return;
	}

	VkPushLabel(cmd, "Atmosphere Sky");

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

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_SkyPipeline);

    glm::mat4 view = rc.CameraCurr.m_View;
    view[3][0] = view[3][1] = view[3][2] = 0.0f; // Set translation to zero

    struct Constants
    {
        glm::mat4	InvViewProjZeroTranslation;
        glm::vec3	LightDirection;
        float	    LightIntensity;
    };
    VkAllocation constants_allocation = VkAllocateUploadBuffer(sizeof(Constants));
    Constants* constants = reinterpret_cast<Constants*>(constants_allocation.Data);
    constants->InvViewProjZeroTranslation = glm::inverse(rc.CameraCurr.m_Projection * view);
    constants->LightDirection = glm::normalize(rc.SunDirection);
    constants->LightIntensity = m_SkyLightIntensity;

    VkDescriptorSet set = VkCreateDescriptorSetForCurrentFrame(m_SkyDescriptorSetLayout,
        {
            { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, constants_allocation.Buffer, constants_allocation.Offset, sizeof(Constants) },
            { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, m_SkyLUTR.ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rc.LinearClamp },
            { 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, m_SkyLUTM.ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rc.LinearClamp },
        });
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_SkyPipelineLayout, 0, 1, &set, 0, NULL);

    vkCmdDraw(cmd, 3, 1, 0, 0);

    vkCmdEndRenderPass(cmd);

	VkPopLabel(cmd);
}
