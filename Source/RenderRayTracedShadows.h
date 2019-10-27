#pragma once

#include "RenderContext.h"
#include "GltfModel.h"

struct VkGeometryInstanceNV
{
    float		transform[12];
    uint32_t	instanceCustomIndex : 24;
    uint32_t	mask : 8;
    uint32_t	instanceOffset : 24;
    uint32_t	flags : 8;
    uint64_t	accelerationStructureHandle;
};

struct BottomLevelAccelerationStructure
{
	VkAccelerationStructureNV						AccelerationStructure								= VK_NULL_HANDLE;
	VmaAllocation									Allocation											= VK_NULL_HANDLE;
	uint64_t										Handle												= VK_NULL_HANDLE;
	VkMemoryRequirements							MemoryRequirements									= {};
	std::vector<VkGeometryNV>						Geometries											= {};
	glm::mat4										Transform											= glm::identity<glm::mat4>();
	uint32_t										InstanceIndex										= 0xffffffffu;
};

class RenderRayTracedShadows
{
public:
	VkAccelerationStructureNV						m_TopLevelAccelerationStructure						= VK_NULL_HANDLE;
	VmaAllocation									m_TopLevelAccelerationStructureAllocation			= VK_NULL_HANDLE;
	uint64_t										m_TopLevelAccelerationStructureHandle				= VK_NULL_HANDLE;
	VkMemoryRequirements							m_TopLevelAccelerationStructureMemoryRequirements	= {};

	std::vector<BottomLevelAccelerationStructure>	m_BottomLevelAccelerationStructures					= {};

	std::vector<VkDescriptorBufferInfo>				m_IndexBufferInfo									= {};
	std::vector<VkDescriptorBufferInfo>				m_VertexBufferInfo									= {};
	std::vector<VkDescriptorImageInfo>				m_BaseColorImageInfo								= {};
	
    VkBuffer										m_ScratchBuffer										= VK_NULL_HANDLE;
    VmaAllocation									m_ScratchBufferAllocation							= VK_NULL_HANDLE;

    VkBuffer										m_GeometryInstanceBuffer							= VK_NULL_HANDLE;
    VmaAllocation									m_GeometryInstanceBufferAllocation					= VK_NULL_HANDLE;
    VkGeometryInstanceNV*							m_GeometryInstanceBufferMappedData					= NULL;

	VkBuffer										m_TransparentInstanceBuffer							= VK_NULL_HANDLE;
    VmaAllocation									m_TransparentInstanceBufferAllocation				= VK_NULL_HANDLE;

	VkTexture										m_TemporalTextures[2]								= {};
	VkTexture										m_VarianceTextures[2]								= {};

	VkBuffer										m_ShaderBindingTableBuffer							= VK_NULL_HANDLE;
    VmaAllocation									m_ShaderBindingTableBufferAllocation				= VK_NULL_HANDLE;
    uint32_t										m_ShaderGroupHandleSize								= 0;
    uint32_t										m_ShaderGroupBaseAlignment							= 0;

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

	VkDescriptorSetLayout							m_ClearDescriptorSetLayout							= VK_NULL_HANDLE;
    VkPipelineLayout								m_ClearPipelineLayout								= VK_NULL_HANDLE;
    VkPipeline										m_ClearPipeline										= VK_NULL_HANDLE;

	bool											m_Enable											= true;
	bool											m_AlphaTest											= true;
	float											m_ConeAngle											= 0.2f;
	bool											m_Reproject											= true;
	float											m_ReprojectAlphaShadow								= 0.05f;
	float											m_ReprojectAlphaMoments								= 0.2f;
	bool											m_Filter											= true;
	int32_t											m_FilterIterations									= 4;
	float											m_FilterPhiVariance									= 8.0f;

	void											Create(const RenderContext& rc, uint32_t model_count, const GltfModel* models);
    void											Destroy();

	void											RecreatePipelines(const RenderContext& rc);
	void											RecreateResolutionDependentResources(const RenderContext& rc);

    void											RayTrace(const RenderContext& rc, VkCommandBuffer cmd);

private:
	void											CreatePipelines(const RenderContext& rc);
	void											DestroyPipelines();

	void											CreateResolutionDependentResources(const RenderContext& rc);
	void											DestroyResolutionDependentResources();
};
