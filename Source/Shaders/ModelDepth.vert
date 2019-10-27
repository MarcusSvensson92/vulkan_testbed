#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 InPosition;
layout(location = 1) in vec2 InTexCoord;
layout(location = 2) in vec3 InNormal;

layout(location = 0) out vec2 OutTexCoord;
layout(location = 1) out vec3 OutNormal;

layout(binding = 0) uniform Constants
{
	mat4	World;
	mat4	WorldViewProjection;
	float	ObjectIndex;
};

void main()
{
	gl_Position = WorldViewProjection * vec4(InPosition, 1.0);
	OutTexCoord = InTexCoord;
	OutNormal = normalize(mat3(World) * InNormal);
}