#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec3 OutRayDirection;

layout(binding = 0) uniform Constants
{
    mat4	InvViewProjZeroTranslation;
	vec3	LightDirection;
	float	LightIntensity;
	float	TimeOfDay;
};

void main()
{
	gl_Position = vec4(((gl_VertexIndex << 1) & 2) * 2.0 - 1.0, (gl_VertexIndex & 2) * -2.0 + 1.0, 0.0, 1.0);
	vec4 world_dir = InvViewProjZeroTranslation * gl_Position;
    OutRayDirection = world_dir.xyz / world_dir.w;
}