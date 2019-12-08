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

struct AccelerationStructureTransparentInstance
{
	uint32_t MeshIndex;
	uint32_t TextureIndex;
	uint32_t IndexOffset;
	uint32_t VertexOffset;
};

struct AccelerationStructureTopLevel
{
	VkAccelerationStructureNV						AccelerationStructure					= VK_NULL_HANDLE;
	VmaAllocation									Allocation								= VK_NULL_HANDLE;
	uint64_t										Handle									= VK_NULL_HANDLE;
	VkMemoryRequirements							MemoryRequirements						= {};
};
struct AccelerationStructureBottomLevel
{
	VkAccelerationStructureNV						AccelerationStructure					= VK_NULL_HANDLE;
	VmaAllocation									Allocation								= VK_NULL_HANDLE;
	uint64_t										Handle									= VK_NULL_HANDLE;
	VkMemoryRequirements							MemoryRequirements						= {};
	std::vector<VkGeometryNV>						Geometries								= {};
	glm::mat4										Transform								= glm::identity<glm::mat4>();
	uint32_t										InstanceIndex							= 0xffffffffu;
};

class AccelerationStructure
{
public:
	AccelerationStructureTopLevel					m_TopLevel								= {};
	std::vector<AccelerationStructureBottomLevel>	m_BottomLevels							= {};
	
	std::vector<VkDescriptorBufferInfo>				m_IndexBufferInfo						= {};
	std::vector<VkDescriptorBufferInfo>				m_VertexBufferInfo						= {};
	std::vector<VkDescriptorImageInfo>				m_BaseColorImageInfo					= {};
	
	VkBuffer										m_ScratchBuffer							= VK_NULL_HANDLE;
	VmaAllocation									m_ScratchBufferAllocation				= VK_NULL_HANDLE;
	
	VkBuffer										m_GeometryInstanceBuffer				= VK_NULL_HANDLE;
	VmaAllocation									m_GeometryInstanceBufferAllocation		= VK_NULL_HANDLE;
	
	VkBuffer										m_TransparentInstanceBuffer				= VK_NULL_HANDLE;
	VmaAllocation									m_TransparentInstanceBufferAllocation	= VK_NULL_HANDLE;
	
	void											Create(const RenderContext& rc, uint32_t model_count, const GltfModel* models);
	void											Destroy();
};
