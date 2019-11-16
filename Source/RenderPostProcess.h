#pragma once

#include "RenderContext.h"

class RenderPostProcess
{
public:
    VkDescriptorSetLayout   m_TemporalBlendDescriptorSetLayout      = VK_NULL_HANDLE;
    VkPipelineLayout        m_TemporalBlendPipelineLayout           = VK_NULL_HANDLE;
    VkPipeline              m_TemporalBlendPipeline                 = VK_NULL_HANDLE;

    VkDescriptorSetLayout   m_TemporalResolveDescriptorSetLayout    = VK_NULL_HANDLE;
    VkPipelineLayout        m_TemporalResolvePipelineLayout         = VK_NULL_HANDLE;
    VkPipeline              m_TemporalResolvePipeline               = VK_NULL_HANDLE;

    VkDescriptorSetLayout   m_ToneMappingDescriptorSetLayout        = VK_NULL_HANDLE;
    VkPipelineLayout        m_ToneMappingPipelineLayout             = VK_NULL_HANDLE;
    VkPipeline              m_ToneMappingPipeline                   = VK_NULL_HANDLE;

	VkDescriptorSetLayout   m_DebugDescriptorSetLayout				= VK_NULL_HANDLE;
    VkPipelineLayout        m_DebugPipelineLayout					= VK_NULL_HANDLE;
    VkPipeline              m_DebugPipeline							= VK_NULL_HANDLE;

	bool					m_TemporalAAEnable						= true;
	VkTexture               m_TemporalTextures[2]					= {};

    float                   m_Exposure                              = 12.0f;

    void                    Create(const RenderContext& rc);
    void                    Destroy();

	void					RecreatePipelines(const RenderContext& rc);
	void					RecreateResolutionDependentResources(const RenderContext& rc);

    void					Jitter(RenderContext& rc);

    void                    Draw(const RenderContext& rc, VkCommandBuffer cmd);

private:
	void					CreatePipelines(const RenderContext& rc);
	void					DestroyPipelines();

	void					CreateResolutionDependentResources(const RenderContext& rc);
	void					DestroyResolutionDependentResources();
};
