#include "AccelerationStructure.h"
#include "VkUtil.h"

static void AddBottomLevel(std::vector<AccelerationStructureBottomLevel>& acceleration_structures, const std::vector<VkGeometryNV>& geometries, const glm::mat4& transform, uint32_t instance_index = 0xffffffffu)
{
	AccelerationStructureBottomLevel acceleration_structure;
	acceleration_structure.Geometries = std::move(geometries);
	acceleration_structure.Transform = transform;
	acceleration_structure.InstanceIndex = instance_index;

	VkAccelerationStructureCreateInfoNV acceleration_structure_info = {};
	acceleration_structure_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
	acceleration_structure_info.info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
	acceleration_structure_info.info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
	acceleration_structure_info.info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_NV;
	acceleration_structure_info.info.geometryCount = static_cast<uint32_t>(acceleration_structure.Geometries.size());
	acceleration_structure_info.info.pGeometries = acceleration_structure.Geometries.data();
	VK(vkCreateAccelerationStructureNV(Vk.Device, &acceleration_structure_info, NULL, &acceleration_structure.AccelerationStructure));

	VkAccelerationStructureMemoryRequirementsInfoNV acceleration_structure_memory_requirements_info = {};
	acceleration_structure_memory_requirements_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
	acceleration_structure_memory_requirements_info.accelerationStructure = acceleration_structure.AccelerationStructure;

	VkMemoryRequirements2 acceleration_structure_memory_requirements = {};
	acceleration_structure_memory_requirements.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
	vkGetAccelerationStructureMemoryRequirementsNV(Vk.Device, &acceleration_structure_memory_requirements_info, &acceleration_structure_memory_requirements);
	acceleration_structure.MemoryRequirements = acceleration_structure_memory_requirements.memoryRequirements;

	VmaAllocationCreateInfo acceleration_structure_allocation_create_info = {};
	acceleration_structure_allocation_create_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	VmaAllocationInfo acceleration_structure_allocation_info;
	VK(vmaAllocateMemory(Vk.Allocator, &acceleration_structure.MemoryRequirements, &acceleration_structure_allocation_create_info, &acceleration_structure.Allocation, &acceleration_structure_allocation_info));

	VkBindAccelerationStructureMemoryInfoNV acceleration_structure_memory_info = {};
	acceleration_structure_memory_info.sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
	acceleration_structure_memory_info.accelerationStructure = acceleration_structure.AccelerationStructure;
	acceleration_structure_memory_info.memory = acceleration_structure_allocation_info.deviceMemory;
	acceleration_structure_memory_info.memoryOffset = acceleration_structure_allocation_info.offset;
	VK(vkBindAccelerationStructureMemoryNV(Vk.Device, 1, &acceleration_structure_memory_info));

	VK(vkGetAccelerationStructureHandleNV(Vk.Device, acceleration_structure.AccelerationStructure, sizeof(uint64_t), &acceleration_structure.Handle));

	acceleration_structures.emplace_back(acceleration_structure);
}

