#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

#include "AtmosphereCommon.glsl"

layout(location = 0) in vec3 InWorldPos;
layout(location = 1) in vec2 InTexCoord;
layout(location = 2) in vec3 InNormal;
layout(location = 3) in vec3 InTangent;
layout(location = 4) in vec3 InBitangent;

layout(location = 0) out vec3 OutColor;

layout(binding = 0) uniform Constants
{
    mat4    World;
	mat4	WorldViewProjection;
	vec3	ViewPosition;
	float	AmbientLightIntensity;
	vec3	LightDirection;
	float	DirectionalLightIntensity;
	vec4	BaseColorFactor;
	vec2	MetallicRoughnessFactor;
	bool	HasBaseColorTexture;
	bool	HasNormalTexture;
	bool	HasMetallicRoughnessTexture;
	bool	DebugEnable;
	uint	DebugIndex;
};
layout(binding = 1) uniform sampler2D BaseColor;
layout(binding = 2) uniform sampler2D Normal;
layout(binding = 3) uniform sampler2D MetallicRoughness;
layout(binding = 4) uniform sampler1D AmbientLightLUT;
layout(binding = 5) uniform sampler1D DirectionalLightLUT;
layout(binding = 6) uniform sampler2D AmbientOcclusion;
layout(binding = 7) uniform sampler2D Shadow;

float MicrofacetDistribution(float n_dot_h, float roughness)
{
	float r2 = roughness * roughness;
	float f = (n_dot_h * r2 - n_dot_h) * n_dot_h + 1.0;
	return r2 / max(1e-6, PI * f * f);
}
float GeometricOcclusion(float n_dot_v, float n_dot_l, float roughness)
{
	float r2 = roughness * roughness;
	float gv = 2.0 * n_dot_v / max(1e-6, n_dot_v + sqrt(r2 + (1.0 - r2) * (n_dot_v * n_dot_v)));
	float gl = 2.0 * n_dot_l / max(1e-6, n_dot_l + sqrt(r2 + (1.0 - r2) * (n_dot_l * n_dot_l)));
	return gv * gl;
}
vec3 SpecularReflection(vec3 f0, float v_dot_h)
{
	return f0 + (1.0 - f0) * pow(1.0 - v_dot_h, 5.0);
}

void main()
{
	vec4 base_color = BaseColorFactor;
	if (HasBaseColorTexture)
	{
		base_color *= texture(BaseColor, InTexCoord);
	}
	if (base_color.a < 0.5)
	{
		discard;
	}

	vec3 view_vec = normalize(ViewPosition - InWorldPos);
    vec3 half_vec = normalize(LightDirection + view_vec);
	
	vec3 normal = normalize(InNormal);
	if (HasNormalTexture)
	{
		vec3 normal_map = normalize(texture(Normal, InTexCoord).rgb * 2.0 - 1.0);
		normal = normalize(normalize(InTangent) * normal_map.x + normalize(InBitangent) * normal_map.y + normal * normal_map.z);
	}

	float n_dot_l = max(dot(normal, LightDirection), 0.0);
	float n_dot_v = max(dot(normal, view_vec), 0.0);
	float n_dot_h = max(dot(normal, half_vec), 0.0);
	float v_dot_h = max(dot(view_vec, half_vec), 0.0);

	vec2 metallic_roughness = MetallicRoughnessFactor;
	if (HasMetallicRoughnessTexture)
	{
		metallic_roughness *= texture(MetallicRoughness, InTexCoord).bg;
	}
	float metallic = metallic_roughness.x;
	float roughness = max(0.004, metallic_roughness.y * metallic_roughness.y);

	float d = MicrofacetDistribution(n_dot_h, roughness);
	float g = GeometricOcclusion(n_dot_v, n_dot_l, roughness);
	vec3 f0 = mix(vec3(0.04), base_color.rgb, metallic);
	vec3 f = SpecularReflection(f0, v_dot_h);
	
	vec3 specular = (d * g * f) / max(4.0 * n_dot_v * n_dot_l, 1e-3);
	vec3 diffuse = (vec3(1.0) - f) * (1.0 - metallic) * base_color.rgb / PI;

	float light_angle = dot(LightDirection, vec3(0.0, 1.0, 0.0));
	float light_tex_coord = LightAngleToTexCoord(light_angle);
	
	vec3 dir_light = texture(DirectionalLightLUT, light_tex_coord).rgb * DirectionalLightIntensity;
	vec3 ambient_light = texture(AmbientLightLUT, light_tex_coord).rgb * AmbientLightIntensity;

	float shadow = texelFetch(Shadow, ivec2(gl_FragCoord.xy), 0).r;
	float ao = texelFetch(AmbientOcclusion, ivec2(gl_FragCoord.xy), 0).r;

	OutColor = (diffuse + specular) * dir_light * n_dot_l * shadow + ambient_light * base_color.rgb * ao;

	if (DebugEnable)
	{
		OutColor = DebugIndex == 0 ? base_color.rgb     : OutColor;
		OutColor = DebugIndex == 1 ? normal * 0.5 + 0.5 : OutColor;
		OutColor = DebugIndex == 2 ? vec3(metallic)     : OutColor;
		OutColor = DebugIndex == 3 ? vec3(roughness)    : OutColor;
		OutColor = DebugIndex == 4 ? vec3(d)            : OutColor;
		OutColor = DebugIndex == 5 ? vec3(g)            : OutColor;
		OutColor = DebugIndex == 6 ? vec3(f)            : OutColor;
		OutColor = DebugIndex == 7 ? vec3(shadow)       : OutColor;
		OutColor = DebugIndex == 8 ? vec3(ao)           : OutColor;
	}
}