#include "App.h"
#include "VkUtil.h"

#ifdef _WIN32
	#define GLFW_EXPOSE_NATIVE_WIN32
#else
	#define GLFW_EXPOSE_NATIVE_X11
#endif
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

void App::ResizeCallback(GLFWwindow* window, int width, int height)
{
	App* app = static_cast<App*>(glfwGetWindowUserPointer(window));
	app->m_Width = static_cast<uint32_t>(width);
	app->m_Height = static_cast<uint32_t>(height);
}
void App::MinimizeCallback(GLFWwindow* window, int minimized)
{
	App* app = static_cast<App*>(glfwGetWindowUserPointer(window));
	app->m_Minimized = minimized == GLFW_TRUE;
}

void App::Initialize(uint32_t width, uint32_t height, const char* title)
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	m_Window = glfwCreateWindow(width, height, title, NULL, NULL);

	m_Width = width;
	m_Height = height;
	m_Minimized = false;
	m_DisplayMode = VK_DISPLAY_MODE_SDR;

	glfwSetWindowUserPointer(m_Window, this);
	glfwSetWindowSizeCallback(m_Window, ResizeCallback);
	glfwSetWindowIconifyCallback(m_Window, MinimizeCallback);

	VkInitializeParams vk_params;
#ifdef _WIN32
	vk_params.WindowHandle = glfwGetWin32Window(m_Window);
	vk_params.DisplayHandle = GetModuleHandle(NULL);
#else
	vk_params.WindowHandle = reinterpret_cast<void*>(glfwGetX11Window(m_Window));
	vk_params.DisplayHandle = glfwGetX11Display();
