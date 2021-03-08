#pragma once

#include "RenderContext.h"
#include "AccelerationStructure.h"

class RenderRayTracedAO
{
public:
	VkDescriptorSetLayout							m_RayTraceDescriptorSetLayout						= VK_NULL_HANDLE;
    VkPipelineLayout								m_RayTracePipelineLayout							= VK_NULL_HANDLE;
    VkPipeline										m_RayTracePipeline									= VK_NULL_HANDLE;

	VkDescriptorSetLayout							m_FilterDescriptorSetLayout							= VK_NULL_HANDLE;
    VkPipelineLayout								m_FilterPipelineLayout								= VK_NULL_HANDLE;
    VkPipeline										m_FilterPipeline									= VK_NULL_HANDLE;

	VkBuffer										m_ShaderBindingTableBuffer							= VK_NULL_HANDLE;
    VmaAllocation									m_ShaderBindingTableBufferAllocation				= VK_NULL_HANDLE;
	VkDeviceAddress									m_ShaderBindingTableBufferDeviceAddress				= VK_NULL_HANDLE;
    uint32_t										m_ShaderGroupHandleSize								= 0;
    uint32_t										m_ShaderGroupHandleAlignedSize						= 0;

	VkTexture										m_RawAmbientOcclusionTexture						= {};

	float											m_Radius											= 10.0f;
	float											m_Falloff											= 1.2f;
	bool											m_Filter											= true;
	int32_t											m_FilterIterations									= 4;
	float											m_FilterKernelSigma									= 1.0f;
	float											m_FilterDepthSigma									= 4.0f;

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
