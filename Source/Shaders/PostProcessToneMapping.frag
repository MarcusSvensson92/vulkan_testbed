#version 450
#extension GL_GOOGLE_include_directive : require

#include "ACES.glsl"

#define DISPLAY_MODE_SDR				0
#define DISPLAY_MODE_HDR10				1
#define DISPLAY_MODE_SCRGB				2

#define DISPLAY_MAPPING_ACES_ODT		0
#define DISPLAY_MAPPING_BT2390_EETF		1
#define DISPLAY_MAPPING_SDR_EMULATION	2
#define DISPLAY_MAPPING_LUM_VIS_SCENE	3
#define DISPLAY_MAPPING_LUM_VIS_DISPLAY	4

layout(location = 0) in vec2 InTexCoord;

layout(location = 0) out vec4 OutColor;

layout(binding = 0) uniform Constants
{
    float	Exposure;
	bool	DebugEnable;
	bool	ViewLuxoDoubleChecker;
	int		DisplayMode;
	int		DisplayMapping;
	int		DisplayMappingAux;
	bool	DisplayMappingSplitScreen;
	int		DisplayMappingSplitScreenOffset;
	float	HdrDisplayLuminanceMin;
	float	HdrDisplayLuminanceMax;
	float	SdrWhiteLevel;
	float	ACESMidPoint;
	float	BT2390MidPoint;
};
layout(binding = 1) uniform sampler2D SceneTexture;
layout(binding = 2) uniform sampler2D UiTexture;
layout(binding = 3) uniform sampler2D LuxoDoubleChecker;

// https://www.dolby.com/us/en/technologies/dolby-vision/ictcp-white-paper.pdf
vec3 LinearToICtCp(vec3 rgb)
{
	// Calculate LMS
	float l = (1688.0 / 4096.0) * rgb.r + (2146.0 / 4096.0) * rgb.g + ( 262.0 / 4096.0) * rgb.b;
	float m = ( 683.0 / 4096.0) * rgb.r + (2951.0 / 4096.0) * rgb.g + ( 462.0 / 4096.0) * rgb.b;
	float s = (  99.0 / 4096.0) * rgb.r + ( 309.0 / 4096.0) * rgb.g + (3688.0 / 4096.0) * rgb.b;

	// Linear -> PQ
	l = Y_2_ST2084(l);
	m = Y_2_ST2084(m);
	s = Y_2_ST2084(s);

	// Calculate ICtCp
	float i = 0.5 * l + 0.5 * m;
	float ct = ( 6610.0 / 4096.0) * l - (13613.0 / 4096.0) * m + (7003.0 / 4096.0) * s;
	float cp = (17933.0 / 4096.0) * l - (17390.0 / 4096.0) * m - ( 543.0 / 4096.0) * s;

	return vec3(i, ct, cp);
}
vec3 ICtCpToLinear(vec3 ictcp)
{
	// Calculate LMS
	float l = ictcp.x + ( 1112064.0 / 129174029.0) * ictcp.y + (14342144.0 / 129174029.0) * ictcp.z;
	float m = ictcp.x - ( 1112064.0 / 129174029.0) * ictcp.y - (14342144.0 / 129174029.0) * ictcp.z;
	float s = ictcp.x + (72341504.0 / 129174029.0) * ictcp.y - (41416704.0 / 129174029.0) * ictcp.z;

	// PQ -> Linear
	l = ST2084_2_Y(l);
	m = ST2084_2_Y(m);
	s = ST2084_2_Y(s);

	// Calculate RGB
	float r =  (1074053.0 /  312533.0) * l - ( 783349.0 /  312533.0) * m + (  21829.0 /  312533.0) * s;
	float g = -(1236583.0 / 1562665.0) * l + (3099703.0 / 1562665.0) * m - ( 300455.0 / 1562665.0) * s;
	float b = -(  40551.0 / 1562665.0) * l - ( 154569.0 / 1562665.0) * m + (1757785.0 / 1562665.0) * s;

	return vec3(r, g, b);
}

