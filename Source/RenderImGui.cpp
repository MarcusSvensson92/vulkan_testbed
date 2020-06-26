#include "RenderImGui.h"
#include "VkUtil.h"

#ifdef _WIN32
	#define GLFW_EXPOSE_NATIVE_WIN32
#endif
#include <GLFW/glfw3native.h>

static const char* RenderImGuiGetClipboardText(void* user_data)
{
    return glfwGetClipboardString(static_cast<GLFWwindow*>(user_data));
}
static void RenderImGuiSetClipboardText(void* user_data, const char* text)
{
    glfwSetClipboardString(static_cast<GLFWwindow*>(user_data), text);
}

static void RenderImGuiScrollCallback(GLFWwindow*, double xoffset, double yoffset)
{
    ImGuiIO& io = ImGui::GetIO();
    io.MouseWheelH += (float)xoffset;
    io.MouseWheel += (float)yoffset;
}
static void RenderImGuiKeyCallback(GLFWwindow*, int key, int, int action, int)
{
    ImGuiIO& io = ImGui::GetIO();
    if (action == GLFW_PRESS)
        io.KeysDown[key] = true;
    if (action == GLFW_RELEASE)
        io.KeysDown[key] = false;
    io.KeyCtrl = io.KeysDown[GLFW_KEY_LEFT_CONTROL] || io.KeysDown[GLFW_KEY_RIGHT_CONTROL];
    io.KeyShift = io.KeysDown[GLFW_KEY_LEFT_SHIFT] || io.KeysDown[GLFW_KEY_RIGHT_SHIFT];
    io.KeyAlt = io.KeysDown[GLFW_KEY_LEFT_ALT] || io.KeysDown[GLFW_KEY_RIGHT_ALT];
    io.KeySuper = io.KeysDown[GLFW_KEY_LEFT_SUPER] || io.KeysDown[GLFW_KEY_RIGHT_SUPER];
}
static void RenderImGuiCharCallback(GLFWwindow*, unsigned int c)
{
    ImGuiIO& io = ImGui::GetIO();
    if (c > 0 && c < 0x10000)
        io.AddInputCharacter((unsigned short)c);
}

