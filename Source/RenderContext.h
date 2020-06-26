#pragma once

#include "Vk.h"
#include "VkTexture.h"
#include "Camera.h"

struct RenderContext
{
	VkTexture                   ColorTexture;
	VkTexture                   DepthTexture;
	VkTexture					UiTexture;
	VkTexture					NormalTexture;
	VkTexture					MotionTexture;
	VkTexture					AmbientOcclusionTexture;
	VkTexture                   ShadowTexture;
	VkTexture					LinearDepthTextures[2];

    VkRenderPass                ColorRenderPass;
	VkRenderPass                DepthRenderPass;
	VkRenderPass				UiRenderPass;
    VkRenderPass                BackBufferRenderPass;

    VkFramebuffer               ColorFramebuffer;
	VkFramebuffer               DepthFramebuffers[2];
	VkFramebuffer				UiFramebuffer;
	std::vector<VkFramebuffer>  BackBufferFramebuffers;

    VkSampler                   NearestClamp;
    VkSampler                   NearestWrap;
    VkSampler                   LinearClamp;
    VkSampler                   LinearWrap;
	VkSampler                   AnisoClamp;
	VkSampler                   AnisoWrap;

	std::vector<VkTexture>		BlueNoiseTextures;

    uint32_t                    Width;
    uint32_t                    Height;

    uint32_t                    FrameCounter;

    Camera                      CameraCurr;
	Camera						CameraPrev;

    glm::vec3                   SunDirection;

	bool						DebugEnable;
	int32_t						DebugIndex;
};
