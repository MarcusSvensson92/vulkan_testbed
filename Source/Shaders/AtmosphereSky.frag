#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

#include "AtmosphereCommon.glsl"

layout(location = 0) in vec3 InRayDirection;

layout(location = 0) out vec3 OutColor;

layout(binding = 0) uniform Constants
{
    mat4	InvViewProjZeroTranslation;
	vec3	LightDirection;
	float	LightIntensity;
};
layout(binding = 1) uniform sampler2D SkyLUTR;
layout(binding = 2) uniform sampler2D SkyLUTM;

void main()
{
	vec3 ray_dir = normalize(InRayDirection);

	float view_angle = dot(ray_dir, vec3(0.0, 1.0, 0.0));
	float light_angle = dot(LightDirection, vec3(0.0, 1.0, 0.0));

	vec2 tex_coord = vec2(ViewAngleToTexCoord(view_angle), LightAngleToTexCoord(light_angle));
	vec3 scatter_r = texture(SkyLUTR, tex_coord).rgb;
    vec3 scatter_m = texture(SkyLUTM, tex_coord).rgb;

	// Calculate phase functions
	// Phase functions describe how much light is scattered when colliding with particles
	float cos_angle = dot(ray_dir, LightDirection);
	float phase_r = PhaseR(cos_angle);
	float phase_m = PhaseM(cos_angle);
	
	// Calculate light color
	OutColor = (scatter_r * phase_r * BETA_R + scatter_m * phase_m * BETA_M) * LightIntensity;
	
	// Add sun light
	OutColor += Sun(cos_angle) * scatter_m;

	// Prevent negative values
	OutColor = max(OutColor, 0.0);
}
