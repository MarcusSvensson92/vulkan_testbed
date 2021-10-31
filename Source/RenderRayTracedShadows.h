#pragma once

#include "RenderContext.h"
#include "AccelerationStructure.h"

class RenderRayTracedShadows
{
public:
	VkDescriptorSetLayout							m_RayTraceDescriptorSetLayouts[2]					= { VK_NULL_HANDLE, VK_NULL_HANDLE };
    VkPipelineLayout								m_RayTracePipelineLayout							= VK_NULL_HANDLE;
    VkPipeline										m_RayTracePipeline									= VK_NULL_HANDLE;

	VkDescriptorSetLayout							m_ReprojectDescriptorSetLayout						= VK_NULL_HANDLE;
    VkPipelineLayout								m_ReprojectPipelineLayout							= VK_NULL_HANDLE;
    VkPipeline										m_ReprojectPipeline									= VK_NULL_HANDLE;

	VkDescriptorSetLayout							m_FilterDescriptorSetLayout							= VK_NULL_HANDLE;
    VkPipelineLayout								m_FilterPipelineLayout								= VK_NULL_HANDLE;
    VkPipeline										m_FilterPipeline									= VK_NULL_HANDLE;

	VkDescriptorSetLayout							m_ResolveDescriptorSetLayout						= VK_NULL_HANDLE;
    VkPipelineLayout								m_ResolvePipelineLayout								= VK_NULL_HANDLE;
    VkPipeline										m_ResolvePipeline									= VK_NULL_HANDLE;

	VkBuffer										m_ShaderBindingTableBuffer							= VK_NULL_HANDLE;
    VmaAllocation									m_ShaderBindingTableBufferAllocation				= VK_NULL_HANDLE;
	VkDeviceAddress									m_ShaderBindingTableBufferDeviceAddress				= 0;
    uint32_t										m_ShaderGroupHandleSize								= 0;
    uint32_t										m_ShaderGroupHandleAlignedSize						= 0;

	VkTexture										m_TemporalTextures[2]								= {};
	VkTexture										m_VarianceTextures[2]								= {};

	bool											m_AlphaTest											= true;
	float											m_ConeAngle											= 0.2f;
	bool											m_Reproject											= true;
	float											m_ReprojectAlphaShadow								= 0.05f;
	float											m_ReprojectAlphaMoments								= 0.2f;
	bool											m_Filter											= true;
	int32_t											m_FilterIterations									= 4;
	float											m_FilterPhiVariance									= 8.0f;

	void											Create(const RenderContext& rc);
    void											Destroy();

	void											RecreatePipelines(const RenderContext& rc);
	void											RecreateResolutionDependentResources(const RenderContext& rc);

    void											RayTrace(const RenderContext& rc, VkCommandBuffer cmd, const AccelerationStructure& as);

private:
	void											CreatePipelines(const RenderContext& rc);
	void											DestroyPipelines();

	void											CreateResolutionDependentResources(const RenderContext& rc);
	void											DestroyResolutionDependentResources();
};
