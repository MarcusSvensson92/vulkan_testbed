#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 InTexCoord;
layout(location = 1) in vec3 InNormal;

layout(location = 0) out vec4 OutNormal;
layout(location = 1) out vec2 OutLinearDepth;

layout(binding = 0) uniform Constants
{
	mat4	World;
	mat4	WorldViewProjection;
	float	DepthParam;
};
layout(binding = 1) uniform sampler2D BaseColor;

void main()
{
	float alpha = texture(BaseColor, InTexCoord).a;
	if (alpha < 0.5)
	{
		discard;
	}

	vec3 normal = normalize(InNormal);
	OutNormal = vec4(normal * 0.5 + 0.5, 0.0);

	float lin_depth = 1.0 / (gl_FragCoord.z * DepthParam + 1.0);
	OutLinearDepth = vec2(lin_depth, max(abs(dFdx(lin_depth)), abs(dFdy(lin_depth))));
}