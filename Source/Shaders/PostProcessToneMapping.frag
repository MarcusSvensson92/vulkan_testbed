#version 450
#extension GL_GOOGLE_include_directive : require

#include "ACES.glsl"

layout(location = 0) out vec4 OutColor;

layout(binding = 0) uniform Constants
{
    float	Exposure;
};
layout(binding = 1) uniform sampler2D HdrTexture;

void main()
{
	// Fetch color
	OutColor = vec4(texelFetch(HdrTexture, ivec2(gl_FragCoord.xy), 0).rgb, 1.0);

	// Apply exposure
	OutColor.rgb *= Exposure;

	// REC709 -> AP0
	const mat3 REC709_2_AP0_MAT = REC709_2_XYZ_MAT * D65_2_D60_MAT * XYZ_2_AP0_MAT;
	OutColor.rgb = max(OutColor.rgb * REC709_2_AP0_MAT, 0.0);
	
	// Apply ACES RRT+ODT
	const mat3 AP1_2_REC709_MAT = AP1_2_XYZ_MAT * D60_2_D65_MAT * XYZ_2_REC709_MAT;
	OutColor.rgb = outputTransform(OutColor.rgb, 0.02, 4.8, 48.0, AP1_2_REC709_MAT, 2, 1);
}