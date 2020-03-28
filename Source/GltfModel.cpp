#include "GltfModel.h"

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#include <vector>
#include <unordered_map>
#include <assert.h>

static void TraverseNodeHierarchy(const cgltf_data* data, const cgltf_node* node, const glm::mat4& parent_transform, std::vector<GltfInstance>& instances)
{
	glm::mat4					transform = glm::identity<glm::mat4>();
	if (node->has_matrix)		transform *= glm::make_mat4(node->matrix);
	if (node->has_translation)	transform *= glm::translate(glm::make_vec3(node->translation));
	if (node->has_rotation)		transform *= glm::toMat4(glm::make_quat(node->rotation));
	if (node->has_scale)		transform *= glm::scale(glm::make_vec3(node->scale));
	transform *= parent_transform;

	if (node->mesh)
	{
		cgltf_size count = node->mesh->primitives_count;
		cgltf_size offset = 0;
		for (cgltf_size i = 0; i < data->meshes_count; ++i)
		{
			if (node->mesh == &data->meshes[i])
				break;
			offset += data->meshes[i].primitives_count;
		}

		GltfInstance instance;
		instance.MeshOffset = static_cast<uint32_t>(offset);
		instance.MeshCount = static_cast<uint32_t>(count);
		instance.Transform = transform;
		instances.emplace_back(instance);
	}

	for (cgltf_size i = 0; i < node->children_count; ++i)
	{
		TraverseNodeHierarchy(data, node->children[i], transform, instances);
	}
}

