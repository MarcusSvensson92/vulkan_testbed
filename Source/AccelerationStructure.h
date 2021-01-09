#pragma once

#include "RenderContext.h"
#include "GltfModel.h"

struct AccelerationStructureTransparentInstance
{
	uint32_t												MeshIndex;
	uint32_t												TextureIndex;
	uint32_t												IndexOffset;
	uint32_t												VertexOffset;
};

struct AccelerationStructureTopLevel
{
	VkAccelerationStructureKHR								AccelerationStructure					= VK_NULL_HANDLE;
	VkBuffer												Buffer									= VK_NULL_HANDLE;
	VmaAllocation											Allocation								= VK_NULL_HANDLE;
	VkDeviceSize											BuildScratchSize						= 0;
	VkAccelerationStructureBuildGeometryInfoKHR				BuildGeometryInfo						= {};
	VkAccelerationStructureGeometryKHR						Geometry								= {};
};
struct AccelerationStructureBottomLevel
{
	VkAccelerationStructureKHR								AccelerationStructure					= VK_NULL_HANDLE;
	VkBuffer												Buffer									= VK_NULL_HANDLE;
	VmaAllocation											Allocation								= VK_NULL_HANDLE;
	VkDeviceSize											BuildScratchSize						= 0;
	VkAccelerationStructureBuildGeometryInfoKHR				BuildGeometryInfo						= {};
	std::vector<VkAccelerationStructureBuildRangeInfoKHR>	BuildRangeInfos							= {};
	std::vector<VkAccelerationStructureGeometryKHR>			Geometries								= {};
};

class AccelerationStructure
{
public:
	AccelerationStructureTopLevel							m_TopLevel								= {};
	std::vector<AccelerationStructureBottomLevel>			m_BottomLevels							= {};
	
	std::vector<VkDescriptorBufferInfo>						m_IndexBufferInfo						= {};
	std::vector<VkDescriptorBufferInfo>						m_VertexBufferInfo						= {};
	std::vector<VkDescriptorImageInfo>						m_BaseColorImageInfo					= {};

	std::vector<VkAccelerationStructureInstanceKHR>			m_Instances								= {};
	VkBuffer												m_InstanceBuffer						= VK_NULL_HANDLE;
	VmaAllocation											m_InstanceBufferAllocation				= VK_NULL_HANDLE;
	
	VkBuffer												m_ScratchBuffer							= VK_NULL_HANDLE;
	VmaAllocation											m_ScratchBufferAllocation				= VK_NULL_HANDLE;
	
	VkBuffer												m_TransparentInstanceBuffer				= VK_NULL_HANDLE;
	VmaAllocation											m_TransparentInstanceBufferAllocation	= VK_NULL_HANDLE;
	
	void													Create(const RenderContext& rc, uint32_t model_count, const GltfModel* models);
	void													Destroy();

private:
	void													AddBottomLevel(std::vector<VkAccelerationStructureGeometryKHR>&& geometries, std::vector<VkAccelerationStructureBuildRangeInfoKHR>&& build_range_infos, const std::vector<uint32_t>& primitive_counts, const glm::mat4& transform, uint32_t instance_index = 0xffffffffu);
};