void RenderImGui::Create(const RenderContext& rc, GLFWwindow* window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
    io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;
    io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
    io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = GLFW_KEY_PAGE_UP;
    io.KeyMap[ImGuiKey_PageDown] = GLFW_KEY_PAGE_DOWN;
    io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
    io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
    io.KeyMap[ImGuiKey_Insert] = GLFW_KEY_INSERT;
    io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
    io.KeyMap[ImGuiKey_Space] = GLFW_KEY_SPACE;
    io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
    io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
    io.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
    io.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
    io.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
    io.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
    io.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
    io.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;

    io.SetClipboardTextFn = RenderImGuiSetClipboardText;
    io.GetClipboardTextFn = RenderImGuiGetClipboardText;
    io.ClipboardUserData = window;
#ifdef _WIN32
    io.ImeWindowHandle = glfwGetWin32Window(window);
#endif

    memset(m_Cursors, 0, sizeof(m_Cursors));
    m_Cursors[ImGuiMouseCursor_Arrow] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    m_Cursors[ImGuiMouseCursor_TextInput] = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
    m_Cursors[ImGuiMouseCursor_ResizeAll] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    m_Cursors[ImGuiMouseCursor_ResizeNS] = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
    m_Cursors[ImGuiMouseCursor_ResizeEW] = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
    m_Cursors[ImGuiMouseCursor_ResizeNESW] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    m_Cursors[ImGuiMouseCursor_ResizeNWSE] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    m_Cursors[ImGuiMouseCursor_Hand] = glfwCreateStandardCursor(GLFW_HAND_CURSOR);

    glfwSetScrollCallback(window, RenderImGuiScrollCallback);
    glfwSetKeyCallback(window, RenderImGuiKeyCallback);
    glfwSetCharCallback(window, RenderImGuiCharCallback);

	VkDescriptorSetLayoutBinding descriptor_set_layout_binding = {};
	descriptor_set_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptor_set_layout_binding.descriptorCount = 1;
	descriptor_set_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	descriptor_set_layout_binding.pImmutableSamplers = &rc.LinearClamp;

	VkDescriptorSetLayoutCreateInfo descriptor_set_layout_info = {};
	descriptor_set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptor_set_layout_info.bindingCount = 1;
	descriptor_set_layout_info.pBindings = &descriptor_set_layout_binding;
	VK(vkCreateDescriptorSetLayout(Vk.Device, &descriptor_set_layout_info, NULL, &m_DescriptorSetLayout));

	VkPushConstantRange push_constants = {};
	push_constants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	push_constants.offset = 0;
	push_constants.size = sizeof(float) * 4;

	VkPipelineLayoutCreateInfo pipeline_layout_info = {};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = 1;
	pipeline_layout_info.pSetLayouts = &m_DescriptorSetLayout;
	pipeline_layout_info.pushConstantRangeCount = 1;
	pipeline_layout_info.pPushConstantRanges = &push_constants;
	VK(vkCreatePipelineLayout(Vk.Device, &pipeline_layout_info, NULL, &m_PipelineLayout));

	VkPipelineColorBlendAttachmentState blend_attachment_state = {};
	blend_attachment_state.blendEnable = VK_TRUE;
	blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
	blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;
	blend_attachment_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	VkUtilCreateGraphicsPipelineParams pipeline_params;
	pipeline_params.VertexBindingDescriptions =
	{
		{ 0, sizeof(ImDrawVert), VK_VERTEX_INPUT_RATE_VERTEX },
	};
	pipeline_params.VertexAttributeDescriptions =
	{
		{ 0, 0, VK_FORMAT_R32G32_SFLOAT,  IM_OFFSETOF(ImDrawVert, pos) },
		{ 1, 0, VK_FORMAT_R32G32_SFLOAT,  IM_OFFSETOF(ImDrawVert, uv)  },
		{ 2, 0, VK_FORMAT_R8G8B8A8_UNORM, IM_OFFSETOF(ImDrawVert, col) },
	};
	pipeline_params.PipelineLayout = m_PipelineLayout;
	pipeline_params.RenderPass = rc.UiRenderPass;
	pipeline_params.VertexShaderFilepath = "../Assets/Shaders/ImGui.vert";
	pipeline_params.FragmentShaderFilepath = "../Assets/Shaders/ImGui.frag";
	pipeline_params.BlendAttachmentStates = { blend_attachment_state };
	m_Pipeline = VkUtilCreateGraphicsPipeline(pipeline_params);

    uint8_t* font_texture_pixels;
    int font_texture_width, font_texture_height;
    io.Fonts->GetTexDataAsRGBA32(&font_texture_pixels, &font_texture_width, &font_texture_height);

    VkTextureCreateParams font_texture_params;
    font_texture_params.Type = VK_IMAGE_TYPE_2D;
    font_texture_params.ViewType = VK_IMAGE_VIEW_TYPE_2D;
    font_texture_params.Width = static_cast<uint32_t>(font_texture_width);
    font_texture_params.Height = static_cast<uint32_t>(font_texture_height);
    font_texture_params.Format = VK_FORMAT_R8G8B8A8_UNORM;
    font_texture_params.Usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    font_texture_params.InitialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    font_texture_params.Data = font_texture_pixels;
    font_texture_params.DataSize = font_texture_width * font_texture_height * 4;
	m_FontTexture = VkTextureCreate(font_texture_params);

    m_VertexBuffer = VK_NULL_HANDLE;
    m_IndexBuffer = VK_NULL_HANDLE;
    m_VertexBufferSize = 0;
    m_IndexBufferSize = 0;
}

