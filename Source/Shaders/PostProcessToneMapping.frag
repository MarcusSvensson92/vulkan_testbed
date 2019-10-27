#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 OutColor;

layout(binding = 0) uniform Constants
{
    float	Exposure;
};
layout(binding = 1) uniform sampler2D HdrTexture;

vec3 Filmic(vec3 x)
{
	const float a = 0.15;
	const float b = 0.50;
	const float c = 0.10;
	const float d = 0.20;
	const float e = 0.02;
	const float f = 0.30;
	return ((x * (a * x + c * b) + d * e) / (x * (a * x + b) + d * f)) - e / f;
}

void main()
{
    OutColor.rgb = texelFetch(HdrTexture, ivec2(gl_FragCoord.xy), 0).rgb;

	// Filmic tonemapping
	const float linear_white = 11.2;
	OutColor.rgb = Filmic(OutColor.rgb * Exposure) / Filmic(vec3(linear_white));

	// Gamma 2.2
	OutColor.rgb = pow(OutColor.rgb, vec3(1.0 / 2.2));

	// Set alpha to 1
	OutColor.a = 1.0;
}