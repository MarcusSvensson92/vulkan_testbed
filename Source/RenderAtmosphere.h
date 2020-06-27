#pragma once

#include "RenderContext.h"
#include "VkTexture.h"

class RenderAtmosphere
{
public:
	VkTexture               m_AmbientLightLUT									= {};
	VkTexture               m_DirectionalLightLUT								= {};
	VkTexture               m_SkyLUTR											= {};
	VkTexture               m_SkyLUTM											= {};

    VkDescriptorSetLayout   m_PrecomputeAmbientLightLUTDescriptorSetLayout      = VK_NULL_HANDLE;
    VkPipelineLayout        m_PrecomputeAmbientLightLUTPipelineLayout           = VK_NULL_HANDLE;
    VkPipeline              m_PrecomputeAmbientLightLUTPipeline                 = VK_NULL_HANDLE;

    VkDescriptorSetLayout   m_PrecomputeDirectionalLightLUTDescriptorSetLayout  = VK_NULL_HANDLE;
    VkPipelineLayout        m_PrecomputeDirectionalLightLUTPipelineLayout       = VK_NULL_HANDLE;
    VkPipeline              m_PrecomputeDirectionalLightLUTPipeline             = VK_NULL_HANDLE;

    VkDescriptorSetLayout   m_PrecomputeSkyLUTDescriptorSetLayout               = VK_NULL_HANDLE;
    VkPipelineLayout        m_PrecomputeSkyLUTPipelineLayout                    = VK_NULL_HANDLE;
    VkPipeline              m_PrecomputeSkyLUTPipeline                          = VK_NULL_HANDLE;

    VkDescriptorSetLayout   m_SkyDescriptorSetLayout                            = VK_NULL_HANDLE;
    VkPipelineLayout        m_SkyPipelineLayout                                 = VK_NULL_HANDLE;
    VkPipeline              m_SkyPipeline                                       = VK_NULL_HANDLE;

	float					m_SkyLightIntensity									= 100.0f;

    void                    Create(const RenderContext& rc);
    void                    Destroy();

	void					RecreatePipelines(const RenderContext& rc);

    void                    DrawSky(const RenderContext& rc, VkCommandBuffer cmd);

private:
	void					CreatePipelines(const RenderContext& rc);
	void					DestroyPipelines();
};