void RenderImGui::Destroy()
{
    if (m_VertexBuffer != VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(Vk.Allocator, m_VertexBuffer, m_VertexBufferAllocation);
    }
    if (m_IndexBuffer != VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(Vk.Allocator, m_IndexBuffer, m_IndexBufferAllocation);
    }

	VkTextureDestroy(m_FontTexture);

	vkDestroyPipeline(Vk.Device, m_Pipeline, NULL);
	vkDestroyPipelineLayout(Vk.Device, m_PipelineLayout, NULL);
	vkDestroyDescriptorSetLayout(Vk.Device, m_DescriptorSetLayout, NULL);

    ImGui::DestroyContext();

    for (ImGuiMouseCursor i = 0; i < ImGuiMouseCursor_COUNT; ++i)
    {
        glfwDestroyCursor(m_Cursors[i]);
    }
}

void RenderImGui::Update(GLFWwindow* window)
{
    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(io.Fonts->IsBuilt());

    int window_width, window_height;
    int display_width, display_height;
    glfwGetWindowSize(window, &window_width, &window_height);
    glfwGetFramebufferSize(window, &display_width, &display_height);
    io.DisplaySize = ImVec2(static_cast<float>(window_width), static_cast<float>(window_height));
    io.DisplayFramebufferScale = ImVec2(window_width > 0 ? static_cast<float>(display_width) / static_cast<float>(window_width) : 0.0f, window_height > 0 ? static_cast<float>(display_height) / static_cast<float>(window_height) : 0.0f);

    static double prev_time = glfwGetTime();
    double curr_time = glfwGetTime();
    float delta_time = static_cast<float>(curr_time - prev_time);
    io.DeltaTime = delta_time > 0.0f ? delta_time : 1.0f / 60.0f;
    prev_time = curr_time;

    for (uint32_t i = 0; i < IM_ARRAYSIZE(ImGuiIO::MouseDown); ++i)
    {
        io.MouseDown[i] = glfwGetMouseButton(window, i) != 0;
    }

    const ImVec2 mouse_pos_backup = io.MousePos;
    io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
    if (glfwGetWindowAttrib(window, GLFW_FOCUSED) != 0)
    {
        if (io.WantSetMousePos)
        {
            glfwSetCursorPos(window, static_cast<double>(mouse_pos_backup.x), static_cast<double>(mouse_pos_backup.y));
        }
        else
        {
            double mouse_x, mouse_y;
            glfwGetCursorPos(window, &mouse_x, &mouse_y);
            io.MousePos = ImVec2(static_cast<float>(mouse_x), static_cast<float>(mouse_y));
        }
    }

    if ((io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) == 0 && glfwGetInputMode(window, GLFW_CURSOR) != GLFW_CURSOR_DISABLED)
    {
        ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
        if (imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
        }
        else
        {
            glfwSetCursor(window, m_Cursors[imgui_cursor] ? m_Cursors[imgui_cursor] : m_Cursors[ImGuiMouseCursor_Arrow]);
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
}

void RenderImGui::Draw(const RenderContext& rc, VkCommandBuffer cmd)
{
    ImDrawData* draw_data = ImGui::GetDrawData();
    if (draw_data->TotalVtxCount == 0)
        return;

    const size_t total_vtx_size = draw_data->TotalVtxCount * sizeof(ImDrawVert);
    const size_t total_idx_size = draw_data->TotalIdxCount * sizeof(ImDrawIdx);
    if (m_VertexBuffer == VK_NULL_HANDLE || m_VertexBufferSize < total_vtx_size)
    {
        vkQueueWaitIdle(Vk.GraphicsQueue);

        if (m_VertexBuffer != VK_NULL_HANDLE)
        {
            vmaDestroyBuffer(Vk.Allocator, m_VertexBuffer, m_VertexBufferAllocation);
        }

        VkBufferCreateInfo buffer_info = {};
        buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.size = total_vtx_size;
        buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo buffer_allocation_info = {};
        buffer_allocation_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        VK(vmaCreateBuffer(Vk.Allocator, &buffer_info, &buffer_allocation_info, &m_VertexBuffer, &m_VertexBufferAllocation, NULL));

        m_VertexBufferSize = total_vtx_size;

    }
    if (m_IndexBuffer == VK_NULL_HANDLE || m_IndexBufferSize < total_idx_size)
    {
        vkQueueWaitIdle(Vk.GraphicsQueue);

        if (m_IndexBuffer != VK_NULL_HANDLE)
        {
            vmaDestroyBuffer(Vk.Allocator, m_IndexBuffer, m_IndexBufferAllocation);
        }

        VkBufferCreateInfo buffer_info = {};
        buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.size = total_idx_size;
        buffer_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo buffer_allocation_info = {};
        buffer_allocation_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        VK(vmaCreateBuffer(Vk.Allocator, &buffer_info, &buffer_allocation_info, &m_IndexBuffer, &m_IndexBufferAllocation, NULL));

        m_IndexBufferSize = total_idx_size;
    }

    VkAllocation vtx_allocation = VkAllocateUploadBuffer(total_vtx_size);
    VkAllocation idx_allocation = VkAllocateUploadBuffer(total_idx_size);
    for (int i = 0; i < draw_data->CmdListsCount; ++i)
    {
        const ImDrawList* im_cmd_list = draw_data->CmdLists[i];
        const size_t vtx_size = im_cmd_list->VtxBuffer.Size * sizeof(ImDrawVert);
        const size_t idx_size = im_cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx);
        memcpy(vtx_allocation.Data, im_cmd_list->VtxBuffer.Data, vtx_size);
        memcpy(idx_allocation.Data, im_cmd_list->IdxBuffer.Data, idx_size);
        vtx_allocation.Data += vtx_size;
        idx_allocation.Data += idx_size;
    }

    VkBufferMemoryBarrier pre_transfer_barriers[2] = {};
    // Vertex buffer
    pre_transfer_barriers[0].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    pre_transfer_barriers[0].srcAccessMask = 0;
    pre_transfer_barriers[0].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    pre_transfer_barriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    pre_transfer_barriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    pre_transfer_barriers[0].buffer = m_VertexBuffer;
    pre_transfer_barriers[0].offset = 0;
    pre_transfer_barriers[0].size = total_vtx_size;
    // Index buffer
    pre_transfer_barriers[1].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    pre_transfer_barriers[1].srcAccessMask = 0;
    pre_transfer_barriers[1].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    pre_transfer_barriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    pre_transfer_barriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    pre_transfer_barriers[1].buffer = m_IndexBuffer;
    pre_transfer_barriers[1].offset = 0;
    pre_transfer_barriers[1].size = total_idx_size;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 2, pre_transfer_barriers, 0, NULL);

    VkBufferCopy vtx_copy_region;
    vtx_copy_region.srcOffset = vtx_allocation.Offset;
    vtx_copy_region.dstOffset = 0;
    vtx_copy_region.size = total_vtx_size;
    vkCmdCopyBuffer(cmd, vtx_allocation.Buffer, m_VertexBuffer, 1, &vtx_copy_region);

    VkBufferCopy idx_copy_region;
    idx_copy_region.srcOffset = idx_allocation.Offset;
    idx_copy_region.dstOffset = 0;
    idx_copy_region.size = total_idx_size;
    vkCmdCopyBuffer(cmd, idx_allocation.Buffer, m_IndexBuffer, 1, &idx_copy_region);

    VkBufferMemoryBarrier post_transfer_barriers[2] = {};
    // Vertex buffer
    post_transfer_barriers[0].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    post_transfer_barriers[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    post_transfer_barriers[0].dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
    post_transfer_barriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    post_transfer_barriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    post_transfer_barriers[0].buffer = m_VertexBuffer;
    post_transfer_barriers[0].offset = 0;
    post_transfer_barriers[0].size = total_vtx_size;
    // Index buffer
    post_transfer_barriers[1].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    post_transfer_barriers[1].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    post_transfer_barriers[1].dstAccessMask = VK_ACCESS_INDEX_READ_BIT;
    post_transfer_barriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    post_transfer_barriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    post_transfer_barriers[1].buffer = m_IndexBuffer;
    post_transfer_barriers[1].offset = 0;
    post_transfer_barriers[1].size = total_idx_size;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, NULL, 2, post_transfer_barriers, 0, NULL);

	VkPushLabel(cmd, "ImGui");

    VkRenderPassBeginInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = rc.UiRenderPass;
    render_pass_info.framebuffer = rc.UiFramebuffer;
    render_pass_info.renderArea.offset = { 0, 0 };
    render_pass_info.renderArea.extent = { rc.Width, rc.Height };
    vkCmdBeginRenderPass(cmd, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = { 0.0f, 0.0f, draw_data->DisplaySize.x, draw_data->DisplaySize.y, 0.0f, 1.0f };
    vkCmdSetViewport(cmd, 0, 1, &viewport);

	VkClearRect clear_rect = {};
	clear_rect.baseArrayLayer = 0;
	clear_rect.layerCount = 1;
	clear_rect.rect.offset.x = 0;
	clear_rect.rect.offset.y = 0;
	clear_rect.rect.extent.width = rc.Width;
	clear_rect.rect.extent.height = rc.Height;

	VkClearAttachment clear_attachments[] =
	{
		{ VK_IMAGE_ASPECT_COLOR_BIT, 0, { 0.0f, 0.0f, 0.0f, 0.0f } },
	};
	vkCmdClearAttachments(cmd, static_cast<uint32_t>(sizeof(clear_attachments) / sizeof(*clear_attachments)), clear_attachments, 1, &clear_rect);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);

    VkDescriptorSet set = VkCreateDescriptorSetForCurrentFrame(m_DescriptorSetLayout,
        {
            { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, m_FontTexture.ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_NULL_HANDLE }
        });
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &set, 0, NULL);
    
    float push_constants[4];
    // Scale
    push_constants[0] = 2.0f / draw_data->DisplaySize.x;
    push_constants[1] = 2.0f / draw_data->DisplaySize.y;
    // Translation
    push_constants[2] = -1.0f - draw_data->DisplayPos.x * push_constants[0];
    push_constants[3] = -1.0f - draw_data->DisplayPos.y * push_constants[1];
    vkCmdPushConstants(cmd, m_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(push_constants), push_constants);

    VkDeviceSize vertex_buffer_offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &m_VertexBuffer, &vertex_buffer_offset);
    vkCmdBindIndexBuffer(cmd, m_IndexBuffer, 0, VK_INDEX_TYPE_UINT16);

    int vtx_offset = 0;
    int idx_offset = 0;
    ImVec2 display_pos = draw_data->DisplayPos;
    for (int i = 0; i < draw_data->CmdListsCount; ++i)
    {
        const ImDrawList* im_cmd_list = draw_data->CmdLists[i];
        for (int j = 0; j < im_cmd_list->CmdBuffer.Size; ++j)
        {
            const ImDrawCmd* im_cmd = &im_cmd_list->CmdBuffer[j];
            if (im_cmd->UserCallback)
            {
                im_cmd->UserCallback(im_cmd_list, im_cmd);
            }
            else
            {
                VkRect2D scissor;
                scissor.offset.x = static_cast<int32_t>(im_cmd->ClipRect.x - display_pos.x) > 0 ? static_cast<int32_t>(im_cmd->ClipRect.x - display_pos.x) : 0;
                scissor.offset.y = static_cast<int32_t>(im_cmd->ClipRect.y - display_pos.y) > 0 ? static_cast<int32_t>(im_cmd->ClipRect.y - display_pos.y) : 0;
                scissor.extent.width = static_cast<uint32_t>(im_cmd->ClipRect.z - im_cmd->ClipRect.x);
                scissor.extent.height = static_cast<uint32_t>(im_cmd->ClipRect.w - im_cmd->ClipRect.y + 1);
                vkCmdSetScissor(cmd, 0, 1, &scissor);

                vkCmdDrawIndexed(cmd, im_cmd->ElemCount, 1, idx_offset, vtx_offset, 0);
            }
            idx_offset += im_cmd->ElemCount;
        }
        vtx_offset += im_cmd_list->VtxBuffer.Size;
    }

    vkCmdEndRenderPass(cmd);

	VkPopLabel(cmd);
}