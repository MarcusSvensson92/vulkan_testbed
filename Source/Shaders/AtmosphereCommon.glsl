#define PI						3.14159265
#define LARGE_VALUE				1e20

#define BETA_R					vec3(6.55e-6, 1.73e-5, 2.30e-5)
#define BETA_M					vec3(2e-6, 2e-6, 2e-6)
#define MIE_G					0.76
#define DENSITY_HEIGHT_SCALE_R	8000.0
#define DENSITY_HEIGHT_SCALE_M	1200.0

#define PLANET_RADIUS			6360e3
#define ATMOSPHERE_RADIUS		6420e3
#define HEIGHT_ABOVE_GROUND		500.0

vec2 RaySphereIntersection(vec3 orig, vec3 dir, float radius)
{
	float a = dot(dir, dir);
	float b = 2.0 * dot(dir, orig);
	float c = dot(orig, orig) - radius * radius;
	float d = sqrt(b * b - 4.0 * a * c);
	return vec2(-b - d, -b + d) / (2.0 * a);
}

vec2 LocalDensity(vec3 point)
{
	float height = length(point) - PLANET_RADIUS;
	return exp(-vec2(height / DENSITY_HEIGHT_SCALE_R, height / DENSITY_HEIGHT_SCALE_M));
}

vec3 Extinction(vec2 density)
{
	return exp(-(BETA_R * density.x + BETA_M * 1.1 * density.y));
}

vec2 DensityToAtmosphere(vec3 point, vec3 light_dir)
{
	vec2 intersection = RaySphereIntersection(point, light_dir, PLANET_RADIUS);
	if (intersection.x > 0.0)
	{
		return vec2(LARGE_VALUE, LARGE_VALUE);
	}
	intersection = RaySphereIntersection(point, light_dir, ATMOSPHERE_RADIUS);
	float ray_len = intersection.y;
	
	const int step_count = 64;
	float step_len = ray_len / float(step_count);

	vec2 density_point_to_atmosphere = vec2(0.0, 0.0);

	for (int i = 0; i < step_count; ++i)
	{
		vec3 point_on_ray = point + light_dir * ((float(i) + 0.5) * step_len);

		vec2 density = LocalDensity(point_on_ray) * step_len;
		density_point_to_atmosphere += density;
	}

	return density_point_to_atmosphere;
}

float PhaseR(float cos_angle)
{
	return (3.0 / (16.0 * PI)) * (1.0 + cos_angle * cos_angle);
}

float PhaseM(float cos_angle)
{
	return (3.0 / (8.0 * PI)) * ((1.0 - MIE_G * MIE_G) * (1.0 + cos_angle * cos_angle)) / ((2.0 + MIE_G * MIE_G) * pow(1.0 + MIE_G * MIE_G - 2.0 * MIE_G * cos_angle, 1.5));
}

float Sun(float cos_angle)
{
	const float g = 0.98;
	const float g2 = g * g;
	return pow(1.0 - g, 2.0) / (4.0 * PI * pow(1.0 + g2 - 2.0 * g * cos_angle, 1.5)) * 0.005;
}

float TexCoordToViewAngle(float tex_coord)
{
	float horizon_angle = -sqrt(HEIGHT_ABOVE_GROUND * (2.0 * PLANET_RADIUS + HEIGHT_ABOVE_GROUND)) / (PLANET_RADIUS + HEIGHT_ABOVE_GROUND);
	if (tex_coord > 0.5)
	{
		return horizon_angle + pow(tex_coord * 2.0 - 1.0, 5.0) * (1.0 - horizon_angle);
	}
	return horizon_angle - pow(tex_coord * 2.0, 5.0) * (1.0 + horizon_angle);
}

float ViewAngleToTexCoord(float view_angle)
{
	float horizon_angle = -sqrt(HEIGHT_ABOVE_GROUND * (2.0 * PLANET_RADIUS + HEIGHT_ABOVE_GROUND)) / (PLANET_RADIUS + HEIGHT_ABOVE_GROUND);
	if (view_angle > horizon_angle)
	{
	    return 0.5 * pow((view_angle - horizon_angle) / (1.0 - horizon_angle), 0.2) + 0.5;
	}
	return 0.5 * pow((horizon_angle - view_angle) / (horizon_angle + 1.0), 0.2);
}

float TexCoordToLightAngle(float tex_coord)
{
	return tan((2.0 * tex_coord - 1.0 + 0.26) * 1.1) / tan(1.26 * 1.1);
}

float LightAngleToTexCoord(float light_angle)
{
	return 0.5 * ((atan(max(light_angle, -0.1935) * tan(1.26 * 1.1)) / 1.1) + (1.0 - 0.26));
}
