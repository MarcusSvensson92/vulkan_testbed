#pragma once

#include "RenderContext.h"

class RenderMotion
{
public:
    VkDescriptorSetLayout   m_GenerateDescriptorSetLayout      = VK_NULL_HANDLE;
    VkPipelineLayout        m_GeneratePipelineLayout           = VK_NULL_HANDLE;
    VkPipeline              m_GeneratePipeline                 = VK_NULL_HANDLE;

    void                    Create(const RenderContext& rc);
    void                    Destroy();

	void					RecreatePipelines(const RenderContext& rc);

    void                    Generate(const RenderContext& rc, VkCommandBuffer cmd);

private:
	void					CreatePipelines(const RenderContext& rc);
	void					DestroyPipelines();
};
