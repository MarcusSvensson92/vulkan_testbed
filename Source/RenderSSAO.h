#pragma once

#include "RenderContext.h"

class RenderSSAO
{
public:
    VkDescriptorSetLayout   m_GenerateDescriptorSetLayout	= VK_NULL_HANDLE;
    VkPipelineLayout        m_GeneratePipelineLayout		= VK_NULL_HANDLE;
    VkPipeline              m_GeneratePipeline				= VK_NULL_HANDLE;

	VkDescriptorSetLayout   m_BlurDescriptorSetLayout		= VK_NULL_HANDLE;
    VkPipelineLayout        m_BlurPipelineLayout			= VK_NULL_HANDLE;
    VkPipeline              m_BlurPipeline					= VK_NULL_HANDLE;

	VkTexture				m_IntermediateTexture			= {};

	float					m_Radius						= 0.5f;
	float					m_Bias							= 0.1f;
	float					m_Intensity						= 1.5f;

	bool					m_Blur							= true;

    void                    Create(const RenderContext& rc);
    void                    Destroy();

	void					RecreatePipelines(const RenderContext& rc);
	void					RecreateResolutionDependentResources(const RenderContext& rc);

    void                    Generate(const RenderContext& rc, VkCommandBuffer cmd);

private:
	void					CreatePipelines(const RenderContext& rc);
	void					DestroyPipelines();

	void					CreateResolutionDependentResources(const RenderContext& rc);
	void					DestroyResolutionDependentResources();
};
