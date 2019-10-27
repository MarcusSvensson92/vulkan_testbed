#include "RenderRayTracedShadows.h"
#include "VkUtil.h"

#include <algorithm>

static void AddBottomLevelAccelerationStructure(std::vector<BottomLevelAccelerationStructure>& acceleration_structures, const std::vector<VkGeometryNV>& geometries, const glm::mat4& transform, uint32_t instance_index = 0xffffffffu)
{
	BottomLevelAccelerationStructure acceleration_structure;
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

void RenderRayTracedShadows::Create(const RenderContext& rc, uint32_t model_count, const GltfModel* models)
{
    if (!Vk.IsRayTracingSupported)
    {
        return;
    }

	struct TransparentInstance
	{
		uint32_t MeshIndex;
		uint32_t TextureIndex;
		uint32_t IndexOffset;
		uint32_t VertexOffset;
	};
	std::vector<TransparentInstance> transparent_instances;

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

					AddBottomLevelAccelerationStructure(m_BottomLevelAccelerationStructures, { geometry }, instance.Transform, static_cast<uint32_t>(transparent_instances.size()));
					
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

					TransparentInstance transparent_instance;
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
				AddBottomLevelAccelerationStructure(m_BottomLevelAccelerationStructures, opaque_geometries, instance.Transform);
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

	// Top level acceleration structure
	{
		VkAccelerationStructureCreateInfoNV acceleration_structure_info = {};
		acceleration_structure_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
		acceleration_structure_info.info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
		acceleration_structure_info.info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
		acceleration_structure_info.info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_NV;
		acceleration_structure_info.info.instanceCount = static_cast<uint32_t>(m_BottomLevelAccelerationStructures.size());
		VK(vkCreateAccelerationStructureNV(Vk.Device, &acceleration_structure_info, NULL, &m_TopLevelAccelerationStructure));

		VkAccelerationStructureMemoryRequirementsInfoNV acceleration_structure_memory_requirements_info = {};
		acceleration_structure_memory_requirements_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
		acceleration_structure_memory_requirements_info.accelerationStructure = m_TopLevelAccelerationStructure;

		VkMemoryRequirements2 acceleration_structure_memory_requirements = {};
		acceleration_structure_memory_requirements.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
		vkGetAccelerationStructureMemoryRequirementsNV(Vk.Device, &acceleration_structure_memory_requirements_info, &acceleration_structure_memory_requirements);
		m_TopLevelAccelerationStructureMemoryRequirements = acceleration_structure_memory_requirements.memoryRequirements;

		VmaAllocationCreateInfo acceleration_structure_allocation_create_info = {};
		acceleration_structure_allocation_create_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;

		VmaAllocationInfo acceleration_structure_allocation_info;
		VK(vmaAllocateMemory(Vk.Allocator, &m_TopLevelAccelerationStructureMemoryRequirements, &acceleration_structure_allocation_create_info, &m_TopLevelAccelerationStructureAllocation, &acceleration_structure_allocation_info));

		VkBindAccelerationStructureMemoryInfoNV acceleration_structure_memory_info = {};
		acceleration_structure_memory_info.sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
		acceleration_structure_memory_info.accelerationStructure = m_TopLevelAccelerationStructure;
		acceleration_structure_memory_info.memory = acceleration_structure_allocation_info.deviceMemory;
		acceleration_structure_memory_info.memoryOffset = acceleration_structure_allocation_info.offset;
		VK(vkBindAccelerationStructureMemoryNV(Vk.Device, 1, &acceleration_structure_memory_info));
	}

	// Scratch buffer
	{
		VkDeviceSize scratch_buffer_size = m_TopLevelAccelerationStructureMemoryRequirements.size;
		for (const BottomLevelAccelerationStructure& acceleration_structure : m_BottomLevelAccelerationStructures)
		{
			scratch_buffer_size = std::max(scratch_buffer_size, acceleration_structure.MemoryRequirements.size);
		}

		VkMemoryRequirements memory_requirements;
		memory_requirements.size = scratch_buffer_size;
		memory_requirements.alignment = m_TopLevelAccelerationStructureMemoryRequirements.alignment;
		memory_requirements.memoryTypeBits = m_TopLevelAccelerationStructureMemoryRequirements.memoryTypeBits;

		VmaAllocationCreateInfo allocation_create_info = {};
		allocation_create_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		VK(vmaAllocateMemory(Vk.Allocator, &memory_requirements, &allocation_create_info, &m_ScratchBufferAllocation, NULL));

		VkBufferCreateInfo buffer_create_info = {};
		buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		buffer_create_info.size = scratch_buffer_size;
		buffer_create_info.usage = VK_BUFFER_USAGE_RAY_TRACING_BIT_NV;
		buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VK(vkCreateBuffer(Vk.Device, &buffer_create_info, NULL, &m_ScratchBuffer));
		vkGetBufferMemoryRequirements(Vk.Device, m_ScratchBuffer, &VkMemoryRequirements());
		VK(vmaBindBufferMemory(Vk.Allocator, m_ScratchBufferAllocation, m_ScratchBuffer));
	}

	// Geometry instance buffer
	{
		VkBufferCreateInfo buffer_create_info = {};
		buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		buffer_create_info.size = sizeof(VkGeometryInstanceNV) * m_BottomLevelAccelerationStructures.size();
		buffer_create_info.usage = VK_BUFFER_USAGE_RAY_TRACING_BIT_NV;
		buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocation_create_info = {};
		allocation_create_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
		allocation_create_info.usage = VMA_MEMORY_USAGE_CPU_ONLY;

		VmaAllocationInfo allocation_info;
		VK(vmaCreateBuffer(Vk.Allocator, &buffer_create_info, &allocation_create_info, &m_GeometryInstanceBuffer, &m_GeometryInstanceBufferAllocation, &allocation_info));
		m_GeometryInstanceBufferMappedData = static_cast<VkGeometryInstanceNV*>(allocation_info.pMappedData);

		for (size_t i = 0; i < m_BottomLevelAccelerationStructures.size(); ++i)
		{
			VkGeometryInstanceNV& instance = m_GeometryInstanceBufferMappedData[i];
			instance.transform[0] = m_BottomLevelAccelerationStructures[i].Transform[0][0]; instance.transform[1] = m_BottomLevelAccelerationStructures[i].Transform[1][0]; instance.transform[2]  = m_BottomLevelAccelerationStructures[i].Transform[2][0]; instance.transform[3]  = m_BottomLevelAccelerationStructures[i].Transform[3][0];
			instance.transform[4] = m_BottomLevelAccelerationStructures[i].Transform[0][1]; instance.transform[5] = m_BottomLevelAccelerationStructures[i].Transform[1][1]; instance.transform[6]  = m_BottomLevelAccelerationStructures[i].Transform[2][1]; instance.transform[7]  = m_BottomLevelAccelerationStructures[i].Transform[3][1];
			instance.transform[8] = m_BottomLevelAccelerationStructures[i].Transform[0][2]; instance.transform[9] = m_BottomLevelAccelerationStructures[i].Transform[1][2]; instance.transform[10] = m_BottomLevelAccelerationStructures[i].Transform[2][2]; instance.transform[12] = m_BottomLevelAccelerationStructures[i].Transform[3][2];
			instance.instanceCustomIndex = m_BottomLevelAccelerationStructures[i].InstanceIndex;
			instance.mask = 0xff;
			instance.instanceOffset = 0;
			instance.flags = 0;
			instance.accelerationStructureHandle = m_BottomLevelAccelerationStructures[i].Handle;
		}
	}

	// Transparent instance buffer
	{
		VkBufferCreateInfo buffer_create_info = {};
		buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		buffer_create_info.size = sizeof(TransparentInstance) * transparent_instances.size();
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
			for (const BottomLevelAccelerationStructure& acceleration_structure : m_BottomLevelAccelerationStructures)
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
			acceleration_structure_info.instanceCount = static_cast<uint32_t>(m_BottomLevelAccelerationStructures.size());
			vkCmdBuildAccelerationStructureNV(cmd, &acceleration_structure_info, m_GeometryInstanceBuffer, 0, VK_FALSE, m_TopLevelAccelerationStructure, VK_NULL_HANDLE, m_ScratchBuffer, 0);

			VkMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
			barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV;
			barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV;
			vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_NV | VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, 0, 1, &barrier, 0, NULL, 0, NULL);
		});

	CreateResolutionDependentResources(rc);

	// Ray Trace
	{
		VkDescriptorSetLayoutBinding set_layout_bindings_0[] =
		{
			{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_RAYGEN_BIT_NV, NULL },
			{ 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_RAYGEN_BIT_NV, NULL },
			{ 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_RAYGEN_BIT_NV, NULL },
			{ 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_RAYGEN_BIT_NV, NULL },
			{ 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_RAYGEN_BIT_NV, NULL },
			{ 5, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV, 1, VK_SHADER_STAGE_RAYGEN_BIT_NV, NULL },
		};
		VkDescriptorSetLayoutCreateInfo set_layout_info_0 = {};
		set_layout_info_0.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		set_layout_info_0.bindingCount = static_cast<uint32_t>(sizeof(set_layout_bindings_0) / sizeof(*set_layout_bindings_0));
		set_layout_info_0.pBindings = set_layout_bindings_0;
		VK(vkCreateDescriptorSetLayout(Vk.Device, &set_layout_info_0, NULL, &m_RayTraceDescriptorSetLayouts[0]));

		VkDescriptorSetLayoutBinding set_layout_bindings_1[] =
		{
			{ 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_ANY_HIT_BIT_NV, NULL },
			{ 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, static_cast<uint32_t>(m_IndexBufferInfo.size()), VK_SHADER_STAGE_ANY_HIT_BIT_NV, NULL },
			{ 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, static_cast<uint32_t>(m_VertexBufferInfo.size()), VK_SHADER_STAGE_ANY_HIT_BIT_NV, NULL },
			{ 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(m_BaseColorImageInfo.size()), VK_SHADER_STAGE_ANY_HIT_BIT_NV, NULL },
		};
		VkDescriptorSetLayoutCreateInfo set_layout_info_1 = {};
		set_layout_info_1.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		set_layout_info_1.bindingCount = static_cast<uint32_t>(sizeof(set_layout_bindings_1) / sizeof(*set_layout_bindings_1));
		set_layout_info_1.pBindings = set_layout_bindings_1;
		VK(vkCreateDescriptorSetLayout(Vk.Device, &set_layout_info_1, NULL, &m_RayTraceDescriptorSetLayouts[1]));

		VkPipelineLayoutCreateInfo pipeline_layout_info = {};
		pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_info.setLayoutCount = sizeof(m_RayTraceDescriptorSetLayouts) / sizeof(*m_RayTraceDescriptorSetLayouts);
		pipeline_layout_info.pSetLayouts = m_RayTraceDescriptorSetLayouts;
		VK(vkCreatePipelineLayout(Vk.Device, &pipeline_layout_info, NULL, &m_RayTracePipelineLayout));
	}

	// Reproject
	{
		VkDescriptorSetLayoutBinding set_layout_bindings[] =
		{
			{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL },
			{ 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL },
			{ 2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL },
			{ 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL },
			{ 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL },
			{ 5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL },
			{ 6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL },
			{ 7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL },
		};
		VkDescriptorSetLayoutCreateInfo set_layout_info = {};
		set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		set_layout_info.bindingCount = static_cast<uint32_t>(sizeof(set_layout_bindings) / sizeof(*set_layout_bindings));
		set_layout_info.pBindings = set_layout_bindings;
		VK(vkCreateDescriptorSetLayout(Vk.Device, &set_layout_info, NULL, &m_ReprojectDescriptorSetLayout));

		VkPipelineLayoutCreateInfo pipeline_layout_info = {};
		pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_info.setLayoutCount = 1;
		pipeline_layout_info.pSetLayouts = &m_ReprojectDescriptorSetLayout;
		VK(vkCreatePipelineLayout(Vk.Device, &pipeline_layout_info, NULL, &m_ReprojectPipelineLayout));
	}

	// Filter
	{
		VkDescriptorSetLayoutBinding set_layout_bindings[] =
		{
			{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL },
			{ 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL },
			{ 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL },
			{ 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL },
		};
		VkDescriptorSetLayoutCreateInfo set_layout_info = {};
		set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		set_layout_info.bindingCount = static_cast<uint32_t>(sizeof(set_layout_bindings) / sizeof(*set_layout_bindings));
		set_layout_info.pBindings = set_layout_bindings;
		VK(vkCreateDescriptorSetLayout(Vk.Device, &set_layout_info, NULL, &m_FilterDescriptorSetLayout));

		VkPipelineLayoutCreateInfo pipeline_layout_info = {};
		pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_info.setLayoutCount = 1;
		pipeline_layout_info.pSetLayouts = &m_FilterDescriptorSetLayout;
		VK(vkCreatePipelineLayout(Vk.Device, &pipeline_layout_info, NULL, &m_FilterPipelineLayout));
	}

	// Resolve
	{
		VkDescriptorSetLayoutBinding set_layout_bindings[] =
		{
			{ 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL },
			{ 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL },
		};
		VkDescriptorSetLayoutCreateInfo set_layout_info = {};
		set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		set_layout_info.bindingCount = static_cast<uint32_t>(sizeof(set_layout_bindings) / sizeof(*set_layout_bindings));
		set_layout_info.pBindings = set_layout_bindings;
		VK(vkCreateDescriptorSetLayout(Vk.Device, &set_layout_info, NULL, &m_ResolveDescriptorSetLayout));

		VkPipelineLayoutCreateInfo pipeline_layout_info = {};
		pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_info.setLayoutCount = 1;
		pipeline_layout_info.pSetLayouts = &m_ResolveDescriptorSetLayout;
		VK(vkCreatePipelineLayout(Vk.Device, &pipeline_layout_info, NULL, &m_ResolvePipelineLayout));
	}

	// Clear
	{
		VkDescriptorSetLayoutBinding set_layout_bindings[] =
		{
			{ 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL },
		};
		VkDescriptorSetLayoutCreateInfo set_layout_info = {};
		set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		set_layout_info.bindingCount = static_cast<uint32_t>(sizeof(set_layout_bindings) / sizeof(*set_layout_bindings));
		set_layout_info.pBindings = set_layout_bindings;
		VK(vkCreateDescriptorSetLayout(Vk.Device, &set_layout_info, NULL, &m_ClearDescriptorSetLayout));

		VkPipelineLayoutCreateInfo pipeline_layout_info = {};
		pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_info.setLayoutCount = 1;
		pipeline_layout_info.pSetLayouts = &m_ClearDescriptorSetLayout;
		VK(vkCreatePipelineLayout(Vk.Device, &pipeline_layout_info, NULL, &m_ClearPipelineLayout));
	}

	VkPhysicalDeviceRayTracingPropertiesNV ray_tracing_properties = {};
	ray_tracing_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_NV;

	VkPhysicalDeviceProperties2KHR physical_device_properties = {};
	physical_device_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;
	physical_device_properties.pNext = &ray_tracing_properties;
	vkGetPhysicalDeviceProperties2(Vk.PhysicalDevice, &physical_device_properties);

	m_ShaderGroupHandleSize = ray_tracing_properties.shaderGroupHandleSize;
	m_ShaderGroupBaseAlignment = ray_tracing_properties.shaderGroupBaseAlignment;

	VkBufferCreateInfo buffer_create_info = {};
	buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_create_info.size = static_cast<VkDeviceSize>(VkAlignUp(m_ShaderGroupHandleSize, m_ShaderGroupBaseAlignment)) * 3;
	buffer_create_info.usage = VK_BUFFER_USAGE_RAY_TRACING_BIT_NV | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo buffer_allocation_create_info = {};
	buffer_allocation_create_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	VK(vmaCreateBuffer(Vk.Allocator, &buffer_create_info, &buffer_allocation_create_info, &m_ShaderBindingTableBuffer, &m_ShaderBindingTableBufferAllocation, NULL));

	CreatePipelines(rc);
}

