#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 InPosition;
layout(location = 1) in vec2 InTexCoord;
layout(location = 2) in vec3 InNormal;
layout(location = 3) in vec4 InTangent;

layout(location = 0) out vec3 OutWorldPos;
layout(location = 1) out vec2 OutTexCoord;
layout(location = 2) out vec3 OutNormal;
layout(location = 3) out vec3 OutTangent;
layout(location = 4) out vec3 OutBitangent;

layout(binding = 0) uniform Constants
{
	mat4    World;
	mat4	WorldViewProjection;
};

void main()
{
	gl_Position = WorldViewProjection * vec4(InPosition, 1.0);
	OutWorldPos = (World * vec4(InPosition, 1.0)).xyz;
    OutTexCoord = InTexCoord;
	OutNormal = normalize(mat3(World) * InNormal);
	OutTangent = normalize(mat3(World) * InTangent.xyz);
	OutBitangent = normalize(cross(OutNormal, OutTangent) * InTangent.w);
}