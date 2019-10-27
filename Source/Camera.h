#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include <GLFW/glfw3.h>

class Camera
{
public:
    glm::mat4   m_View                  = glm::identity<glm::mat4>();
    glm::mat4   m_Projection            = glm::identity<glm::mat4>();
    glm::mat4   m_ProjectionNoJitter    = glm::identity<glm::mat4>();

    glm::vec3   m_Right                 = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3   m_Up                    = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3   m_Look                  = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3   m_Position              = glm::vec3(0.0f, 0.0f, 0.0f);

	float		m_FovY					= 0.0f;
	float		m_NearZ					= 0.0f;
	float		m_FarZ					= 0.0f;

	glm::vec2	m_Jitter				= glm::vec2(0.0f, 0.0f);
	glm::vec2	m_JitterDelta			= glm::vec2(0.0f, 0.0f);

    void        LookAt(const glm::vec3& position, const glm::vec3& target, const glm::vec3& up);
    void        Perspective(float fov_y, float aspect_ratio, float near_z, float far_z);
    void        Jitter(const glm::vec2& jitter);

    void        Pitch(float angle);
    void        Yaw(float angle);
    void        Move(float distance);
    void        Strafe(float distance);

    void        UpdateView();
};

class CameraController
{
public:
    double      m_LastCursorX		= 0.0;
    double      m_LastCursorY		= 0.0;

    float       m_LastMoveSpeed		= 0.0f;
    float       m_LastStrafeSpeed	= 0.0f;

    void        Update(Camera& camera, GLFWwindow* window, float dt);
};