void RenderRayTracedShadows::Destroy()
{
    if (!Vk.IsRayTracingSupported)
    {
        return;
    }

	for (const BottomLevelAccelerationStructure& acceleration_structure : m_BottomLevelAccelerationStructures)
	{
		vkDestroyAccelerationStructureNV(Vk.Device, acceleration_structure.AccelerationStructure, nullptr);
		vmaFreeMemory(Vk.Allocator, acceleration_structure.Allocation);
	}

	vkDestroyAccelerationStructureNV(Vk.Device, m_TopLevelAccelerationStructure, nullptr);
	vmaFreeMemory(Vk.Allocator, m_TopLevelAccelerationStructureAllocation);

	vkDestroyBuffer(Vk.Device, m_TransparentInstanceBuffer, NULL);
	vmaFreeMemory(Vk.Allocator, m_TransparentInstanceBufferAllocation);

    vkDestroyBuffer(Vk.Device, m_ScratchBuffer, NULL);
    vmaFreeMemory(Vk.Allocator, m_ScratchBufferAllocation);

	vmaDestroyBuffer(Vk.Allocator, m_GeometryInstanceBuffer, m_GeometryInstanceBufferAllocation);

	DestroyResolutionDependentResources();

	vmaDestroyBuffer(Vk.Allocator, m_ShaderBindingTableBuffer, m_ShaderBindingTableBufferAllocation);

	DestroyPipelines();

	vkDestroyPipelineLayout(Vk.Device, m_RayTracePipelineLayout, NULL);
	vkDestroyDescriptorSetLayout(Vk.Device, m_RayTraceDescriptorSetLayouts[0], NULL);
	vkDestroyDescriptorSetLayout(Vk.Device, m_RayTraceDescriptorSetLayouts[1], NULL);

	vkDestroyPipelineLayout(Vk.Device, m_ReprojectPipelineLayout, NULL);
	vkDestroyDescriptorSetLayout(Vk.Device, m_ReprojectDescriptorSetLayout, NULL);

	vkDestroyPipelineLayout(Vk.Device, m_FilterPipelineLayout, NULL);
	vkDestroyDescriptorSetLayout(Vk.Device, m_FilterDescriptorSetLayout, NULL);

	vkDestroyPipelineLayout(Vk.Device, m_ResolvePipelineLayout, NULL);
	vkDestroyDescriptorSetLayout(Vk.Device, m_ResolveDescriptorSetLayout, NULL);

	vkDestroyPipelineLayout(Vk.Device, m_ClearPipelineLayout, NULL);
	vkDestroyDescriptorSetLayout(Vk.Device, m_ClearDescriptorSetLayout, NULL);
}