#endif
	vk_params.BackBufferWidth = width;
	vk_params.BackBufferHeight = height;
	vk_params.DesiredBackBufferCount = 2;
	vk_params.DisplayMode = m_DisplayMode;
	vk_params.EnableValidationLayer = false;
	VkInitialize(vk_params);

	VkUtilCreateRenderPassParams color_render_pass_params;
	color_render_pass_params.ColorAttachmentFormats = { VK_FORMAT_B10G11R11_UFLOAT_PACK32 };
	color_render_pass_params.DepthAttachmentFormat = VK_FORMAT_D32_SFLOAT;
	m_RenderContext.ColorRenderPass = VkUtilCreateRenderPass(color_render_pass_params);

    VkUtilCreateRenderPassParams depth_render_pass_params;
	depth_render_pass_params.ColorAttachmentFormats = { VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R16G16_UNORM };
	depth_render_pass_params.DepthAttachmentFormat = VK_FORMAT_D32_SFLOAT;
	m_RenderContext.DepthRenderPass = VkUtilCreateRenderPass(depth_render_pass_params);

	VkUtilCreateRenderPassParams ui_render_pass_params;
	ui_render_pass_params.ColorAttachmentFormats = { VK_FORMAT_R8G8B8A8_UNORM };
	m_RenderContext.UiRenderPass = VkUtilCreateRenderPass(ui_render_pass_params);

	CreateResolutionDependentResources(width, height);

    VkSamplerCreateInfo nearest_clamp_info = {};
    nearest_clamp_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    nearest_clamp_info.magFilter = VK_FILTER_NEAREST;
    nearest_clamp_info.minFilter = VK_FILTER_NEAREST;
    nearest_clamp_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    nearest_clamp_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    nearest_clamp_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    nearest_clamp_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    nearest_clamp_info.minLod = -FLT_MAX;
    nearest_clamp_info.maxLod = FLT_MAX;
    nearest_clamp_info.maxAnisotropy = 1.0f;
    VK(vkCreateSampler(Vk.Device, &nearest_clamp_info, NULL, &m_RenderContext.NearestClamp));

    VkSamplerCreateInfo nearest_wrap_info = {};
    nearest_wrap_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    nearest_wrap_info.magFilter = VK_FILTER_NEAREST;
    nearest_wrap_info.minFilter = VK_FILTER_NEAREST;
    nearest_wrap_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    nearest_wrap_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    nearest_wrap_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    nearest_wrap_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    nearest_wrap_info.minLod = -FLT_MAX;
    nearest_wrap_info.maxLod = FLT_MAX;
    nearest_wrap_info.maxAnisotropy = 1.0f;
    VK(vkCreateSampler(Vk.Device, &nearest_wrap_info, NULL, &m_RenderContext.NearestWrap));

    VkSamplerCreateInfo linear_clamp_info = {};
    linear_clamp_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    linear_clamp_info.magFilter = VK_FILTER_LINEAR;
    linear_clamp_info.minFilter = VK_FILTER_LINEAR;
    linear_clamp_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    linear_clamp_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    linear_clamp_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    linear_clamp_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    linear_clamp_info.minLod = -FLT_MAX;
    linear_clamp_info.maxLod = FLT_MAX;
    linear_clamp_info.maxAnisotropy = 1.0f;
    VK(vkCreateSampler(Vk.Device, &linear_clamp_info, NULL, &m_RenderContext.LinearClamp));

    VkSamplerCreateInfo linear_wrap_info = {};
    linear_wrap_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    linear_wrap_info.magFilter = VK_FILTER_LINEAR;
    linear_wrap_info.minFilter = VK_FILTER_LINEAR;
    linear_wrap_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    linear_wrap_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    linear_wrap_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    linear_wrap_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    linear_wrap_info.minLod = -FLT_MAX;
    linear_wrap_info.maxLod = FLT_MAX;
    linear_wrap_info.maxAnisotropy = 1.0f;
    VK(vkCreateSampler(Vk.Device, &linear_wrap_info, NULL, &m_RenderContext.LinearWrap));

	VkSamplerCreateInfo aniso_clamp_info = {};
	aniso_clamp_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	aniso_clamp_info.magFilter = VK_FILTER_LINEAR;
	aniso_clamp_info.minFilter = VK_FILTER_LINEAR;
	aniso_clamp_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	aniso_clamp_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	aniso_clamp_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	aniso_clamp_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	aniso_clamp_info.minLod = -FLT_MAX;
	aniso_clamp_info.maxLod = FLT_MAX;
	aniso_clamp_info.anisotropyEnable = VK_TRUE;
	aniso_clamp_info.maxAnisotropy = 8.0f;
	VK(vkCreateSampler(Vk.Device, &aniso_clamp_info, NULL, &m_RenderContext.AnisoClamp));

	VkSamplerCreateInfo aniso_wrap_info = {};
	aniso_wrap_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	aniso_wrap_info.magFilter = VK_FILTER_LINEAR;
	aniso_wrap_info.minFilter = VK_FILTER_LINEAR;
	aniso_wrap_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	aniso_wrap_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	aniso_wrap_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	aniso_wrap_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	aniso_wrap_info.minLod = -FLT_MAX;
	aniso_wrap_info.maxLod = FLT_MAX;
	aniso_wrap_info.anisotropyEnable = VK_TRUE;
	aniso_wrap_info.maxAnisotropy = 8.0f;
	VK(vkCreateSampler(Vk.Device, &aniso_wrap_info, NULL, &m_RenderContext.AnisoWrap));

	m_RenderContext.BlueNoiseTextures.resize(16);
	for (size_t i = 0; i < m_RenderContext.BlueNoiseTextures.size(); ++i)
	{
		std::string filepath = "../Assets/Textures/BlueNoise_" + std::to_string(i) + ".png";
		m_RenderContext.BlueNoiseTextures[i] = VkTextureLoad(filepath.c_str(), false);
	}

	m_RenderContext.CameraCurr.LookAt(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	m_RenderContext.CameraPrev = m_RenderContext.CameraCurr;

	m_RenderContext.SunDirection = glm::vec3(0.0f, 1.0f, 0.15f);

	m_RenderContext.DebugEnable = false;
	m_RenderContext.DebugIndex = 0;

	m_Models[MODEL_SPONZA].Load("../Assets/glTF-Sample-Models/2.0/Sponza/glTF/Sponza.gltf");
	m_Models[MODEL_SPHERES].Load("../Assets/glTF-Sample-Models/2.0/MetalRoughSpheres/glTF/MetalRoughSpheres.gltf");

	m_Models[MODEL_SPHERES].Transform(glm::translate(glm::vec3(32.0f, 4.0f, 0.0f)));

	m_AccelerationStructure.Create(m_RenderContext, MODEL_COUNT, m_Models);

	m_RenderModel.Create(m_RenderContext);
	m_RenderMotion.Create(m_RenderContext);
	m_RenderSSAO.Create(m_RenderContext);
	m_RenderShadows.Create(m_RenderContext);
	m_RenderAtmosphere.Create(m_RenderContext);
	m_RenderPostProcess.Create(m_RenderContext);
	m_RenderImGui.Create(m_RenderContext, m_Window);

	m_RenderModel.SetAmbientLightLUT(m_RenderAtmosphere.m_AmbientLightLUT.ImageView);
	m_RenderModel.SetDirectionalLightLUT(m_RenderAtmosphere.m_DirectionalLightLUT.ImageView);
}