// https://www.itu.int/dms_pub/itu-r/opb/rep/R-REP-BT.2390-8-2020-PDF-E.pdf
float EETF(float e, float lb, float lw, float lmin, float lmax)
{
	e = (e - lb) / (lw - lb);

	float min_lum = (lmin - lb) / (lw - lb);
	float max_lum = (lmax - lb) / (lw - lb);

	float ks = 1.5 * max_lum - 0.5;
	float b = min_lum;

	if (e > ks && e <= 1.0)
	{
		float t = (e - ks) / (1.0 - ks);
		float t2 = t * t;
		float t3 = t * t2;
		e = (2.0 * t3 - 3.0 * t2 + 1.0) * ks + (t3 - 2.0 * t2 + t) * (1.0 - ks) + (-2.0 * t3 + 3.0 * t2) * max_lum;
	}

	if (e >= 0.0 && e <= 1.0)
	{
		float t = 1.0 - e;
		float t2 = t * t;
		float t4 = t2 * t2;
		e = e + b * t4;
	}

	e = e * (lw - lb) + lb;

	return e;
}

// https://github.com/ehartNV/UnrealEngine_HDR/blob/release/Engine/Shaders/PostProcessTonemap.usf
vec3 VisualizeLuminance(float luminance)
{
	const vec3 colors[9] = vec3[9](
		vec3(0.0, 0.0, 0.0),
		vec3(0.0, 0.0, 1.0),
		vec3(0.0, 1.0, 1.0),
		vec3(0.0, 1.0, 0.0),
		vec3(1.0, 1.0, 0.0),
		vec3(1.0, 0.0, 0.0),
		vec3(1.0, 0.0, 1.0),
		vec3(1.0, 1.0, 1.0),
		vec3(1.0, 1.0, 1.0)
	);
	float log_lum = log2(luminance);
	float scale = clamp(log_lum * 0.5 + 2.0, 0.0, 7.0);
	int index = int(scale);
	return mix(colors[index], colors[index + 1], scale - float(index));
}