void RenderRayTracedShadows::CreatePipelines(const RenderContext& rc)
{
	// Ray Trace
	{
		VkShaderModule rgen_shader = VkUtilLoadShaderModule("../Assets/Shaders/Shadows.rgen");
		VkShaderModule rmiss_shader = VkUtilLoadShaderModule("../Assets/Shaders/Shadows.rmiss");
		VkShaderModule rahit_shader = VkUtilLoadShaderModule("../Assets/Shaders/Shadows.rahit");

		VkPipelineShaderStageCreateInfo rgen_shader_stage = {};
		rgen_shader_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		rgen_shader_stage.stage = VK_SHADER_STAGE_RAYGEN_BIT_NV;
		rgen_shader_stage.module = rgen_shader;
		rgen_shader_stage.pName = "main";
		VkPipelineShaderStageCreateInfo rmiss_shader_stage = {};
		rmiss_shader_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		rmiss_shader_stage.stage = VK_SHADER_STAGE_MISS_BIT_NV;
		rmiss_shader_stage.module = rmiss_shader;
		rmiss_shader_stage.pName = "main";
		VkPipelineShaderStageCreateInfo rahit_shader_stage = {};
		rahit_shader_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		rahit_shader_stage.stage = VK_SHADER_STAGE_ANY_HIT_BIT_NV;
		rahit_shader_stage.module = rahit_shader;
		rahit_shader_stage.pName = "main";
		const VkPipelineShaderStageCreateInfo shader_stages[] =
		{
			rgen_shader_stage,
			rmiss_shader_stage,
			rahit_shader_stage,
		};

		VkRayTracingShaderGroupCreateInfoNV rgen_shader_group = {};
		rgen_shader_group.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_NV;
		rgen_shader_group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV;
		rgen_shader_group.generalShader = 0;
		rgen_shader_group.closestHitShader = VK_SHADER_UNUSED_NV;
		rgen_shader_group.anyHitShader = VK_SHADER_UNUSED_NV;
		rgen_shader_group.intersectionShader = VK_SHADER_UNUSED_NV;
		VkRayTracingShaderGroupCreateInfoNV rmiss_shader_group = {};
		rmiss_shader_group.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_NV;
		rmiss_shader_group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV;
		rmiss_shader_group.generalShader = 1;
		rmiss_shader_group.closestHitShader = VK_SHADER_UNUSED_NV;
		rmiss_shader_group.anyHitShader = VK_SHADER_UNUSED_NV;
		rmiss_shader_group.intersectionShader = VK_SHADER_UNUSED_NV;
		VkRayTracingShaderGroupCreateInfoNV rahit_shader_group = {};
		rahit_shader_group.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_NV;
		rahit_shader_group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV;
		rahit_shader_group.generalShader = VK_SHADER_UNUSED_NV;
		rahit_shader_group.closestHitShader = VK_SHADER_UNUSED_NV;
		rahit_shader_group.anyHitShader = 2;
		rahit_shader_group.intersectionShader = VK_SHADER_UNUSED_NV;
		const VkRayTracingShaderGroupCreateInfoNV shader_groups[] =
		{
			rgen_shader_group,
			rmiss_shader_group,
			rahit_shader_group,
		};

		VkRayTracingPipelineCreateInfoNV pipeline_info = {};
		pipeline_info.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_NV;
		pipeline_info.layout = m_RayTracePipelineLayout;
		pipeline_info.stageCount = static_cast<uint32_t>(sizeof(shader_stages) / sizeof(VkPipelineShaderStageCreateInfo));
		pipeline_info.pStages = shader_stages;
		pipeline_info.groupCount = static_cast<uint32_t>(sizeof(shader_groups) / sizeof(VkRayTracingShaderGroupCreateInfoNV));
		pipeline_info.pGroups = shader_groups;
		pipeline_info.maxRecursionDepth = 1;
		VK(vkCreateRayTracingPipelinesNV(Vk.Device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &m_RayTracePipeline));

		vkDestroyShaderModule(Vk.Device, rgen_shader, NULL);
		vkDestroyShaderModule(Vk.Device, rmiss_shader, NULL);
		vkDestroyShaderModule(Vk.Device, rahit_shader, NULL);

		VkDeviceSize buffer_size = static_cast<VkDeviceSize>(VkAlignUp(m_ShaderGroupHandleSize, m_ShaderGroupBaseAlignment)) * 3;
		VkAllocation buffer_allocation = VkAllocateUploadBuffer(buffer_size);

		vkGetRayTracingShaderGroupHandlesNV(Vk.Device, m_RayTracePipeline, 0, 1, m_ShaderGroupHandleSize, buffer_allocation.Data + 0 * VkAlignUp(m_ShaderGroupHandleSize, m_ShaderGroupBaseAlignment));
		vkGetRayTracingShaderGroupHandlesNV(Vk.Device, m_RayTracePipeline, 1, 1, m_ShaderGroupHandleSize, buffer_allocation.Data + 1 * VkAlignUp(m_ShaderGroupHandleSize, m_ShaderGroupBaseAlignment));
		vkGetRayTracingShaderGroupHandlesNV(Vk.Device, m_RayTracePipeline, 2, 1, m_ShaderGroupHandleSize, buffer_allocation.Data + 2 * VkAlignUp(m_ShaderGroupHandleSize, m_ShaderGroupBaseAlignment));

		VkRecordCommands(
			[=](VkCommandBuffer cmd)
			{
				VkBufferMemoryBarrier pre_transfer_barrier = {};
				pre_transfer_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
				pre_transfer_barrier.srcAccessMask = 0;
				pre_transfer_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				pre_transfer_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				pre_transfer_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				pre_transfer_barrier.buffer = m_ShaderBindingTableBuffer;
				pre_transfer_barrier.offset = 0;
				pre_transfer_barrier.size = buffer_size;
				vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 1, &pre_transfer_barrier, 0, NULL);

				VkBufferCopy buffer_copy_region;
				buffer_copy_region.srcOffset = buffer_allocation.Offset;
				buffer_copy_region.dstOffset = 0;
				buffer_copy_region.size = buffer_size;
				vkCmdCopyBuffer(cmd, buffer_allocation.Buffer, m_ShaderBindingTableBuffer, 1, &buffer_copy_region);

				VkBufferMemoryBarrier post_transfer_barrier = {};
				post_transfer_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
				post_transfer_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				post_transfer_barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
				post_transfer_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				post_transfer_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				post_transfer_barrier.buffer = m_ShaderBindingTableBuffer;
				post_transfer_barrier.offset = 0;
				post_transfer_barrier.size = buffer_size;
				vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_NV, 0, 0, NULL, 1, &post_transfer_barrier, 0, NULL);
			});
	}

	// Reproject
	{
		VkUtilCreateComputePipelineParams pipeline_params;
		pipeline_params.PipelineLayout = m_ReprojectPipelineLayout;
		pipeline_params.ComputeShaderFilepath = "../Assets/Shaders/ShadowsReproject.comp";
		m_ReprojectPipeline = VkUtilCreateComputePipeline(pipeline_params);
	}

	// Filter
	{
		VkUtilCreateComputePipelineParams pipeline_params;
		pipeline_params.PipelineLayout = m_FilterPipelineLayout;
		pipeline_params.ComputeShaderFilepath = "../Assets/Shaders/ShadowsFilter.comp";
		m_FilterPipeline = VkUtilCreateComputePipeline(pipeline_params);
	}

	// Resolve
	{
		VkUtilCreateComputePipelineParams pipeline_params;
		pipeline_params.PipelineLayout = m_ResolvePipelineLayout;
		pipeline_params.ComputeShaderFilepath = "../Assets/Shaders/ShadowsResolve.comp";
		m_ResolvePipeline = VkUtilCreateComputePipeline(pipeline_params);
	}

	// Clear
	{
		VkUtilCreateComputePipelineParams pipeline_params;
		pipeline_params.PipelineLayout = m_ClearPipelineLayout;
		pipeline_params.ComputeShaderFilepath = "../Assets/Shaders/ShadowsClear.comp";
		m_ClearPipeline = VkUtilCreateComputePipeline(pipeline_params);
	}
}