void App::Terminate()
{
	vkDeviceWaitIdle(Vk.Device);

	m_RenderModel.Destroy();
	m_RenderMotion.Destroy();
	m_RenderSSAO.Destroy();
	m_RenderShadows.Destroy();
	m_RenderAtmosphere.Destroy();
	m_RenderPostProcess.Destroy();
	m_RenderImGui.Destroy();

	m_AccelerationStructure.Destroy();

	for (uint32_t i = 0; i < MODEL_COUNT; ++i)
	{
		m_Models[i].Destroy();
	}

	for (const VkTexture& texture : m_RenderContext.BlueNoiseTextures)
	{
		VkTextureDestroy(texture);
	}

    vkDestroySampler(Vk.Device, m_RenderContext.NearestClamp, NULL);
    vkDestroySampler(Vk.Device, m_RenderContext.NearestWrap, NULL);
    vkDestroySampler(Vk.Device, m_RenderContext.LinearClamp, NULL);
    vkDestroySampler(Vk.Device, m_RenderContext.LinearWrap, NULL);
	vkDestroySampler(Vk.Device, m_RenderContext.AnisoClamp, NULL);
	vkDestroySampler(Vk.Device, m_RenderContext.AnisoWrap, NULL);

	DestroyResolutionDependentResources();

	vkDestroyRenderPass(Vk.Device, m_RenderContext.ColorRenderPass, NULL);
    vkDestroyRenderPass(Vk.Device, m_RenderContext.DepthRenderPass, NULL);
	vkDestroyRenderPass(Vk.Device, m_RenderContext.UiRenderPass, NULL);

	VkTerminate();

	glfwDestroyWindow(m_Window);
	glfwTerminate();
}

