#pragma once

#include "RenderContext.h"

#include <GLFW/glfw3.h>
#include <imgui.h>

class RenderImGui
{
public:
    GLFWcursor*				m_Cursors[ImGuiMouseCursor_COUNT]   = {};

    VkDescriptorSetLayout	m_DescriptorSetLayout               = VK_NULL_HANDLE;
    VkPipelineLayout		m_PipelineLayout                    = VK_NULL_HANDLE;
    VkPipeline				m_Pipeline                          = VK_NULL_HANDLE;

    VkTexture				m_FontTexture						= {};
                  
    VkBuffer				m_VertexBuffer                      = VK_NULL_HANDLE;
    VmaAllocation			m_VertexBufferAllocation            = VK_NULL_HANDLE;
    VkDeviceSize			m_VertexBufferSize                  = 0;

    VkBuffer				m_IndexBuffer                       = VK_NULL_HANDLE;
    VmaAllocation			m_IndexBufferAllocation             = VK_NULL_HANDLE;
    VkDeviceSize			m_IndexBufferSize                   = 0;

    void                    Create(const RenderContext& rc, GLFWwindow* window);
    void                    Destroy();

    void                    Update(GLFWwindow* window);
    void                    Draw(const RenderContext& rc, VkCommandBuffer cmd);
};