void RenderRayTracedShadows::DestroyPipelines()
{
	vkDestroyPipeline(Vk.Device, m_RayTracePipeline, NULL);
	vkDestroyPipeline(Vk.Device, m_ReprojectPipeline, NULL);
	vkDestroyPipeline(Vk.Device, m_FilterPipeline, NULL);
	vkDestroyPipeline(Vk.Device, m_ResolvePipeline, NULL);
	vkDestroyPipeline(Vk.Device, m_ClearPipeline, NULL);
}

void RenderRayTracedShadows::CreateResolutionDependentResources(const RenderContext& rc)
{
	VkTextureCreateParams temporal_texture_params;
	temporal_texture_params.Type = VK_IMAGE_TYPE_2D;
	temporal_texture_params.ViewType = VK_IMAGE_VIEW_TYPE_2D;
	temporal_texture_params.Width = rc.Width;
	temporal_texture_params.Height = rc.Height;
	temporal_texture_params.Format = VK_FORMAT_R16G16B16A16_UNORM;
	temporal_texture_params.Usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	temporal_texture_params.InitialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	m_TemporalTextures[0] = VkTextureCreate(temporal_texture_params);
	m_TemporalTextures[1] = VkTextureCreate(temporal_texture_params);

	VkTextureCreateParams variance_texture_params;
	variance_texture_params.Type = VK_IMAGE_TYPE_2D;
	variance_texture_params.ViewType = VK_IMAGE_VIEW_TYPE_2D;
	variance_texture_params.Width = rc.Width;
	variance_texture_params.Height = rc.Height;
	variance_texture_params.Format = VK_FORMAT_R16G16_UNORM;
	variance_texture_params.Usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	variance_texture_params.InitialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	m_VarianceTextures[0] = VkTextureCreate(variance_texture_params);
	m_VarianceTextures[1] = VkTextureCreate(variance_texture_params);
}