void App::CreateResolutionDependentResources(uint32_t width, uint32_t height)
{
    VkTextureCreateParams color_texture_params;
    color_texture_params.Type = VK_IMAGE_TYPE_2D;
    color_texture_params.ViewType = VK_IMAGE_VIEW_TYPE_2D;
    color_texture_params.Width = width;
    color_texture_params.Height = height;
    color_texture_params.Format = VK_FORMAT_B10G11R11_UFLOAT_PACK32;
    color_texture_params.Usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    color_texture_params.InitialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	m_RenderContext.ColorTexture = VkTextureCreate(color_texture_params);

    VkTextureCreateParams depth_texture_params;
    depth_texture_params.Type = VK_IMAGE_TYPE_2D;
    depth_texture_params.ViewType = VK_IMAGE_VIEW_TYPE_2D;
    depth_texture_params.Width = width;
    depth_texture_params.Height = height;
    depth_texture_params.Format = VK_FORMAT_D32_SFLOAT;
    depth_texture_params.Usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    depth_texture_params.InitialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	m_RenderContext.DepthTexture = VkTextureCreate(depth_texture_params);

	VkTextureCreateParams ui_texture_params;
	ui_texture_params.Type = VK_IMAGE_TYPE_2D;
	ui_texture_params.ViewType = VK_IMAGE_VIEW_TYPE_2D;
	ui_texture_params.Width = width;
	ui_texture_params.Height = height;
	ui_texture_params.Format = VK_FORMAT_R8G8B8A8_UNORM;
	ui_texture_params.Usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	ui_texture_params.InitialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	m_RenderContext.UiTexture = VkTextureCreate(ui_texture_params);

	VkTextureCreateParams normal_texture_params;
	normal_texture_params.Type = VK_IMAGE_TYPE_2D;
	normal_texture_params.ViewType = VK_IMAGE_VIEW_TYPE_2D;
	normal_texture_params.Width = width;
	normal_texture_params.Height = height;
	normal_texture_params.Format = VK_FORMAT_R8G8B8A8_UNORM;
	normal_texture_params.Usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	normal_texture_params.InitialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	m_RenderContext.NormalTexture = VkTextureCreate(normal_texture_params);

	VkTextureCreateParams motion_texture_params;
	motion_texture_params.Type = VK_IMAGE_TYPE_2D;
	motion_texture_params.ViewType = VK_IMAGE_VIEW_TYPE_2D;
	motion_texture_params.Width = width;
	motion_texture_params.Height = height;
	motion_texture_params.Format = VK_FORMAT_R16G16B16A16_SFLOAT;
	motion_texture_params.Usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	motion_texture_params.InitialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	m_RenderContext.MotionTexture = VkTextureCreate(motion_texture_params);

	VkTextureCreateParams ambient_occlusion_texture_params;
	ambient_occlusion_texture_params.Type = VK_IMAGE_TYPE_2D;
	ambient_occlusion_texture_params.ViewType = VK_IMAGE_VIEW_TYPE_2D;
	ambient_occlusion_texture_params.Width = width;
	ambient_occlusion_texture_params.Height = height;
	ambient_occlusion_texture_params.Format = VK_FORMAT_R8_UNORM;
	ambient_occlusion_texture_params.Usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	ambient_occlusion_texture_params.InitialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	m_RenderContext.AmbientOcclusionTexture = VkTextureCreate(ambient_occlusion_texture_params);

	const size_t no_shadow_size = width * height * sizeof(uint16_t);
	std::vector<uint8_t> no_shadow(no_shadow_size);
	memset(no_shadow.data(), 0xff, no_shadow_size);
	VkTextureCreateParams shadow_texture_params;
	shadow_texture_params.Type = VK_IMAGE_TYPE_2D;
	shadow_texture_params.ViewType = VK_IMAGE_VIEW_TYPE_2D;
	shadow_texture_params.Width = width;
	shadow_texture_params.Height = height;
	shadow_texture_params.Format = VK_FORMAT_R16_UNORM;
	shadow_texture_params.Usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	shadow_texture_params.InitialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	shadow_texture_params.Data = no_shadow.data();
	shadow_texture_params.DataSize = no_shadow_size;
	m_RenderContext.ShadowTexture = VkTextureCreate(shadow_texture_params);

	VkTextureCreateParams linear_depth_texture_params;
	linear_depth_texture_params.Type = VK_IMAGE_TYPE_2D;
	linear_depth_texture_params.ViewType = VK_IMAGE_VIEW_TYPE_2D;
	linear_depth_texture_params.Width = width;
	linear_depth_texture_params.Height = height;
	linear_depth_texture_params.Format = VK_FORMAT_R16G16_UNORM;
	linear_depth_texture_params.Usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	linear_depth_texture_params.InitialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	m_RenderContext.LinearDepthTextures[0] = VkTextureCreate(linear_depth_texture_params);
	m_RenderContext.LinearDepthTextures[1] = VkTextureCreate(linear_depth_texture_params);

	VkUtilCreateFramebufferParams color_framebuffer_params;
	color_framebuffer_params.RenderPass = m_RenderContext.ColorRenderPass;
	color_framebuffer_params.ColorAttachments = { m_RenderContext.ColorTexture.ImageView };
	color_framebuffer_params.DepthAttachment = m_RenderContext.DepthTexture.ImageView;
	color_framebuffer_params.Width = width;
	color_framebuffer_params.Height = height;
	m_RenderContext.ColorFramebuffer = VkUtilCreateFramebuffer(color_framebuffer_params);

	for (uint32_t i = 0; i < 2; ++i)
	{
		VkUtilCreateFramebufferParams depth_framebuffer_params;
		depth_framebuffer_params.RenderPass = m_RenderContext.DepthRenderPass;
		depth_framebuffer_params.ColorAttachments = { m_RenderContext.NormalTexture.ImageView, m_RenderContext.LinearDepthTextures[i].ImageView };
		depth_framebuffer_params.DepthAttachment = m_RenderContext.DepthTexture.ImageView;
		depth_framebuffer_params.Width = width;
		depth_framebuffer_params.Height = height;
		m_RenderContext.DepthFramebuffers[i] = VkUtilCreateFramebuffer(depth_framebuffer_params);
	}

	VkUtilCreateFramebufferParams ui_framebuffer_params;
	ui_framebuffer_params.RenderPass = m_RenderContext.UiRenderPass;
	ui_framebuffer_params.ColorAttachments = { m_RenderContext.UiTexture.ImageView };
	ui_framebuffer_params.Width = width;
	ui_framebuffer_params.Height = height;
	m_RenderContext.UiFramebuffer = VkUtilCreateFramebuffer(ui_framebuffer_params);

	VkUtilCreateRenderPassParams back_buffer_render_pass_params;
	back_buffer_render_pass_params.ColorAttachmentFormats = { Vk.SwapchainSurfaceFormat.format };
	m_RenderContext.BackBufferRenderPass = VkUtilCreateRenderPass(back_buffer_render_pass_params);

	m_RenderContext.BackBufferFramebuffers.resize(Vk.SwapchainImageCount);
    for (uint32_t i = 0; i < Vk.SwapchainImageCount; ++i)
    {
        VkUtilCreateFramebufferParams back_buffer_framebuffer_params;
        back_buffer_framebuffer_params.RenderPass = m_RenderContext.BackBufferRenderPass;
        back_buffer_framebuffer_params.ColorAttachments = { Vk.SwapchainImageViews[i] };
        back_buffer_framebuffer_params.Width = width;
        back_buffer_framebuffer_params.Height = height;
		m_RenderContext.BackBufferFramebuffers[i] = VkUtilCreateFramebuffer(back_buffer_framebuffer_params);
    }

	m_RenderContext.Width = width;
	m_RenderContext.Height = height;

	m_RenderContext.FrameCounter = 0;

	m_RenderContext.CameraCurr.Perspective(glm::radians(75.0f), static_cast<float>(width) / static_cast<float>(height), 0.1f, 100.0f);
	m_RenderContext.CameraPrev = m_RenderContext.CameraCurr;
}

