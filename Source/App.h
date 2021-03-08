#pragma once

#include "RenderContext.h"

#include "RenderModel.h"
#include "RenderSSAO.h"
#include "RenderMotion.h"
#include "RenderRayTracedAO.h"
#include "RenderRayTracedShadows.h"
#include "RenderAtmosphere.h"
#include "RenderPostProcess.h"
#include "RenderImGui.h"

#include "GltfModel.h"

#include "AccelerationStructure.h"

struct GLFWwindow;

class App
{
public:
	GLFWwindow*				m_Window;
	uint32_t				m_Width;
	uint32_t				m_Height;
	bool					m_Minimized;
	VkDisplayMode			m_DisplayMode;

	RenderContext			m_RenderContext;

	RenderModel				m_RenderModel;
	RenderMotion			m_RenderMotion;
	RenderSSAO				m_RenderSSAO;
	RenderRayTracedAO		m_RenderAO;
	RenderRayTracedShadows	m_RenderShadows;
	RenderAtmosphere		m_RenderAtmosphere;
	RenderPostProcess		m_RenderPostProcess;
	RenderImGui				m_RenderImGui;

	enum : uint32_t
	{
		MODEL_SPONZA = 0,
		MODEL_SPHERES,
		MODEL_COUNT
	};
	GltfModel				m_Models[MODEL_COUNT];

	AccelerationStructure	m_AccelerationStructure;

	void                    Initialize(uint32_t width, uint32_t height, const char* title);
	void                    Terminate();

	void					Run();

	static void				ResizeCallback(GLFWwindow* window, int width, int height);
	static void				MinimizeCallback(GLFWwindow* window, int minimized);

private:
	void					CreateResolutionDependentResources(uint32_t width, uint32_t height);
	void					DestroyResolutionDependentResources();
};
