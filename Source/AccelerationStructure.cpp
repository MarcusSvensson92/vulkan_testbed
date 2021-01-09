#include "AccelerationStructure.h"
#include "VkUtil.h"

void AccelerationStructure::AddBottomLevel(std::vector<VkAccelerationStructureGeometryKHR>&& geometries, std::vector<VkAccelerationStructureBuildRangeInfoKHR>&& build_range_infos, const std::vector<uint32_t>& primitive_counts, const glm::mat4& transform, uint32_t instance_index)
{
	AccelerationStructureBottomLevel acceleration_structure;
	acceleration_structure.Geometries = std::move(geometries);
	acceleration_structure.BuildRangeInfos = std::move(build_range_infos);

	acceleration_structure.BuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	acceleration_structure.BuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	acceleration_structure.BuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	acceleration_structure.BuildGeometryInfo.geometryCount = static_cast<uint32_t>(acceleration_structure.Geometries.size());
	acceleration_structure.BuildGeometryInfo.pGeometries = acceleration_structure.Geometries.data();

	VkAccelerationStructureBuildSizesInfoKHR build_size_info = {};
	build_size_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

	vkGetAccelerationStructureBuildSizesKHR(Vk.Device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &acceleration_structure.BuildGeometryInfo, primitive_counts.data(), &build_size_info);

	acceleration_structure.BuildScratchSize = build_size_info.buildScratchSize;

	VkBufferCreateInfo buffer_create_info = {};
	buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_create_info.size = build_size_info.accelerationStructureSize;
	buffer_create_info.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
	buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo allocation_create_info = {};
	allocation_create_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	VK(vmaCreateBuffer(Vk.Allocator, &buffer_create_info, &allocation_create_info, &acceleration_structure.Buffer, &acceleration_structure.Allocation, NULL));

	VkAccelerationStructureCreateInfoKHR acceleration_structure_info = {};
	acceleration_structure_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	acceleration_structure_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	acceleration_structure_info.buffer = acceleration_structure.Buffer;
	acceleration_structure_info.size = build_size_info.accelerationStructureSize;
	VK(vkCreateAccelerationStructureKHR(Vk.Device, &acceleration_structure_info, NULL, &acceleration_structure.AccelerationStructure));

	acceleration_structure.BuildGeometryInfo.dstAccelerationStructure = acceleration_structure.AccelerationStructure;

	m_BottomLevels.emplace_back(std::move(acceleration_structure));

	VkAccelerationStructureInstanceKHR instance = {};
	instance.transform.matrix[0][0] = transform[0][0]; instance.transform.matrix[0][1] = transform[1][0]; instance.transform.matrix[0][2] = transform[2][0]; instance.transform.matrix[0][3] = transform[3][0];
	instance.transform.matrix[1][0] = transform[0][1]; instance.transform.matrix[1][1] = transform[1][1]; instance.transform.matrix[1][2] = transform[2][1]; instance.transform.matrix[1][3] = transform[3][1];
	instance.transform.matrix[2][0] = transform[0][2]; instance.transform.matrix[2][1] = transform[1][2]; instance.transform.matrix[2][2] = transform[2][2]; instance.transform.matrix[2][3] = transform[3][2];
	instance.instanceCustomIndex = instance_index;
	instance.mask = 0xff;
	instance.instanceShaderBindingTableRecordOffset = 0;
	instance.flags = 0;
	instance.accelerationStructureReference = VkUtilGetDeviceAddress(acceleration_structure.AccelerationStructure);
	m_Instances.emplace_back(instance);
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

			std::vector<VkAccelerationStructureGeometryKHR> opaque_geometries;
			std::vector<VkAccelerationStructureBuildRangeInfoKHR> opaque_build_range_infos;
			std::vector<uint32_t> opaque_primitive_counts;

			for (uint32_t k = instance.MeshOffset; k < (instance.MeshOffset + instance.MeshCount); ++k)
			{
				const GltfMesh& mesh = model.m_Meshes[k];
				const GltfMaterial& material = model.m_Materials[mesh.MaterialIndex];

				VkAccelerationStructureGeometryKHR geometry = {};
				geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
				geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
				geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
				geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
				geometry.geometry.triangles.vertexData.deviceAddress = VkUtilGetDeviceAddress(model.m_VertexBuffer);
				geometry.geometry.triangles.vertexStride = sizeof(glm::vec3);
				geometry.geometry.triangles.maxVertex = mesh.VertexCount;
				geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT16;
				geometry.geometry.triangles.indexData.deviceAddress = VkUtilGetDeviceAddress(model.m_IndexBuffer);
				geometry.flags = material.IsOpaque ? VK_GEOMETRY_OPAQUE_BIT_KHR : 0;

				VkAccelerationStructureBuildRangeInfoKHR build_range_info = {};
				build_range_info.primitiveCount = mesh.IndexCount / 3;
				build_range_info.primitiveOffset = mesh.IndexOffset * sizeof(uint16_t);
				build_range_info.firstVertex = mesh.VertexOffset + static_cast<uint32_t>(model.m_VertexBufferOffsets[VERTEX_ATTRIBUTE_POSITION] / sizeof(glm::vec3));

				if (material.IsOpaque)
				{
					opaque_geometries.emplace_back(geometry);
					opaque_build_range_infos.emplace_back(build_range_info);
					opaque_primitive_counts.emplace_back(build_range_info.primitiveCount);
				}
				else
				{
					if (!material.HasBaseColorTexture)
					{
						continue;
					}

					std::vector<VkAccelerationStructureGeometryKHR> transparent_geometries{ geometry };
					std::vector<VkAccelerationStructureBuildRangeInfoKHR> transparent_build_range_infos{ build_range_info };
					AddBottomLevel(std::move(transparent_geometries), std::move(transparent_build_range_infos), { build_range_info.primitiveCount }, instance.Transform, static_cast<uint32_t>(transparent_instances.size()));
					
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
				AddBottomLevel(std::move(opaque_geometries), std::move(opaque_build_range_infos), opaque_primitive_counts, instance.Transform);
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

	// Geometry instance buffer
	{
		VkBufferCreateInfo buffer_create_info = {};
		buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		buffer_create_info.size = sizeof(VkAccelerationStructureInstanceKHR) * m_Instances.size();
		buffer_create_info.usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocation_create_info = {};
		allocation_create_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
		allocation_create_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		VK(vmaCreateBuffer(Vk.Allocator, &buffer_create_info, &allocation_create_info, &m_InstanceBuffer, &m_InstanceBufferAllocation, NULL));

		VkAllocation buffer_allocation = VkAllocateUploadBuffer(buffer_create_info.size);
		memcpy(buffer_allocation.Data, m_Instances.data(), sizeof(VkAccelerationStructureInstanceKHR)* m_Instances.size());

		VkRecordCommands(
			[=](VkCommandBuffer cmd)
			{
				VkBufferMemoryBarrier pre_transfer_barrier = {};
				pre_transfer_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
				pre_transfer_barrier.srcAccessMask = 0;
				pre_transfer_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				pre_transfer_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				pre_transfer_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				pre_transfer_barrier.buffer = m_InstanceBuffer;
				pre_transfer_barrier.offset = 0;
				pre_transfer_barrier.size = buffer_create_info.size;
				vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 1, &pre_transfer_barrier, 0, NULL);

				VkBufferCopy copy_region;
				copy_region.srcOffset = buffer_allocation.Offset;
				copy_region.dstOffset = 0;
				copy_region.size = buffer_create_info.size;
				vkCmdCopyBuffer(cmd, buffer_allocation.Buffer, m_InstanceBuffer, 1, &copy_region);

				VkBufferMemoryBarrier post_transfer_barrier = {};
				post_transfer_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
				post_transfer_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				post_transfer_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				post_transfer_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				post_transfer_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				post_transfer_barrier.buffer = m_InstanceBuffer;
				post_transfer_barrier.offset = 0;
				post_transfer_barrier.size = buffer_create_info.size;
				vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, NULL, 1, &post_transfer_barrier, 0, NULL);
			});
	}

	// Top level
	{
		m_TopLevel.Geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		m_TopLevel.Geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
		m_TopLevel.Geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
		m_TopLevel.Geometry.geometry.instances.arrayOfPointers = VK_FALSE;
		m_TopLevel.Geometry.geometry.instances.data.deviceAddress = VkUtilGetDeviceAddress(m_InstanceBuffer);

		m_TopLevel.BuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		m_TopLevel.BuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		m_TopLevel.BuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		m_TopLevel.BuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		m_TopLevel.BuildGeometryInfo.geometryCount = 1;
		m_TopLevel.BuildGeometryInfo.pGeometries = &m_TopLevel.Geometry;

		VkAccelerationStructureBuildSizesInfoKHR build_size_info = {};
		build_size_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

		uint32_t count = static_cast<uint32_t>(m_Instances.size());
		vkGetAccelerationStructureBuildSizesKHR(Vk.Device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &m_TopLevel.BuildGeometryInfo, &count, &build_size_info);

		m_TopLevel.BuildScratchSize = build_size_info.buildScratchSize;

		VkBufferCreateInfo buffer_create_info = {};
		buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		buffer_create_info.size = build_size_info.accelerationStructureSize;
		buffer_create_info.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
		buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocation_create_info = {};
		allocation_create_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		VK(vmaCreateBuffer(Vk.Allocator, &buffer_create_info, &allocation_create_info, &m_TopLevel.Buffer, &m_TopLevel.Allocation, NULL));

		VkAccelerationStructureCreateInfoKHR acceleration_structure_info = {};
		acceleration_structure_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		acceleration_structure_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		acceleration_structure_info.buffer = m_TopLevel.Buffer;
		acceleration_structure_info.size = build_size_info.accelerationStructureSize;
		VK(vkCreateAccelerationStructureKHR(Vk.Device, &acceleration_structure_info, NULL, &m_TopLevel.AccelerationStructure));

		m_TopLevel.BuildGeometryInfo.dstAccelerationStructure = m_TopLevel.AccelerationStructure;
	}

	// Scratch buffer
	{
		VkDeviceSize scratch_buffer_size = m_TopLevel.BuildScratchSize;
		for (const AccelerationStructureBottomLevel& acceleration_structure : m_BottomLevels)
		{
			scratch_buffer_size = VkMax(scratch_buffer_size, acceleration_structure.BuildScratchSize);
		}

		VkBufferCreateInfo buffer_create_info = {};
		buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		buffer_create_info.size = scratch_buffer_size;
		buffer_create_info.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
		buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocation_create_info = {};
		allocation_create_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		VK(vmaCreateBuffer(Vk.Allocator, &buffer_create_info, &allocation_create_info, &m_ScratchBuffer, &m_ScratchBufferAllocation, NULL));

		VkDeviceAddress scratch_buffer_address = VkUtilGetDeviceAddress(m_ScratchBuffer);

		m_TopLevel.BuildGeometryInfo.scratchData.deviceAddress = scratch_buffer_address;
		for (AccelerationStructureBottomLevel& acceleration_structure : m_BottomLevels)
		{
			acceleration_structure.BuildGeometryInfo.scratchData.deviceAddress = scratch_buffer_address;
		}
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
			const uint32_t bottom_level_count = static_cast<uint32_t>(m_BottomLevels.size());
			for (uint32_t i = 0; i < bottom_level_count; ++i)
			{
				const AccelerationStructureBottomLevel& acceleration_structure = m_BottomLevels[i];

				const VkAccelerationStructureBuildRangeInfoKHR* build_range_infos = acceleration_structure.BuildRangeInfos.data();
				vkCmdBuildAccelerationStructuresKHR(cmd, 1, &acceleration_structure.BuildGeometryInfo, &build_range_infos);

				VkMemoryBarrier barrier = {};
				barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
				barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
				barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
				vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &barrier, 0, NULL, 0, NULL);
			}

			VkAccelerationStructureBuildRangeInfoKHR build_range_info = {};
			build_range_info.primitiveCount = bottom_level_count;

			const VkAccelerationStructureBuildRangeInfoKHR* build_range_infos = &build_range_info;
			vkCmdBuildAccelerationStructuresKHR(cmd, 1, &m_TopLevel.BuildGeometryInfo, &build_range_infos);

			VkMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
			barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
			barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
			vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &barrier, 0, NULL, 0, NULL);
		});
}

void AccelerationStructure::Destroy()
{
    if (!Vk.IsRayTracingSupported)
    {
        return;
    }

	vmaDestroyBuffer(Vk.Allocator, m_TransparentInstanceBuffer, m_TransparentInstanceBufferAllocation);
	vmaDestroyBuffer(Vk.Allocator, m_InstanceBuffer, m_InstanceBufferAllocation);
	vmaDestroyBuffer(Vk.Allocator, m_ScratchBuffer, m_ScratchBufferAllocation);

	vkDestroyAccelerationStructureKHR(Vk.Device, m_TopLevel.AccelerationStructure, nullptr);
	vmaDestroyBuffer(Vk.Allocator, m_TopLevel.Buffer, m_TopLevel.Allocation);

	for (const AccelerationStructureBottomLevel& acceleration_structure : m_BottomLevels)
	{
		vkDestroyAccelerationStructureKHR(Vk.Device, acceleration_structure.AccelerationStructure, nullptr);
		vmaDestroyBuffer(Vk.Allocator, acceleration_structure.Buffer, acceleration_structure.Allocation);
	}
}