void App::DestroyResolutionDependentResources()
{
	vkDestroyFramebuffer(Vk.Device, m_RenderContext.ColorFramebuffer, NULL);
	vkDestroyFramebuffer(Vk.Device, m_RenderContext.DepthFramebuffers[0], NULL);
	vkDestroyFramebuffer(Vk.Device, m_RenderContext.DepthFramebuffers[1], NULL);
	vkDestroyFramebuffer(Vk.Device, m_RenderContext.UiFramebuffer, NULL);
	for (VkFramebuffer framebuffer : m_RenderContext.BackBufferFramebuffers)
	{
		vkDestroyFramebuffer(Vk.Device, framebuffer, NULL);
	}

	vkDestroyRenderPass(Vk.Device, m_RenderContext.BackBufferRenderPass, NULL);

	VkTextureDestroy(m_RenderContext.ColorTexture);
	VkTextureDestroy(m_RenderContext.DepthTexture);
	VkTextureDestroy(m_RenderContext.UiTexture);
	VkTextureDestroy(m_RenderContext.NormalTexture);
	VkTextureDestroy(m_RenderContext.MotionTexture);
	VkTextureDestroy(m_RenderContext.AmbientOcclusionTexture);
	VkTextureDestroy(m_RenderContext.ShadowTexture);
	VkTextureDestroy(m_RenderContext.LinearDepthTextures[0]);
	VkTextureDestroy(m_RenderContext.LinearDepthTextures[1]);
}