void AccelerationStructure::Create(const RenderContext& rc, uint32_t model_count, const GltfModel* models)
{
    if (!Vk.IsRayTracingSupported)
    {
        return;
    }

	std::vector<AccelerationStructureTransparentInstance> transparent_instances;

	// Bottom levels
	for (uint32_t i = 0; i < model_count; ++i)
	{
		const GltfModel& model = models[i];

		std::vector<uint32_t> base_color_textures;
		bool any_transparent = false;

		const uint32_t instance_count = static_cast<uint32_t>(model.m_Instances.size());
		for (uint32_t j = 0; j < instance_count; ++j)
		{
			const GltfInstance& instance = model.m_Instances[j];

			std::vector<VkGeometryNV> opaque_geometries;

			for (uint32_t k = instance.MeshOffset; k < (instance.MeshOffset + instance.MeshCount); ++k)
			{
				const GltfMesh& mesh = model.m_Meshes[k];
				const GltfMaterial& material = model.m_Materials[mesh.MaterialIndex];

				VkGeometryNV geometry = {};
				geometry.sType = VK_STRUCTURE_TYPE_GEOMETRY_NV;
				geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_NV;
				geometry.geometry.aabbs.sType = VK_STRUCTURE_TYPE_GEOMETRY_AABB_NV;
				geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_GEOMETRY_TRIANGLES_NV;
				geometry.geometry.triangles.vertexData = model.m_VertexBuffer;
				geometry.geometry.triangles.vertexOffset = model.m_VertexBufferOffsets[VERTEX_ATTRIBUTE_POSITION] + static_cast<VkDeviceSize>(mesh.VertexOffset) * sizeof(glm::vec3);
				geometry.geometry.triangles.vertexCount = mesh.VertexCount;
				geometry.geometry.triangles.vertexStride = sizeof(glm::vec3);
				geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
				geometry.geometry.triangles.indexData = model.m_IndexBuffer;
				geometry.geometry.triangles.indexOffset = static_cast<VkDeviceSize>(mesh.IndexOffset) * sizeof(uint16_t);
				geometry.geometry.triangles.indexCount = mesh.IndexCount;
				geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT16;
				geometry.flags = material.IsOpaque ? VK_GEOMETRY_OPAQUE_BIT_NV : 0;

				if (material.IsOpaque)
				{
					opaque_geometries.emplace_back(geometry);
				}
				else
				{
					if (!material.HasBaseColorTexture)
					{
						continue;
					}

					AddBottomLevel(m_BottomLevels, { geometry }, instance.Transform, static_cast<uint32_t>(transparent_instances.size()));
					
					uint32_t texture_index = 0;
					const uint32_t base_color_texture_count = static_cast<uint32_t>(base_color_textures.size());
					for (; texture_index < base_color_texture_count; ++texture_index)
					{
						if (base_color_textures[texture_index] == material.BaseColorTextureIndex)
						{
							break;
						}
					}
					if (texture_index == base_color_texture_count)
					{
						base_color_textures.push_back(material.BaseColorTextureIndex);
					}

					AccelerationStructureTransparentInstance transparent_instance;
					transparent_instance.MeshIndex = static_cast<uint32_t>(m_IndexBufferInfo.size());
					transparent_instance.TextureIndex = static_cast<uint32_t>(m_BaseColorImageInfo.size()) + texture_index;
					transparent_instance.IndexOffset = mesh.IndexOffset;
					transparent_instance.VertexOffset = mesh.VertexOffset;
					transparent_instances.emplace_back(transparent_instance);

					any_transparent = true;
				}
			}

			if (!opaque_geometries.empty())
			{
				AddBottomLevel(m_BottomLevels, opaque_geometries, instance.Transform);
			}
		}

		if (any_transparent)
		{
			m_IndexBufferInfo.push_back({ model.m_IndexBuffer, 0, VK_WHOLE_SIZE });
			m_VertexBufferInfo.push_back({ model.m_VertexBuffer, model.m_VertexBufferOffsets[VERTEX_ATTRIBUTE_TEXCOORD], model.m_VertexBufferOffsets[VERTEX_ATTRIBUTE_TEXCOORD + 1] - model.m_VertexBufferOffsets[VERTEX_ATTRIBUTE_TEXCOORD] });

			for (uint32_t base_color_texture_index : base_color_textures)
			{
				VkTexture base_color_texture = model.m_Textures[base_color_texture_index];
				m_BaseColorImageInfo.push_back({ rc.LinearWrap, base_color_texture.ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
			}
		}
	}

	// Top level
	{
		VkAccelerationStructureCreateInfoNV acceleration_structure_info = {};
		acceleration_structure_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
		acceleration_structure_info.info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
		acceleration_structure_info.info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
		acceleration_structure_info.info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_NV;
		acceleration_structure_info.info.instanceCount = static_cast<uint32_t>(m_BottomLevels.size());
		VK(vkCreateAccelerationStructureNV(Vk.Device, &acceleration_structure_info, NULL, &m_TopLevel.AccelerationStructure));

		VkAccelerationStructureMemoryRequirementsInfoNV acceleration_structure_memory_requirements_info = {};
		acceleration_structure_memory_requirements_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
		acceleration_structure_memory_requirements_info.accelerationStructure = m_TopLevel.AccelerationStructure;

		VkMemoryRequirements2 acceleration_structure_memory_requirements = {};
		acceleration_structure_memory_requirements.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
		vkGetAccelerationStructureMemoryRequirementsNV(Vk.Device, &acceleration_structure_memory_requirements_info, &acceleration_structure_memory_requirements);
		m_TopLevel.MemoryRequirements = acceleration_structure_memory_requirements.memoryRequirements;

		VmaAllocationCreateInfo acceleration_structure_allocation_create_info = {};
		acceleration_structure_allocation_create_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;

		VmaAllocationInfo acceleration_structure_allocation_info;
		VK(vmaAllocateMemory(Vk.Allocator, &m_TopLevel.MemoryRequirements, &acceleration_structure_allocation_create_info, &m_TopLevel.Allocation, &acceleration_structure_allocation_info));

		VkBindAccelerationStructureMemoryInfoNV acceleration_structure_memory_info = {};
		acceleration_structure_memory_info.sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
		acceleration_structure_memory_info.accelerationStructure = m_TopLevel.AccelerationStructure;
		acceleration_structure_memory_info.memory = acceleration_structure_allocation_info.deviceMemory;
		acceleration_structure_memory_info.memoryOffset = acceleration_structure_allocation_info.offset;
		VK(vkBindAccelerationStructureMemoryNV(Vk.Device, 1, &acceleration_structure_memory_info));
	}

	// Scratch buffer
	{
		VkDeviceSize scratch_buffer_size = m_TopLevel.MemoryRequirements.size;
		for (const AccelerationStructureBottomLevel& acceleration_structure : m_BottomLevels)
		{
			scratch_buffer_size = VkMax(scratch_buffer_size, acceleration_structure.MemoryRequirements.size);
		}

		VkMemoryRequirements memory_requirements;
		memory_requirements.size = scratch_buffer_size;
		memory_requirements.alignment = m_TopLevel.MemoryRequirements.alignment;
		memory_requirements.memoryTypeBits = m_TopLevel.MemoryRequirements.memoryTypeBits;

		VmaAllocationCreateInfo allocation_create_info = {};
		allocation_create_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		VK(vmaAllocateMemory(Vk.Allocator, &memory_requirements, &allocation_create_info, &m_ScratchBufferAllocation, NULL));

		VkBufferCreateInfo buffer_create_info = {};
		buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		buffer_create_info.size = scratch_buffer_size;
		buffer_create_info.usage = VK_BUFFER_USAGE_RAY_TRACING_BIT_NV;
		buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VK(vkCreateBuffer(Vk.Device, &buffer_create_info, NULL, &m_ScratchBuffer));

		VkMemoryRequirements dummy_memory_requirements;
		vkGetBufferMemoryRequirements(Vk.Device, m_ScratchBuffer, &dummy_memory_requirements);

		VK(vmaBindBufferMemory(Vk.Allocator, m_ScratchBufferAllocation, m_ScratchBuffer));
	}

	// Geometry instance buffer
	{
		VkBufferCreateInfo buffer_create_info = {};
		buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		buffer_create_info.size = sizeof(VkGeometryInstanceNV) * m_BottomLevels.size();
		buffer_create_info.usage = VK_BUFFER_USAGE_RAY_TRACING_BIT_NV | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocation_create_info = {};
		allocation_create_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
		allocation_create_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		VK(vmaCreateBuffer(Vk.Allocator, &buffer_create_info, &allocation_create_info, &m_GeometryInstanceBuffer, &m_GeometryInstanceBufferAllocation, NULL));
		
		VkAllocation buffer_allocation = VkAllocateUploadBuffer(buffer_create_info.size);
		for (size_t i = 0; i < m_BottomLevels.size(); ++i)
		{
			VkGeometryInstanceNV& instance = reinterpret_cast<VkGeometryInstanceNV*>(buffer_allocation.Data)[i];
			instance.transform[0] = m_BottomLevels[i].Transform[0][0]; instance.transform[1] = m_BottomLevels[i].Transform[1][0]; instance.transform[2]  = m_BottomLevels[i].Transform[2][0]; instance.transform[3]  = m_BottomLevels[i].Transform[3][0];
			instance.transform[4] = m_BottomLevels[i].Transform[0][1]; instance.transform[5] = m_BottomLevels[i].Transform[1][1]; instance.transform[6]  = m_BottomLevels[i].Transform[2][1]; instance.transform[7]  = m_BottomLevels[i].Transform[3][1];
			instance.transform[8] = m_BottomLevels[i].Transform[0][2]; instance.transform[9] = m_BottomLevels[i].Transform[1][2]; instance.transform[10] = m_BottomLevels[i].Transform[2][2]; instance.transform[11] = m_BottomLevels[i].Transform[3][2];
			instance.instanceCustomIndex = m_BottomLevels[i].InstanceIndex;
			instance.mask = 0xff;
			instance.instanceOffset = 0;
			instance.flags = 0;
			instance.accelerationStructureHandle = m_BottomLevels[i].Handle;
		}

		VkRecordCommands(
			[=](VkCommandBuffer cmd)
			{
				VkBufferMemoryBarrier pre_transfer_barrier = {};
				pre_transfer_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
				pre_transfer_barrier.srcAccessMask = 0;
				pre_transfer_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				pre_transfer_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				pre_transfer_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				pre_transfer_barrier.buffer = m_GeometryInstanceBuffer;
				pre_transfer_barrier.offset = 0;
				pre_transfer_barrier.size = buffer_create_info.size;
				vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 1, &pre_transfer_barrier, 0, NULL);

				VkBufferCopy copy_region;
				copy_region.srcOffset = buffer_allocation.Offset;
				copy_region.dstOffset = 0;
				copy_region.size = buffer_create_info.size;
				vkCmdCopyBuffer(cmd, buffer_allocation.Buffer, m_GeometryInstanceBuffer, 1, &copy_region);

				VkBufferMemoryBarrier post_transfer_barrier = {};
				post_transfer_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
				post_transfer_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				post_transfer_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				post_transfer_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				post_transfer_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				post_transfer_barrier.buffer = m_GeometryInstanceBuffer;
				post_transfer_barrier.offset = 0;
				post_transfer_barrier.size = buffer_create_info.size;
				vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, NULL, 1, &post_transfer_barrier, 0, NULL);
			});
	}

	// Transparent instance buffer
	{
		VkBufferCreateInfo buffer_create_info = {};
		buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		buffer_create_info.size = sizeof(AccelerationStructureTransparentInstance) * transparent_instances.size();
		buffer_create_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocation_create_info = {};
		allocation_create_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
		allocation_create_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		VK(vmaCreateBuffer(Vk.Allocator, &buffer_create_info, &allocation_create_info, &m_TransparentInstanceBuffer, &m_TransparentInstanceBufferAllocation, NULL));

		VkAllocation buffer_allocation = VkAllocateUploadBuffer(buffer_create_info.size);
		memcpy(buffer_allocation.Data, transparent_instances.data(), buffer_create_info.size);

		VkRecordCommands(
			[=](VkCommandBuffer cmd)
			{
				VkBufferMemoryBarrier pre_transfer_barrier = {};
				pre_transfer_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
				pre_transfer_barrier.srcAccessMask = 0;
				pre_transfer_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				pre_transfer_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				pre_transfer_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				pre_transfer_barrier.buffer = m_TransparentInstanceBuffer;
				pre_transfer_barrier.offset = 0;
				pre_transfer_barrier.size = buffer_create_info.size;
				vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 1, &pre_transfer_barrier, 0, NULL);

				VkBufferCopy copy_region;
				copy_region.srcOffset = buffer_allocation.Offset;
				copy_region.dstOffset = 0;
				copy_region.size = buffer_create_info.size;
				vkCmdCopyBuffer(cmd, buffer_allocation.Buffer, m_TransparentInstanceBuffer, 1, &copy_region);

				VkBufferMemoryBarrier post_transfer_barrier = {};
				post_transfer_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
				post_transfer_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				post_transfer_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				post_transfer_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				post_transfer_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				post_transfer_barrier.buffer = m_TransparentInstanceBuffer;
				post_transfer_barrier.offset = 0;
				post_transfer_barrier.size = buffer_create_info.size;
				vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, NULL, 1, &post_transfer_barrier, 0, NULL);
			});
	}

	VkRecordCommands(
		[=](VkCommandBuffer cmd)
		{
			// Build bottom level acceleration structures
			for (const AccelerationStructureBottomLevel& acceleration_structure : m_BottomLevels)
			{
				VkAccelerationStructureInfoNV acceleration_structure_info = {};
				acceleration_structure_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
				acceleration_structure_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
				acceleration_structure_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_NV;
				acceleration_structure_info.geometryCount = static_cast<uint32_t>(acceleration_structure.Geometries.size());
				acceleration_structure_info.pGeometries = acceleration_structure.Geometries.data();
				vkCmdBuildAccelerationStructureNV(cmd, &acceleration_structure_info, VK_NULL_HANDLE, 0, VK_FALSE, acceleration_structure.AccelerationStructure, VK_NULL_HANDLE, m_ScratchBuffer, 0);

				VkMemoryBarrier barrier = {};
				barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
				barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV;
				barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV;
				vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_NV | VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, 0, 1, &barrier, 0, NULL, 0, NULL);
			}

			// Build top level acceleration structure
			VkAccelerationStructureInfoNV acceleration_structure_info = {};
			acceleration_structure_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
			acceleration_structure_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
			acceleration_structure_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_NV;
			acceleration_structure_info.instanceCount = static_cast<uint32_t>(m_BottomLevels.size());
			vkCmdBuildAccelerationStructureNV(cmd, &acceleration_structure_info, m_GeometryInstanceBuffer, 0, VK_FALSE, m_TopLevel.AccelerationStructure, VK_NULL_HANDLE, m_ScratchBuffer, 0);

			VkMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
			barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV;
			barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV;
			vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_NV | VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, 0, 1, &barrier, 0, NULL, 0, NULL);
		});
}

void AccelerationStructure::Destroy()
{
    if (!Vk.IsRayTracingSupported)
    {
        return;
    }

	vmaDestroyBuffer(Vk.Allocator, m_TransparentInstanceBuffer, m_TransparentInstanceBufferAllocation);

	vmaDestroyBuffer(Vk.Allocator, m_GeometryInstanceBuffer, m_GeometryInstanceBufferAllocation);

	vkDestroyBuffer(Vk.Device, m_ScratchBuffer, NULL);
	vmaFreeMemory(Vk.Allocator, m_ScratchBufferAllocation);

	vkDestroyAccelerationStructureNV(Vk.Device, m_TopLevel.AccelerationStructure, nullptr);
	vmaFreeMemory(Vk.Allocator, m_TopLevel.Allocation);

	for (const AccelerationStructureBottomLevel& acceleration_structure : m_BottomLevels)
	{
		vkDestroyAccelerationStructureNV(Vk.Device, acceleration_structure.AccelerationStructure, nullptr);
		vmaFreeMemory(Vk.Allocator, acceleration_structure.Allocation);
	}
}