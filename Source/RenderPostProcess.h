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

	bool					m_TemporalAAEnable						= true;
	VkTexture               m_TemporalTextures[2]					= {};

	VkTexture				m_LuxoDoubleChecker						= {};
	bool					m_ViewLuxoDoubleChecker					= false;

    float                   m_Exposure                              = 0.5f;

	float					m_Saturation							= 1.1f;
	float					m_Contrast								= 1.0f;
	float					m_Gamma									= 1.2f;
	float					m_GamutExpansion						= 2.0f;

	enum DisplayMapping
	{
		DISPLAY_MAPPING_ACES_ODT = 0,
		DISPLAY_MAPPING_BT2390_EETF,
		DISPLAY_MAPPING_SDR_EMULATION,
		DISPLAY_MAPPING_LUM_VIS_SCENE,
		DISPLAY_MAPPING_LUM_VIS_DISPLAY,
		DISPLAY_MAPPING_COUNT,
	};
	int32_t					m_DisplayMapping						= DISPLAY_MAPPING_BT2390_EETF;
	int32_t					m_DisplayMappingAux						= DISPLAY_MAPPING_BT2390_EETF;
	bool					m_DisplayMappingSplitScreen				= false;
	float					m_DisplayMappingSplitScreenOffset		= 0.0f;

	float					m_HdrDisplayLuminanceMin				= 0.01f;
	float					m_HdrDisplayLuminanceMax				= 1000.0f;

	float					m_SdrWhiteLevel							= 200.0f;

	float					m_ACESMidPoint							= 25.0f;
	float					m_BT2390MidPoint						= 25.0f;

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