void App::Run()
{
	CameraController controller;

	while (!glfwWindowShouldClose(m_Window))
	{
		glfwPollEvents();

		if (m_Minimized || m_Width == 0 || m_Height == 0)
			continue;

		if (m_Width != m_RenderContext.Width || m_Height != m_RenderContext.Height || m_DisplayMode != Vk.DisplayMode)
		{
			vkDeviceWaitIdle(Vk.Device);

			VkResize(m_Width, m_Height, m_DisplayMode);

			DestroyResolutionDependentResources();
			CreateResolutionDependentResources(m_Width, m_Height);

			m_RenderSSAO.RecreateResolutionDependentResources(m_RenderContext);
			m_RenderShadows.RecreateResolutionDependentResources(m_RenderContext);
			m_RenderPostProcess.RecreateResolutionDependentResources(m_RenderContext);

			m_RenderPostProcess.RecreatePipelines(m_RenderContext);
		}

		if (glfwGetKey(m_Window, GLFW_KEY_F5) == GLFW_PRESS)
		{
			vkDeviceWaitIdle(Vk.Device);

#ifdef _WIN32
			system("\"\"../Tools/ShaderCompiler/Bin/ShaderCompiler.exe\"\" ../Source/Shaders/ ../Assets/Shaders/");
#else
			system("../Tools/ShaderCompiler/Bin/ShaderCompiler ../Source/Shaders/ ../Assets/Shaders/");
#endif

			m_RenderModel.RecreatePipelines(m_RenderContext);
			m_RenderMotion.RecreatePipelines(m_RenderContext);
			m_RenderSSAO.RecreatePipelines(m_RenderContext);
			m_RenderShadows.RecreatePipelines(m_RenderContext);
			//m_RenderAtmosphere.RecreatePipelines(m_RenderContext);
			m_RenderPostProcess.RecreatePipelines(m_RenderContext);
		}

		{
			static double last_time = glfwGetTime();
			double time = glfwGetTime();
			float dt = static_cast<float>(time - last_time);
			last_time = time;

			m_RenderContext.CameraPrev = m_RenderContext.CameraCurr;
			controller.Update(m_RenderContext.CameraCurr, m_Window, dt);

			m_RenderPostProcess.Jitter(m_RenderContext);

			m_RenderImGui.Update(m_Window);
		}

		{
			ImGui::StyleColorsDark();
			ImGui::NewFrame();

			ImGui::Begin("Settings");
			if (ImGui::CollapsingHeader("Lighting"))
			{
				ImGui::SliderFloat3("Light Dir", &m_RenderContext.SunDirection.x, -1.0f, 1.0f);
				ImGui::InputFloat("Amb Light", &m_RenderModel.m_AmbientLightIntensity);
				ImGui::InputFloat("Dir Light", &m_RenderModel.m_DirectionalLightIntensity);
				ImGui::InputFloat("Sky Light", &m_RenderAtmosphere.m_SkyLightIntensity);

				m_RenderContext.SunDirection = glm::normalize(m_RenderContext.SunDirection);
			}
			if (ImGui::CollapsingHeader("SSAO"))
			{
				ImGui::SliderFloat("Radius", &m_RenderSSAO.m_Radius, 0.0f, 5.0f);
				ImGui::SliderFloat("Bias", &m_RenderSSAO.m_Bias, 0.0f, 1.0f);
				ImGui::SliderFloat("Intensity", &m_RenderSSAO.m_Intensity, 0.0f, 10.0f);

				ImGui::Checkbox("Blur", &m_RenderSSAO.m_Blur);
			}
			if (ImGui::CollapsingHeader("Ray Traced Shadows"))
			{
				ImGui::Checkbox("Enable##Shadows", &m_RenderShadows.m_Enable);
				ImGui::Checkbox("Alpha Test", &m_RenderShadows.m_AlphaTest);
				ImGui::SliderFloat("Cone Angle", &m_RenderShadows.m_ConeAngle, 0.0f, 15.0f);

				ImGui::Checkbox("Reproject", &m_RenderShadows.m_Reproject);
				ImGui::SliderFloat("Reproject Alpha Shadow", &m_RenderShadows.m_ReprojectAlphaShadow, 0.0f, 1.0f);
				ImGui::SliderFloat("Reproject Alpha Moments", &m_RenderShadows.m_ReprojectAlphaMoments, 0.0f, 1.0f);
				
				ImGui::Checkbox("Filter", &m_RenderShadows.m_Filter);
				ImGui::SliderInt("Filter Iterations", &m_RenderShadows.m_FilterIterations, 0, 6);
				ImGui::SliderFloat("Filter Phi Variance", &m_RenderShadows.m_FilterPhiVariance, 0.0f, 20.0f);
			}
			if (ImGui::CollapsingHeader("Post Process"))
			{
				ImGui::Checkbox("Temporal AA", &m_RenderPostProcess.m_TemporalAAEnable);
				ImGui::SliderFloat("Exposure", &m_RenderPostProcess.m_Exposure, -8.0f, 4.0f);
				ImGui::SliderFloat("Saturation", &m_RenderPostProcess.m_Saturation, 0.5f, 1.5f);
				ImGui::SliderFloat("Contrast", &m_RenderPostProcess.m_Contrast, 0.5f, 1.5f);
				ImGui::SliderFloat("Gamma", &m_RenderPostProcess.m_Gamma, 0.5f, 1.5f);
				ImGui::SliderFloat("Gamut Expansion", &m_RenderPostProcess.m_GamutExpansion, 0.0f, 4.0f);
				ImGui::Checkbox("View Luxo Double Checker", &m_RenderPostProcess.m_ViewLuxoDoubleChecker);
			}
			if (ImGui::CollapsingHeader("Display"))
			{
				const char* display_mode_names[] = { "SDR", "HDR10", "scRGB" };
				if (ImGui::BeginCombo("Display Mode", display_mode_names[m_DisplayMode]))
				{
					for (uint32_t display_mode = 0; display_mode < VK_DISPLAY_MODE_COUNT; ++display_mode)
					{
						if (Vk.IsDisplayModeSupported[display_mode])
						{
							bool is_selected = m_DisplayMode == display_mode;
							if (ImGui::Selectable(display_mode_names[display_mode], is_selected))
								m_DisplayMode = static_cast<VkDisplayMode>(display_mode);
							if (is_selected)
								ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndCombo();
				}

				if (Vk.DisplayMode == VK_DISPLAY_MODE_HDR10 || Vk.DisplayMode == VK_DISPLAY_MODE_SCRGB)
				{
					const char* display_mapping_names[] = { "ACES ODT", "BT2390 EETF", "SDR Emulation", "Luminance Visualization Scene", "Luminance Visualization Display" };
					if (ImGui::BeginCombo("Display Mapping", display_mapping_names[m_RenderPostProcess.m_DisplayMapping]))
					{
						for (uint32_t display_mapping = 0; display_mapping < RenderPostProcess::DISPLAY_MAPPING_COUNT; ++display_mapping)
						{
							bool is_selected = m_RenderPostProcess.m_DisplayMapping == display_mapping;
							if (ImGui::Selectable(display_mapping_names[display_mapping], is_selected))
								m_RenderPostProcess.m_DisplayMapping = static_cast<RenderPostProcess::DisplayMapping>(display_mapping);
							if (is_selected)
								ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
					}
					if (ImGui::BeginCombo("Display Mapping Aux", display_mapping_names[m_RenderPostProcess.m_DisplayMappingAux]))
					{
						for (uint32_t display_mapping = 0; display_mapping < RenderPostProcess::DISPLAY_MAPPING_COUNT; ++display_mapping)
						{
							bool is_selected = m_RenderPostProcess.m_DisplayMappingAux == display_mapping;
							if (ImGui::Selectable(display_mapping_names[display_mapping], is_selected))
								m_RenderPostProcess.m_DisplayMappingAux = static_cast<RenderPostProcess::DisplayMapping>(display_mapping);
							if (is_selected)
								ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
					}
					ImGui::Checkbox("Display Mapping Split Screen", &m_RenderPostProcess.m_DisplayMappingSplitScreen);
					ImGui::SliderFloat("Display Mapping Split Screen Offset", &m_RenderPostProcess.m_DisplayMappingSplitScreenOffset, -1.0f, 1.0f);

					ImGui::SliderFloat("Display Min Point", &m_RenderPostProcess.m_HdrDisplayLuminanceMin, 0.001f, 0.1f, "%.3f");
					ImGui::SliderFloat("Display Max Point", &m_RenderPostProcess.m_HdrDisplayLuminanceMax, 400.0f, 1000.0f, "%.0f");

					ImGui::SliderFloat("SDR White Level", &m_RenderPostProcess.m_SdrWhiteLevel, 100.0f, 400.0f, "%.0f");

					ImGui::SliderFloat("ACES Mid Point", &m_RenderPostProcess.m_ACESMidPoint, 1.0f, 100.0f, "%.1f");
					ImGui::SliderFloat("BT2390 Mid Point", &m_RenderPostProcess.m_BT2390MidPoint, 1.0f, 100.0f, "%.1f");
				}
			}
			if (ImGui::CollapsingHeader("Debug"))
			{
				const char* debug_names[] =
				{
					"Albedo",
					"Normal",
					"Metallic",
					"Roughness",
					"Microfacet Distribution",
					"Geometric Occlusion",
					"Specular Reflection",
					"Shadow",
					"Ambient Occlusion"
				};
				const int32_t debug_name_count = static_cast<int32_t>(sizeof(debug_names) / sizeof(*debug_names));

				ImGui::Checkbox("Enable##Debug", &m_RenderContext.DebugEnable);
				ImGui::SliderInt("Index", &m_RenderContext.DebugIndex, 0, debug_name_count - 1);
				ImGui::LabelText("Name", debug_names[m_RenderContext.DebugIndex]);
			}
			ImGui::End();

			ImGui::Begin("Performance (ms)");
			{
				ImGui::Text("Models Depth:              %.3f", VkGetLabel("Models Depth"));
				ImGui::Text("SSAO Generate:             %.3f", VkGetLabel("SSAO Generate"));
				ImGui::Text("SSAO Blur:                 %.3f", VkGetLabel("SSAO Blur"));
				ImGui::Text("Motion Generate:           %.3f", VkGetLabel("Motion Generate"));
				ImGui::Text("Shadows Trace Rays:        %.3f", VkGetLabel("Shadows Trace Rays"));
				ImGui::Text("Shadows Reproject:         %.3f", VkGetLabel("Shadows Reproject"));
				ImGui::Text("Shadows Filter:            %.3f", VkGetLabel("Shadows Filter"));
				ImGui::Text("Models Color:              %.3f", VkGetLabel("Models Color"));
				ImGui::Text("Atmosphere Sky:            %.3f", VkGetLabel("Atmosphere Sky"));
				ImGui::Text("Post Process TAA:          %.3f", VkGetLabel("Post Process TAA"));
				ImGui::Text("Post Process Tone Mapping: %.3f", VkGetLabel("Post Process Tone Mapping"));
				ImGui::Text("ImGui:                     %.3f", VkGetLabel("ImGui"));
			}
			ImGui::End();

			ImGui::Render();
		}

		{
			VkCommandBuffer cmd = VkBeginFrame();

			// Depth pass
			m_RenderModel.DrawDepth(m_RenderContext, cmd, MODEL_COUNT, m_Models);

			// Generate motion vectors and linear depth
			m_RenderMotion.Generate(m_RenderContext, cmd);

			// Generate SSAO
			m_RenderSSAO.Generate(m_RenderContext, cmd);

			// Ray trace shadows
			m_RenderShadows.RayTrace(m_RenderContext, cmd, m_AccelerationStructure);

			// Color pass
			m_RenderModel.DrawColor(m_RenderContext, cmd, MODEL_COUNT, m_Models);

			// Draw sky
			m_RenderAtmosphere.DrawSky(m_RenderContext, cmd);

			// Draw ImGui
			m_RenderImGui.Draw(m_RenderContext, cmd);

			// Post effects
			m_RenderPostProcess.Draw(m_RenderContext, cmd);

			VkEndFrame();
		}

		++m_RenderContext.FrameCounter;
	}
}