bool GltfModel::Load(const std::string& filepath)
{
    cgltf_options options = {};
    cgltf_data* data = NULL;
    cgltf_result result = cgltf_parse_file(&options, filepath.c_str(), &data);
    if (result == cgltf_result_success)
        result = cgltf_load_buffers(&options, data, filepath.c_str());
    if (result == cgltf_result_success)
        result = cgltf_validate(data);
    if (result != cgltf_result_success)
    {
        cgltf_free(data);
        return false;
    }

    const size_t mesh_count = data->meshes_count;
    const size_t material_count = data->materials_count;

    size_t total_vertex_count = 0;
    size_t total_index_count = 0;

    for (size_t i = 0; i < mesh_count; ++i)
    {
        const cgltf_mesh& mesh = data->meshes[i];
        const size_t prim_count = mesh.primitives_count;
        for (size_t j = 0; j < prim_count; ++j)
        {
            const cgltf_primitive& prim = mesh.primitives[j];

            const size_t vertex_count = prim.attributes[0].data->count;
            const size_t index_count = prim.indices->count;

            size_t material_index = ~0ULL;
            for (size_t k = 0; k < material_count; ++k)
            {
                if (prim.material == (data->materials + k))
                {
                    material_index = k;
                    break;
                }
            }

            GltfMesh mesh;
            mesh.IndexCount = static_cast<uint32_t>(index_count);
            mesh.IndexOffset = static_cast<uint32_t>(total_index_count);
            mesh.VertexCount = static_cast<uint32_t>(vertex_count);
            mesh.VertexOffset = static_cast<uint32_t>(total_vertex_count);
            mesh.MaterialIndex = static_cast<uint32_t>(material_index);
            m_Meshes.emplace_back(mesh);

            total_vertex_count += vertex_count;
            total_index_count += index_count;
        }
    }

    VkDeviceSize vertex_buffer_size = 0;
    for (uint32_t attribute = 0; attribute < VERTEX_ATTRIBUTE_COUNT; ++attribute)
    {
        m_VertexBufferOffsets[attribute] = vertex_buffer_size;
        switch (attribute)
        {
        case VERTEX_ATTRIBUTE_POSITION:
            vertex_buffer_size += sizeof(glm::vec3) * total_vertex_count;
            break;
        case VERTEX_ATTRIBUTE_TEXCOORD:
            vertex_buffer_size += sizeof(glm::vec2) * total_vertex_count;
            break;
        case VERTEX_ATTRIBUTE_NORMAL:
            vertex_buffer_size += sizeof(glm::vec3) * total_vertex_count;
            break;
        case VERTEX_ATTRIBUTE_TANGENT:
            vertex_buffer_size += sizeof(glm::vec4) * total_vertex_count;
            break;
        }
		vertex_buffer_size = VkAlignUp(vertex_buffer_size, Vk.PhysicalDeviceProperties.limits.minStorageBufferOffsetAlignment);
    }

    VkBufferCreateInfo vertex_buffer_info = {};
    vertex_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vertex_buffer_info.size = vertex_buffer_size;
    vertex_buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | (Vk.IsRayTracingSupported ? VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT : 0);
    vertex_buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo vertex_buffer_allocation_info = {};
    vertex_buffer_allocation_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    VK(vmaCreateBuffer(Vk.Allocator, &vertex_buffer_info, &vertex_buffer_allocation_info, &m_VertexBuffer, &m_VertexBufferAllocation, NULL));

    VkDeviceSize index_buffer_size = sizeof(uint16_t) * total_index_count;

    VkBufferCreateInfo index_buffer_info = {};
    index_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    index_buffer_info.size = index_buffer_size;
    index_buffer_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | (Vk.IsRayTracingSupported ? VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT : 0);
    index_buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo index_buffer_allocation_info = {};
    index_buffer_allocation_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    VK(vmaCreateBuffer(Vk.Allocator, &index_buffer_info, &index_buffer_allocation_info, &m_IndexBuffer, &m_IndexBufferAllocation, NULL));

    VkDeviceSize buffer_size = vertex_buffer_size + index_buffer_size;
    VkAllocation buffer_allocation = VkAllocateUploadBuffer(buffer_size);

    float* positions = reinterpret_cast<float*>(buffer_allocation.Data + m_VertexBufferOffsets[VERTEX_ATTRIBUTE_POSITION]);
    float* texcoords = reinterpret_cast<float*>(buffer_allocation.Data + m_VertexBufferOffsets[VERTEX_ATTRIBUTE_TEXCOORD]);
    float* normals = reinterpret_cast<float*>(buffer_allocation.Data + m_VertexBufferOffsets[VERTEX_ATTRIBUTE_NORMAL]);
    float* tangents = reinterpret_cast<float*>(buffer_allocation.Data + m_VertexBufferOffsets[VERTEX_ATTRIBUTE_TANGENT]);
	uint16_t* indices = reinterpret_cast<uint16_t*>(buffer_allocation.Data + vertex_buffer_size);

    for (size_t i = 0, vertex_offset = 0, index_offset = 0; i < mesh_count; ++i)
    {
        const cgltf_mesh& mesh = data->meshes[i];
        const size_t prim_count = mesh.primitives_count;
        for (size_t j = 0; j < prim_count; ++j)
        {
            const cgltf_primitive& prim = mesh.primitives[j];

            // Copy vertex data
            const size_t attribute_count = prim.attributes_count;
            for (size_t k = 0; k < attribute_count; ++k)
            {
                cgltf_accessor* accessor = prim.attributes[k].data;
                cgltf_buffer_view* buffer_view = accessor->buffer_view;
                switch (prim.attributes[k].type)
                {
                case cgltf_attribute_type_position:
                    assert(accessor->component_type == cgltf_component_type_r_32f);
                    assert(accessor->type == cgltf_type_vec3);
                    memcpy(positions + vertex_offset * 3, static_cast<uint8_t*>(buffer_view->buffer->data) + buffer_view->offset + accessor->offset, accessor->count * accessor->stride);
                    break;
                case cgltf_attribute_type_texcoord:
                    assert(accessor->component_type == cgltf_component_type_r_32f);
                    assert(accessor->type == cgltf_type_vec2);
                    memcpy(texcoords + vertex_offset * 2, static_cast<uint8_t*>(buffer_view->buffer->data) + buffer_view->offset + accessor->offset, accessor->count * accessor->stride);
                    break;
                case cgltf_attribute_type_normal:
                    assert(accessor->component_type == cgltf_component_type_r_32f);
                    assert(accessor->type == cgltf_type_vec3);
                    memcpy(normals + vertex_offset * 3, static_cast<uint8_t*>(buffer_view->buffer->data) + buffer_view->offset + accessor->offset, accessor->count * accessor->stride);
                    break;
                case cgltf_attribute_type_tangent:
                    assert(accessor->component_type == cgltf_component_type_r_32f);
                    assert(accessor->type == cgltf_type_vec4);
                    memcpy(tangents + vertex_offset * 4, static_cast<uint8_t*>(buffer_view->buffer->data) + buffer_view->offset + accessor->offset, accessor->count * accessor->stride);
                    break;
                }
            }

            // Copy index data
            cgltf_accessor* accessor = prim.indices;
            cgltf_buffer_view* buffer_view = accessor->buffer_view;
            assert(accessor->component_type == cgltf_component_type_r_16u);
            assert(accessor->type == cgltf_type_scalar);
            memcpy(indices + index_offset, static_cast<uint8_t*>(buffer_view->buffer->data) + buffer_view->offset + accessor->offset, accessor->count * accessor->stride);

            vertex_offset += prim.attributes[0].data->count;
            index_offset += prim.indices->count;
        }
    }

    VkRecordCommands(
        [=](VkCommandBuffer cmd)
        {
            VkBufferMemoryBarrier pre_transfer_barriers[2] = {};
            // Vertex buffer
            pre_transfer_barriers[0].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            pre_transfer_barriers[0].srcAccessMask = 0;
            pre_transfer_barriers[0].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            pre_transfer_barriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            pre_transfer_barriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            pre_transfer_barriers[0].buffer = m_VertexBuffer;
            pre_transfer_barriers[0].offset = 0;
            pre_transfer_barriers[0].size = vertex_buffer_size;
            // Index buffer
            pre_transfer_barriers[1].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            pre_transfer_barriers[1].srcAccessMask = 0;
            pre_transfer_barriers[1].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            pre_transfer_barriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            pre_transfer_barriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            pre_transfer_barriers[1].buffer = m_IndexBuffer;
            pre_transfer_barriers[1].offset = 0;
            pre_transfer_barriers[1].size = index_buffer_size;
            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 2, pre_transfer_barriers, 0, NULL);

            VkBufferCopy vertex_buffer_copy_region;
            vertex_buffer_copy_region.srcOffset = buffer_allocation.Offset;
            vertex_buffer_copy_region.dstOffset = 0;
            vertex_buffer_copy_region.size = vertex_buffer_size;
            vkCmdCopyBuffer(cmd, buffer_allocation.Buffer, m_VertexBuffer, 1, &vertex_buffer_copy_region);

            VkBufferCopy index_buffer_copy_region;
            index_buffer_copy_region.srcOffset = buffer_allocation.Offset + vertex_buffer_size;
            index_buffer_copy_region.dstOffset = 0;
            index_buffer_copy_region.size = index_buffer_size;
            vkCmdCopyBuffer(cmd, buffer_allocation.Buffer, m_IndexBuffer, 1, &index_buffer_copy_region);

            VkBufferMemoryBarrier post_transfer_barriers[2] = {};
            // Vertex buffer
            post_transfer_barriers[0].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            post_transfer_barriers[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            post_transfer_barriers[0].dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
            post_transfer_barriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            post_transfer_barriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            post_transfer_barriers[0].buffer = m_VertexBuffer;
            post_transfer_barriers[0].offset = 0;
            post_transfer_barriers[0].size = vertex_buffer_size;
            // Index buffer
            post_transfer_barriers[1].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            post_transfer_barriers[1].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            post_transfer_barriers[1].dstAccessMask = VK_ACCESS_INDEX_READ_BIT;
            post_transfer_barriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            post_transfer_barriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            post_transfer_barriers[1].buffer = m_IndexBuffer;
            post_transfer_barriers[1].offset = 0;
            post_transfer_barriers[1].size = index_buffer_size;
            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, NULL, 2, post_transfer_barriers, 0, NULL);
        });

    size_t last_slash = filepath.find_last_of("/\\");
    std::string directory = last_slash != std::string::npos ? filepath.substr(0, last_slash + 1) : "";

    std::unordered_map<std::string, uint32_t> texture_map;

    const uint32_t default_base_color_texture_index = static_cast<uint32_t>(m_Textures.size());
    glm::uint default_base_color = glm::packUnorm4x8(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    VkTextureCreateParams default_base_color_texture_params;
    default_base_color_texture_params.Type = VK_IMAGE_TYPE_2D;
    default_base_color_texture_params.ViewType = VK_IMAGE_VIEW_TYPE_2D;
    default_base_color_texture_params.Width = 1;
    default_base_color_texture_params.Height = 1;
    default_base_color_texture_params.Format = VK_FORMAT_R8G8B8A8_UNORM;
    default_base_color_texture_params.Usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    default_base_color_texture_params.InitialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    default_base_color_texture_params.Data = &default_base_color;
    default_base_color_texture_params.DataSize = sizeof(default_base_color);
    m_Textures.emplace_back(VkTextureCreate(default_base_color_texture_params));

    const uint32_t default_normal_texture_index = static_cast<uint32_t>(m_Textures.size());
    glm::uint default_normal = glm::packUnorm4x8(glm::vec4(0.5f, 0.5f, 1.0f, 0.0f));
    VkTextureCreateParams default_normal_texture_params;
    default_normal_texture_params.Type = VK_IMAGE_TYPE_2D;
    default_normal_texture_params.ViewType = VK_IMAGE_VIEW_TYPE_2D;
    default_normal_texture_params.Width = 1;
    default_normal_texture_params.Height = 1;
    default_normal_texture_params.Format = VK_FORMAT_R8G8B8A8_UNORM;
    default_normal_texture_params.Usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    default_normal_texture_params.InitialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    default_normal_texture_params.Data = &default_normal;
    default_normal_texture_params.DataSize = sizeof(default_normal);
    m_Textures.emplace_back(VkTextureCreate(default_normal_texture_params));

    const uint32_t default_metallic_roughness_texture_index = static_cast<uint32_t>(m_Textures.size());
    glm::uint default_metallic_roughness = glm::packUnorm4x8(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    VkTextureCreateParams default_metallic_roughness_texture_params;
    default_metallic_roughness_texture_params.Type = VK_IMAGE_TYPE_2D;
    default_metallic_roughness_texture_params.ViewType = VK_IMAGE_VIEW_TYPE_2D;
    default_metallic_roughness_texture_params.Width = 1;
    default_metallic_roughness_texture_params.Height = 1;
    default_metallic_roughness_texture_params.Format = VK_FORMAT_R8G8B8A8_UNORM;
    default_metallic_roughness_texture_params.Usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    default_metallic_roughness_texture_params.InitialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    default_metallic_roughness_texture_params.Data = &default_metallic_roughness;
    default_metallic_roughness_texture_params.DataSize = sizeof(default_metallic_roughness);
    m_Textures.emplace_back(VkTextureCreate(default_metallic_roughness_texture_params));

    m_Materials.resize(material_count);
    for (size_t i = 0; i < material_count; ++i)
    {
        const cgltf_material& material = data->materials[i];
        assert(material.has_pbr_metallic_roughness);
        const cgltf_pbr_metallic_roughness& pbr_material = material.pbr_metallic_roughness;

        bool has_base_color_texture = pbr_material.base_color_texture.texture && pbr_material.base_color_texture.texture->image;
        bool has_normal_texture = material.normal_texture.texture && material.normal_texture.texture->image;
        bool has_metallic_roughness_texture = pbr_material.metallic_roughness_texture.texture && pbr_material.metallic_roughness_texture.texture->image;

        const char* base_color_texture_filename = has_base_color_texture ? pbr_material.base_color_texture.texture->image->uri : NULL;
        const char* normal_texture_filename = has_normal_texture ? material.normal_texture.texture->image->uri : NULL;
        const char* metallic_roughness_texture_filename = has_metallic_roughness_texture ? pbr_material.metallic_roughness_texture.texture->image->uri : NULL;
        
        struct TextureInfo
        {
            const char* m_Filename;
            bool        m_Srgb;
        };
        TextureInfo info[] =
        {
            { base_color_texture_filename, true },
            { normal_texture_filename, false },
            { metallic_roughness_texture_filename, false },
        };
        const size_t info_count = sizeof(info) / sizeof(*info);

        for (size_t j = 0; j < info_count; ++j)
        {
            if (info[j].m_Filename == NULL || texture_map.find(info[j].m_Filename) != texture_map.end())
                continue;

            texture_map[info[j].m_Filename] = static_cast<uint32_t>(m_Textures.size());
            m_Textures.emplace_back(VkTextureLoad((directory + info[j].m_Filename).c_str(), info[j].m_Srgb));
        }

        has_base_color_texture = has_base_color_texture && texture_map.find(base_color_texture_filename) != texture_map.end();
        has_normal_texture = has_normal_texture && texture_map.find(normal_texture_filename) != texture_map.end();
        has_metallic_roughness_texture = has_metallic_roughness_texture && texture_map.find(metallic_roughness_texture_filename) != texture_map.end();

        m_Materials[i].BaseColorTextureIndex = has_base_color_texture ? texture_map[base_color_texture_filename] : default_base_color_texture_index;
        m_Materials[i].NormalTextureIndex = has_normal_texture ? texture_map[normal_texture_filename] : default_normal_texture_index;
        m_Materials[i].MetallicRoughnessTextureIndex = has_metallic_roughness_texture ? texture_map[metallic_roughness_texture_filename] : default_metallic_roughness_texture_index;

		m_Materials[i].HasBaseColorTexture = has_base_color_texture;
		m_Materials[i].HasNormalTexture = has_normal_texture;
		m_Materials[i].HasMetallicRoughnessTexture = has_metallic_roughness_texture;
		m_Materials[i].IsOpaque = material.alpha_mode == cgltf_alpha_mode_opaque;

        m_Materials[i].BaseColorFactor = glm::make_vec4(pbr_material.base_color_factor);
        m_Materials[i].MetallicRoughnessFactor = glm::vec2(pbr_material.metallic_factor, pbr_material.roughness_factor);
    }

	const cgltf_size scene_count = data->scenes_count;
	for (cgltf_size i = 0; i < scene_count; ++i)
	{
		const cgltf_size node_count = data->scenes[i].nodes_count;
		for (cgltf_size j = 0; j < node_count; ++j)
		{
			TraverseNodeHierarchy(data, data->scenes[i].nodes[j], glm::identity<glm::mat4>(), m_Instances);
		}
	}

    cgltf_free(data);

    return true;
}
void GltfModel::Destroy()
{
    for (const VkTexture& texture : m_Textures)
    {
		VkTextureDestroy(texture);
    }

    vmaDestroyBuffer(Vk.Allocator, m_VertexBuffer, m_VertexBufferAllocation);
    vmaDestroyBuffer(Vk.Allocator, m_IndexBuffer, m_IndexBufferAllocation);
}

void GltfModel::Transform(const glm::mat4& transform)
{
	for (GltfInstance& instance : m_Instances)
	{
		instance.Transform = transform * instance.Transform;
	}
}

void GltfModel::BindVertexBuffer(VkCommandBuffer cmd, uint32_t binding, GltfVertexAttribute attribute) const
{
    vkCmdBindVertexBuffers(cmd, binding, 1, &m_VertexBuffer, &m_VertexBufferOffsets[attribute]);
}
void GltfModel::BindIndexBuffer(VkCommandBuffer cmd) const
{
    vkCmdBindIndexBuffer(cmd, m_IndexBuffer, 0, VK_INDEX_TYPE_UINT16);
}
void GltfModel::Draw(VkCommandBuffer cmd, uint32_t mesh_index, uint32_t instance_count) const
{
    vkCmdDrawIndexed(cmd, m_Meshes[mesh_index].IndexCount, instance_count, m_Meshes[mesh_index].IndexOffset, m_Meshes[mesh_index].VertexOffset, 0);
}