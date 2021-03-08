#pragma once

#include "RenderContext.h"
#include "GltfModel.h"

class RenderModel
{
public:
    VkDescriptorSetLayout   m_DescriptorSetLayout		= VK_NULL_HANDLE;
    VkPipelineLayout        m_PipelineLayout			= VK_NULL_HANDLE;
    VkPipeline              m_PipelineDepth				= VK_NULL_HANDLE;
    VkPipeline              m_PipelineColor				= VK_NULL_HANDLE;

	VkImageView				m_AmbientLightLUT			= VK_NULL_HANDLE;
	VkImageView				m_DirectionalLightLUT		= VK_NULL_HANDLE;

	float                   m_AmbientLightIntensity		= 60.0f;
	float                   m_DirectionalLightIntensity = 40.0f;

    void                    Create(const RenderContext& rc);
    void                    Destroy();

	void					RecreatePipelines(const RenderContext& rc);

	void					SetAmbientLightLUT(VkImageView lut);
	void					SetDirectionalLightLUT(VkImageView lut);

    void                    DrawDepth(const RenderContext& rc, VkCommandBuffer cmd, uint32_t model_count, const GltfModel* models);
    void                    DrawColor(const RenderContext& rc, VkCommandBuffer cmd, uint32_t model_count, const GltfModel* models);

private:
	void					CreatePipelines(const RenderContext& rc);
	void					DestroyPipelines();
};