void RenderRayTracedShadows::DestroyResolutionDependentResources()
{
	VkTextureDestroy(m_TemporalTextures[0]);
	VkTextureDestroy(m_TemporalTextures[1]);
	VkTextureDestroy(m_VarianceTextures[0]);
	VkTextureDestroy(m_VarianceTextures[1]);
}

void RenderRayTracedShadows::RecreatePipelines(const RenderContext& rc)
{
	if (!Vk.IsRayTracingSupported)
	{
		return;
	}

	DestroyPipelines();
	CreatePipelines(rc);
}

void RenderRayTracedShadows::RecreateResolutionDependentResources(const RenderContext& rc)
{
	if (!Vk.IsRayTracingSupported)
	{
		return;
	}

	DestroyResolutionDependentResources();
	CreateResolutionDependentResources(rc);
}

void RenderRayTracedShadows::RayTrace(const RenderContext& rc, VkCommandBuffer cmd)
{
	if (!Vk.IsRayTracingSupported)
	{
		return;
	}

    if (!m_Enable)
    {
		VkUtilImageBarrier(cmd, rc.ShadowTexture.Image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);

		// Clear
		{
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_ClearPipeline);

			VkDescriptorSet set = VkCreateDescriptorSetForCurrentFrame(m_ClearDescriptorSetLayout,
				{
					{ 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0, rc.ShadowTexture.ImageView, VK_IMAGE_LAYOUT_GENERAL, VK_NULL_HANDLE },
				});
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_ClearPipelineLayout, 0, 1, &set, 0, NULL);

			vkCmdDispatch(cmd, (rc.Width + 7) / 8, (rc.Height + 7) / 8, 1);
		}

		VkUtilImageBarrier(cmd, rc.ShadowTexture.Image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);

        return;
    }

	VkUtilImageBarrier(cmd, rc.DepthTexture.Image, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);
	VkUtilImageBarrier(cmd, rc.ShadowTexture.Image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);

	{
		VkPushLabel(cmd, "Shadows Trace Rays");

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, m_RayTracePipeline);

		glm::mat4 view = rc.CameraCurr.m_View;
		view[3][0] = view[3][1] = view[3][2] = 0.0f; // Set translation to zero

		struct Constants
		{
			glm::mat4	InvViewProjZeroTranslation;
			glm::vec3	ViewPosition;
			float		CosAngle;
			glm::vec3	LightDirection;
			uint32_t	AlphaTest;
		};
		VkAllocation constants_allocation = VkAllocateUploadBuffer(sizeof(Constants));
		Constants* constants = reinterpret_cast<Constants*>(constants_allocation.Data);
		constants->InvViewProjZeroTranslation = glm::inverse(rc.CameraCurr.m_Projection * view);
		constants->ViewPosition = rc.CameraCurr.m_Position;
		constants->CosAngle = std::cos(glm::radians(m_ConeAngle) * 0.5f);
		constants->LightDirection = glm::normalize(rc.SunDirection);
		constants->AlphaTest = m_AlphaTest;

		VkDescriptorSet sets[] =
		{
			VkCreateDescriptorSetForCurrentFrame(m_RayTraceDescriptorSetLayouts[0],
			{
				{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, constants_allocation.Buffer, constants_allocation.Offset, sizeof(Constants) },
				{ 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0, rc.ShadowTexture.ImageView, VK_IMAGE_LAYOUT_GENERAL, VK_NULL_HANDLE },
				{ 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, rc.DepthTexture.ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rc.NearestClamp },
				{ 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, rc.NormalTexture.ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rc.NearestClamp },
				{ 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, rc.BlueNoiseTextures[m_Reproject ? (rc.FrameCounter % static_cast<uint32_t>(rc.BlueNoiseTextures.size())) : 0].ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rc.NearestWrap },
				{ 5, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV, 0, m_TopLevelAccelerationStructure },
			}),
			VkCreateDescriptorSetForCurrentFrame(m_RayTraceDescriptorSetLayouts[1],
			{
				{ 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, m_TransparentInstanceBuffer },
				{ 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, static_cast<uint32_t>(m_IndexBufferInfo.size()), m_IndexBufferInfo.data() },
				{ 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, static_cast<uint32_t>(m_VertexBufferInfo.size()), m_VertexBufferInfo.data() },
				{ 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, static_cast<uint32_t>(m_BaseColorImageInfo.size()), m_BaseColorImageInfo.data() },
			})
		};
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, m_RayTracePipelineLayout, 0, sizeof(sets) / sizeof(*sets), sets, 0, NULL);

		vkCmdTraceRaysNV(cmd, m_ShaderBindingTableBuffer, 0, m_ShaderBindingTableBuffer, VkAlignUp(m_ShaderGroupHandleSize, m_ShaderGroupBaseAlignment), m_ShaderGroupHandleSize, m_ShaderBindingTableBuffer, 2 * VkAlignUp(m_ShaderGroupHandleSize, m_ShaderGroupBaseAlignment), m_ShaderGroupHandleSize, VK_NULL_HANDLE, 0, 0, rc.Width, rc.Height, 1);

		VkPopLabel(cmd);
	}

	VkUtilImageBarrier(cmd, rc.DepthTexture.Image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);
	VkUtilImageBarrier(cmd, rc.ShadowTexture.Image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
	
	if (m_Reproject)
	{
		VkUtilImageBarrier(cmd, m_TemporalTextures[rc.FrameCounter & 1].Image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);
		VkUtilImageBarrier(cmd, m_VarianceTextures[0].Image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);

		{
			VkPushLabel(cmd, "Shadows Reproject");

			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_ReprojectPipeline);

			static uint32_t last_frame= ~0U;
			const bool is_hist_valid = last_frame == rc.FrameCounter;
			last_frame = rc.FrameCounter + 1;

			struct Constants
			{
				float		AlphaShadow;
				float		AlphaMoments;
				uint32_t	IsHistValid;
			};
			VkAllocation constants_allocation = VkAllocateUploadBuffer(sizeof(Constants));
			Constants* constants = reinterpret_cast<Constants*>(constants_allocation.Data);
			constants->AlphaShadow = m_ReprojectAlphaShadow;
			constants->AlphaMoments = m_ReprojectAlphaMoments;
			constants->IsHistValid = is_hist_valid;

			VkDescriptorSet set = VkCreateDescriptorSetForCurrentFrame(m_ReprojectDescriptorSetLayout,
				{
					{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, constants_allocation.Buffer, constants_allocation.Offset, sizeof(Constants) },
					{ 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0, m_VarianceTextures[0].ImageView, VK_IMAGE_LAYOUT_GENERAL, VK_NULL_HANDLE },
					{ 2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0, m_TemporalTextures[rc.FrameCounter & 1].ImageView, VK_IMAGE_LAYOUT_GENERAL, VK_NULL_HANDLE },
					{ 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, m_TemporalTextures[(rc.FrameCounter + 1) & 1].ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rc.NearestClamp },
					{ 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, rc.ShadowTexture.ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rc.NearestClamp },
					{ 5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, rc.MotionTexture.ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rc.NearestClamp },
					{ 6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, rc.LinearDepthTextures[rc.FrameCounter & 1].ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rc.NearestClamp },
					{ 7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, rc.LinearDepthTextures[(rc.FrameCounter + 1) & 1].ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rc.NearestClamp },
				});
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_ReprojectPipelineLayout, 0, 1, &set, 0, NULL);

			vkCmdDispatch(cmd, (rc.Width + 7) / 8, (rc.Height + 7) / 8, 1);

			VkPopLabel(cmd);
		}

		VkUtilImageBarrier(cmd, m_TemporalTextures[rc.FrameCounter & 1].Image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
		VkUtilImageBarrier(cmd, m_VarianceTextures[0].Image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);

		if (m_Filter)
		{
			VkPushLabel(cmd, "Shadows Filter");

			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_FilterPipeline);

			for (int32_t i = 0; i < m_FilterIterations; ++i)
			{
				uint32_t src_index = i & 1;
				uint32_t dst_index = 1 - src_index;

				VkUtilImageBarrier(cmd, m_VarianceTextures[dst_index].Image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);

				struct Constants
				{
					int32_t	StepSize;
					float	PhiVariance;
				};
				VkAllocation constants_allocation = VkAllocateUploadBuffer(sizeof(Constants));
				Constants* constants = reinterpret_cast<Constants*>(constants_allocation.Data);
				constants->StepSize = 1 << i;
				constants->PhiVariance = m_FilterPhiVariance;

				VkDescriptorSet set = VkCreateDescriptorSetForCurrentFrame(m_FilterDescriptorSetLayout,
					{
						{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, constants_allocation.Buffer, constants_allocation.Offset, sizeof(Constants) },
						{ 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0, m_VarianceTextures[dst_index].ImageView, VK_IMAGE_LAYOUT_GENERAL, VK_NULL_HANDLE },
						{ 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, m_VarianceTextures[src_index].ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rc.NearestClamp },
						{ 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, rc.LinearDepthTextures[rc.FrameCounter & 1].ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rc.NearestClamp },
					});
				vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_FilterPipelineLayout, 0, 1, &set, 0, NULL);

				vkCmdDispatch(cmd, (rc.Width + 7) / 8, (rc.Height + 7) / 8, 1);

				VkUtilImageBarrier(cmd, m_VarianceTextures[dst_index].Image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
			}

			VkPopLabel(cmd);
		}

		VkUtilImageBarrier(cmd, rc.ShadowTexture.Image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);

		{
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_ResolvePipeline);

			uint32_t variance_texture_index = m_Filter ? (m_FilterIterations & 1) : 0;
			VkImageView variance_image_view = m_VarianceTextures[variance_texture_index].ImageView;

			VkDescriptorSet set = VkCreateDescriptorSetForCurrentFrame(m_ResolveDescriptorSetLayout,
				{
					{ 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0, rc.ShadowTexture.ImageView, VK_IMAGE_LAYOUT_GENERAL, VK_NULL_HANDLE },
					{ 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, variance_image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rc.NearestClamp },
				});
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_ResolvePipelineLayout, 0, 1, &set, 0, NULL);

			vkCmdDispatch(cmd, (rc.Width + 7) / 8, (rc.Height + 7) / 8, 1);
		}

		VkUtilImageBarrier(cmd, rc.ShadowTexture.Image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
	}
}