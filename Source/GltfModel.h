#pragma once

#include "Vk.h"
#include "VkTexture.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <string>

enum GltfVertexAttribute
{
	VERTEX_ATTRIBUTE_POSITION = 0,
	VERTEX_ATTRIBUTE_TEXCOORD,
	VERTEX_ATTRIBUTE_NORMAL,
	VERTEX_ATTRIBUTE_TANGENT,
	VERTEX_ATTRIBUTE_COUNT,
};

struct GltfInstance
{
	uint32_t					MeshOffset;
	uint32_t					MeshCount;
	glm::mat4					Transform;
};

struct GltfMesh
{
	uint32_t					IndexCount;
	uint32_t					IndexOffset;
	uint32_t					VertexCount;
	uint32_t					VertexOffset;
	uint32_t					MaterialIndex;
};

struct GltfMaterial
{
	uint32_t					BaseColorTextureIndex;
	uint32_t					NormalTextureIndex;
	uint32_t					MetallicRoughnessTextureIndex;
	bool						HasBaseColorTexture;
	bool						HasNormalTexture;
	bool						HasMetallicRoughnessTexture;
	bool						IsOpaque;
	glm::vec4					BaseColorFactor;
	glm::vec2					MetallicRoughnessFactor;
};

class GltfModel
{
public:
	std::vector<GltfInstance>	m_Instances										= {};
    std::vector<GltfMesh>		m_Meshes										= {};
    std::vector<GltfMaterial>	m_Materials										= {};
	std::vector<VkTexture>		m_Textures										= {};

    VkBuffer					m_VertexBuffer									= VK_NULL_HANDLE;
    VmaAllocation				m_VertexBufferAllocation						= VK_NULL_HANDLE;

    VkBuffer					m_IndexBuffer									= VK_NULL_HANDLE;
    VmaAllocation				m_IndexBufferAllocation							= VK_NULL_HANDLE;

    VkDeviceSize				m_VertexBufferOffsets[VERTEX_ATTRIBUTE_COUNT]	= {};

    bool						Load(const std::string& filepath);
    void						Destroy();

	void						Transform(const glm::mat4& transform);

    void						BindVertexBuffer(VkCommandBuffer cmd, uint32_t binding, GltfVertexAttribute attribute) const;
    void						BindIndexBuffer(VkCommandBuffer cmd) const;
    void						Draw(VkCommandBuffer cmd, uint32_t mesh_index, uint32_t instance_count = 1) const;
};
