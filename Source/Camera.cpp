#include "Camera.h"

void Camera::LookAt(const glm::vec3& position, const glm::vec3& target, const glm::vec3& up)
{
    m_Position = position;
    m_Look = glm::normalize(target - position);
    m_Right = glm::normalize(glm::cross(m_Look, up));
    m_Up = glm::cross(m_Right, m_Look);
    UpdateView();
}
void Camera::Perspective(float fov_y, float aspect_ratio, float near_z, float far_z)
{
    m_ProjectionNoJitter = glm::perspective(fov_y, aspect_ratio, far_z, near_z);
    m_Projection = m_ProjectionNoJitter;
	m_FovY = fov_y;
	m_NearZ = near_z;
	m_FarZ = far_z;
}
void Camera::Jitter(const glm::vec2& jitter)
{
	m_JitterDelta = m_Jitter - jitter;
	m_Jitter = jitter;
    glm::mat4 jitter_mat = glm::translate(glm::vec3(jitter * -2.0f, 0.0f));
    m_Projection = jitter_mat * m_ProjectionNoJitter;
}

void Camera::UpdateView()
{
    m_Look = glm::normalize(m_Look);
    m_Up = glm::normalize(glm::cross(m_Look, m_Right));
    m_Right = glm::cross(m_Up, m_Look);

    m_View[0][0] = m_Right.x;
    m_View[1][0] = m_Right.y;
    m_View[2][0] = m_Right.z;
    m_View[0][1] = m_Up.x;
    m_View[1][1] = m_Up.y;
    m_View[2][1] = m_Up.z;
    m_View[0][2] = -m_Look.x;
    m_View[1][2] = -m_Look.y;
    m_View[2][2] = -m_Look.z;
    m_View[3][0] = -glm::dot(m_Right, m_Position);
    m_View[3][1] = -glm::dot(m_Up, m_Position);
    m_View[3][2] = glm::dot(m_Look, m_Position);
}

void Camera::Pitch(float angle)
{
    glm::mat3 rotation = glm::mat3(glm::rotate(angle, m_Right));
    m_Up = rotation * m_Up;
    m_Look = rotation * m_Look;
}
void Camera::Yaw(float angle)
{
    glm::mat3 rotation = glm::mat3(glm::rotate(angle, glm::vec3(0.f, 1.f, 0.f)));
    m_Right = rotation * m_Right;
    m_Up = rotation * m_Up;
    m_Look = rotation * m_Look;
}
void Camera::Move(float distance)
{
    m_Position += distance * m_Look;
}
void Camera::Strafe(float distance)
{
    m_Position += distance * m_Right;
}

static void SmoothSpeed(float& prev_speed, float& curr_speed, float dt)
{
	float t = dt * 20.0f;			// Acceleration-ish
	t = t < 0.0f ? 0.0f : t;		// Clamp to 0
	t = t > 1.0f ? 1.0f : t;		// Clamp to 1
	t = t * t * (3.0f - 2.0f * t);	// Smoothstep
	curr_speed = prev_speed + (curr_speed - prev_speed) * t;
	prev_speed = curr_speed;
}
void CameraController::Update(Camera& camera, GLFWwindow* window, float dt)
{
    double cursor_x, cursor_y;
    glfwGetCursorPos(window, &cursor_x, &cursor_y);
    float cursor_dx = static_cast<float>(cursor_x - m_LastCursorX);
    float cursor_dy = static_cast<float>(cursor_y - m_LastCursorY);
    m_LastCursorX = cursor_x;
    m_LastCursorY = cursor_y;

    const float cursor_speed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS ? -0.005f : 0.f;
    float pitch = cursor_speed * cursor_dy;
    float yaw = cursor_speed * cursor_dx;

	float move_speed =
		(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ? 16.0f : 2.0f) *
		((glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS ?  1.0f : 0.0f) +
		 (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS ? -1.0f : 0.0f));
	float strafe_speed =
		(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ? 16.0f : 2.0f) *
		((glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS ?  1.0f : 0.0f) +
		 (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS ? -1.0f : 0.0f));

	SmoothSpeed(m_LastMoveSpeed, move_speed, dt);
	SmoothSpeed(m_LastStrafeSpeed, strafe_speed, dt);

	float move = move_speed * dt;
	float strafe = strafe_speed * dt;

    camera.Pitch(pitch);
    camera.Yaw(yaw);
    camera.Move(move);
    camera.Strafe(strafe);
    camera.UpdateView();
}