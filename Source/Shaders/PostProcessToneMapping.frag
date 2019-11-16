#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 OutColor;

layout(binding = 0) uniform Constants
{
    float	Exposure;
};
layout(binding = 1) uniform sampler2D HdrTexture;

// https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
vec3 ACES(vec3 x)
{
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main()
{
    OutColor.rgb = texelFetch(HdrTexture, ivec2(gl_FragCoord.xy), 0).rgb;

	// Exposure
	OutColor.rgb *= Exposure;

	// ACES tone mapping
	OutColor.rgb = ACES(OutColor.rgb * 0.8);

	// Gamma 2.2
	OutColor.rgb = pow(OutColor.rgb, vec3(1.0 / 2.2));

	// Set alpha to 1
	OutColor.a = 1.0;
}