void main()
{
	// Fetch color
	vec3 color_rec709 = texelFetch(SceneTexture, ivec2(gl_FragCoord.xy), 0).rgb;

	if (DebugEnable)
	{
		OutColor.rgb = color_rec709;

		if (DisplayMode != DISPLAY_MODE_SDR)
		{
			// REC709 -> REC2020
			const mat3 REC709_2_REC2020_MAT = REC709_2_XYZ_MAT * XYZ_2_REC2020_MAT;
			OutColor.rgb = max(OutColor.rgb * REC709_2_REC2020_MAT, 0.0);

			// Scale to white level
			OutColor.rgb = OutColor.rgb * SdrWhiteLevel;
		}
	}
	else
	{
		if (ViewLuxoDoubleChecker)
		{
			color_rec709 = texture(LuxoDoubleChecker, InTexCoord).rgb;
		}

		// Apply exposure
		color_rec709 *= Exposure;

		// REC709 -> AP0
		const mat3 REC709_2_AP0_MAT = REC709_2_XYZ_MAT * D65_2_D60_MAT * XYZ_2_AP0_MAT;
		vec3 color_ap0 = color_rec709 * REC709_2_AP0_MAT;

		int display_mapping = DisplayMapping;
		if (DisplayMappingSplitScreen && int(gl_FragCoord.x) > DisplayMappingSplitScreenOffset)
			display_mapping = DisplayMappingAux;

		if (DisplayMode == DISPLAY_MODE_SDR)
		{
			const mat3 AP1_2_REC709_MAT = AP1_2_XYZ_MAT * D60_2_D65_MAT * XYZ_2_REC709_MAT;
			OutColor.rgb = outputTransform(color_ap0, 0.02, 4.8, 48.0, AP1_2_REC709_MAT, 1, 1);
		}
		else
		{
			if (display_mapping == DISPLAY_MAPPING_ACES_ODT)
			{
				const mat3 AP1_2_REC2020_MAT = AP1_2_XYZ_MAT * D60_2_D65_MAT * XYZ_2_REC2020_MAT;
				OutColor.rgb = outputTransform(color_ap0, HdrDisplayLuminanceMin, ACESMidPoint, HdrDisplayLuminanceMax, AP1_2_REC2020_MAT, 4, 0);
			}
			else if (display_mapping == DISPLAY_MAPPING_BT2390_EETF) // Based on "DD2018: Paul Malin - HDR display in Call of Duty" (https://www.youtube.com/watch?v=EN1Uk6vJqRw)
			{
				const mat3 AP1_2_REC2020_MAT = AP1_2_XYZ_MAT * D60_2_D65_MAT * XYZ_2_REC2020_MAT;
				OutColor.rgb = outputTransform(color_ap0, 0.0001, BT2390MidPoint, 10000.0, AP1_2_REC2020_MAT, 4, 0);

				// Linear -> ICtCp
				OutColor.rgb = LinearToICtCp(OutColor.rgb);

				// Apply EETF
				float i1 = OutColor.r;
				float i2 = EETF(i1, 0.0, 1.0, Y_2_ST2084(HdrDisplayLuminanceMin), Y_2_ST2084(HdrDisplayLuminanceMax));
				OutColor.r = i2;
				OutColor.gb *= min(i1 / i2, i2 / i1);

				// ICtCp -> Linear
				OutColor.rgb = ICtCpToLinear(OutColor.rgb);

				// Desaturate highlights
				float luminance = dot(OutColor.rgb, REC2020_2_XYZ_MAT[1]);
				float saturation = mix(1.0, 0.7, clamp((luminance - 150.0) / (200.0 - 150.0), 0.0, 1.0));
				OutColor.rgb = mix(luminance.xxx, OutColor.rgb, saturation);
			}
			else if (display_mapping == DISPLAY_MAPPING_SDR_EMULATION)
			{
				// Apply ACES SDR output transform
				const mat3 AP1_2_REC709_MAT = AP1_2_XYZ_MAT * D60_2_D65_MAT * XYZ_2_REC709_MAT;
				OutColor.rgb = outputTransform(color_ap0, 0.02, 4.8, 48.0, AP1_2_REC709_MAT, 1, 1);

				// Quantize as 8-bit unorm
				OutColor.rgb = round(OutColor.rgb * 255.0) / 255.0;

				// BT1886 -> Linear
				OutColor.rgb = bt1886_f(OutColor.rgb, 2.4, 1.0, 0.0);

				// REC709 -> REC2020
				const mat3 REC709_2_REC2020_MAT = REC709_2_XYZ_MAT * XYZ_2_REC2020_MAT;
				OutColor.rgb = max(OutColor.rgb * REC709_2_REC2020_MAT, 0.0);

				// Scale to white level
				OutColor.rgb *= SdrWhiteLevel;
			}
			else if (display_mapping == DISPLAY_MAPPING_LUM_VIS_SCENE)
			{
				float luminance = dot(color_rec709 / Exposure, REC709_2_XYZ_MAT[1]);
				OutColor.rgb = VisualizeLuminance(luminance) * SdrWhiteLevel;
			}
			else if (display_mapping == DISPLAY_MAPPING_LUM_VIS_DISPLAY)
			{
				const mat3 AP1_2_REC2020_MAT = AP1_2_XYZ_MAT * D60_2_D65_MAT * XYZ_2_REC2020_MAT;
				vec3 color_rec2020 = outputTransform(color_ap0, HdrDisplayLuminanceMin, ACESMidPoint, HdrDisplayLuminanceMax, AP1_2_REC2020_MAT, 4, 0);
				float luminance = dot(color_rec2020, REC2020_2_XYZ_MAT[1]);
				OutColor.rgb = VisualizeLuminance(luminance) * SdrWhiteLevel;
			}
			else
			{
				OutColor.rgb = vec3(0.0);
			}
		}
	}

	// Apply UI
	vec4 ui_color = texelFetch(UiTexture, ivec2(gl_FragCoord.xy), 0);
	if (DisplayMode != DISPLAY_MODE_SDR)
	{
		// sRGB -> Linear
		ui_color.rgb = moncurve_f(ui_color.rgb, 2.4, 0.055);

		// REC709 -> REC2020
		const mat3 REC709_2_REC2020_MAT = REC709_2_XYZ_MAT * XYZ_2_REC2020_MAT;
		ui_color.rgb = max(ui_color.rgb * REC709_2_REC2020_MAT, 0.0);

		// Scale to white level
		ui_color.rgb = ui_color.rgb * SdrWhiteLevel;
	}

	// Blend UI and scene color
	OutColor.rgb = OutColor.rgb * (1.0 - ui_color.a) + ui_color.rgb * ui_color.a;

	if (DisplayMode == DISPLAY_MODE_HDR10)
	{
		// Linear -> PQ
		OutColor.rgb = Y_2_ST2084(OutColor.rgb);
	}
	else if (DisplayMode == DISPLAY_MODE_SCRGB)
	{
		OutColor.rgb = Y_2_linCV(OutColor.rgb, HdrDisplayLuminanceMax, HdrDisplayLuminanceMin);

		// REC2020 -> REC709
		const mat3 REC2020_2_REC709_MAT = REC2020_2_XYZ_MAT * XYZ_2_REC709_MAT;
		OutColor.rgb = max(OutColor.rgb * REC2020_2_REC709_MAT, 0.0);

		OutColor.rgb = OutColor.rgb * (HdrDisplayLuminanceMax / 80.0);
	}

	// Set alpha to 1
	OutColor.a = 1.0;
}