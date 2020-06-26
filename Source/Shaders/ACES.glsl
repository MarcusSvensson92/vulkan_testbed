// https://github.com/ampas/aces-dev/blob/v1.2/transforms/ctl/README-MATRIX.md

const mat3 AP0_2_XYZ_MAT = mat3( 0.9525523959,  0.0000000000,  0.0000936786,
								 0.3439664498,  0.7281660966, -0.0721325464,
								 0.0000000000,  0.0000000000,  1.0088251844);

const mat3 XYZ_2_AP0_MAT = mat3( 1.0498110175,  0.0000000000, -0.0000974845,
								-0.4959030231,  1.3733130458,  0.0982400361,
								 0.0000000000,  0.0000000000,  0.9912520182);

const mat3 AP1_2_XYZ_MAT = mat3( 0.6624541811,  0.1340042065,  0.1561876870,
								 0.2722287168,  0.6740817658,  0.0536895174,
								-0.0055746495,  0.0040607335,  1.0103391003);

const mat3 XYZ_2_AP1_MAT = mat3( 1.6410233797, -0.3248032942, -0.2364246952,
								-0.6636628587,  1.6153315917,  0.0167563477,
								 0.0117218943, -0.0082844420,  0.9883948585);

const mat3 AP0_2_AP1_MAT = mat3( 1.4514393161, -0.2365107469, -0.2149285693,
								-0.0765537734,  1.1762296998, -0.0996759264,
								 0.0083161484, -0.0060324498,  0.9977163014);

const mat3 AP1_2_AP0_MAT = mat3( 0.6954522414,  0.1406786965,  0.1638690622,
								 0.0447945634,  0.8596711185,  0.0955343182,
								-0.0055258826,  0.0040252103,  1.0015006723);

const mat3 XYZ_2_REC709_MAT = mat3( 3.2409699419, -1.5373831776, -0.4986107603,
								   -0.9692436363,  1.8759675015,  0.0415550574,
									0.0556300797, -0.2039769589,  1.0569715142);

const mat3 REC709_2_XYZ_MAT = mat3( 0.4123907993,  0.3575843394,  0.1804807884,
								    0.2126390059,  0.7151686788,  0.0721923154,
								    0.0193308187,  0.1191947798,  0.9505321523);

const mat3 XYZ_2_REC2020_MAT = mat3( 1.7166511880, -0.3556707838, -0.2533662814,
								    -0.6666843518,  1.6164812366,  0.0157685458,
									 0.0176398574, -0.0427706133,  0.9421031212);

const mat3 REC2020_2_XYZ_MAT = mat3( 0.6369580482,  0.1446169036,  0.1688809752,
								     0.2627002120,  0.6779980715,  0.0593017165,
								     0.0000000000,  0.0280726931,  1.0609850578);

const mat3 D60_2_D65_MAT = mat3( 0.9872240000, -0.0061132700,  0.0159533000,
								-0.0075983600,  1.0018600000,  0.0053300200,
								 0.0030725700, -0.0050959500,  1.0816800000);

const mat3 D65_2_D60_MAT = mat3( 1.0130349239,  0.0061053089, -0.0149709632,
								 0.0076982296,  0.9981648318, -0.0050320341,
								-0.0028413125,  0.0046851556,  0.9245066529);

// https://github.com/ampas/aces-dev/blob/v1.2/transforms/ctl/lib/ACESlib.Transform_Common.ctl

float rgb_2_saturation(vec3 rgb)
{
	float rgbMax = max(max(rgb.r, rgb.g), rgb.b);
	float rgbMin = min(min(rgb.r, rgb.g), rgb.b);
	return (max(rgbMax, 1e-10) - max(rgbMin, 1e-10)) / max(rgbMax, 1e-2);
}

// https://github.com/ampas/aces-dev/blob/v1.2/transforms/ctl/lib/ACESlib.Utilities_Color.ctl

vec3 XYZ_2_xyY(vec3 XYZ)
{
	float divisor = (XYZ.x + XYZ.y + XYZ.z);
	if (divisor == 0.0)
		divisor = 1e-10;
	vec3 xyY;
	xyY.x = XYZ.x / divisor;
	xyY.y = XYZ.y / divisor;
	xyY.z = XYZ.y;
	return xyY;
}
vec3 xyY_2_XYZ(vec3 xyY)
{
	vec3 XYZ;
	XYZ.x = xyY.x * xyY.z / max(xyY.y, 1e-10);
	XYZ.y = xyY.z;
	XYZ.z = (1.0 - xyY.x - xyY.y) * xyY.z / max(xyY.y, 1e-10);
	return XYZ;
}

vec3 moncurve_f(vec3 x, float gamma, float offs)
{
	const float fs = ((gamma - 1.0) / offs) * pow(offs * gamma / ((gamma - 1.0) * (1.0 + offs)), gamma);
	const float xb = offs / (gamma - 1.0);

	vec3 y;
	y.x = (x.x >= xb) ? (pow((x.x + offs) / (1.0 + offs), gamma)) : (x.x * fs);
	y.y = (x.y >= xb) ? (pow((x.y + offs) / (1.0 + offs), gamma)) : (x.y * fs);
	y.z = (x.z >= xb) ? (pow((x.z + offs) / (1.0 + offs), gamma)) : (x.z * fs);
	return y;
}
vec3 moncurve_r(vec3 y, float gamma, float offs)
{
	const float yb = pow(offs * gamma / ((gamma - 1.0) * (1.0 + offs)), gamma);
	const float rs = pow((gamma - 1.0) / offs, gamma - 1.0) * pow((1.0 + offs) / gamma, gamma);

	vec3 x;
	x.x = (y.x >= yb) ? ((1.0 + offs) * pow(y.x, 1.0 / gamma) - offs) : (y.x * rs);
	x.y = (y.y >= yb) ? ((1.0 + offs) * pow(y.y, 1.0 / gamma) - offs) : (y.y * rs);
	x.z = (y.z >= yb) ? ((1.0 + offs) * pow(y.z, 1.0 / gamma) - offs) : (y.z * rs);
	return x;
}

vec3 bt1886_f(vec3 V, float gamma, float Lw, float Lb)
{
  const float a = pow(pow(Lw, 1.0 / gamma) - pow(Lb, 1.0 / gamma), gamma);
  const float b = pow(Lb, 1.0 / gamma) / (pow(Lw, 1.0 / gamma) - pow(Lb, 1.0 / gamma));
  vec3 L = a * pow(max(V + b, 0.0), vec3(gamma));
  return L;
}
vec3 bt1886_r(vec3 L, float gamma, float Lw, float Lb)
{
	const float a = pow(pow(Lw, 1.0 / gamma) - pow(Lb, 1.0 / gamma), gamma);
	const float b = pow(Lb, 1.0 / gamma) / (pow(Lw, 1.0 / gamma) - pow(Lb, 1.0 / gamma));
	vec3 V = pow(max(L / a, 0.0), vec3(1.0 / gamma)) - b;
	return V;
}

float ST2084_2_Y(float N)
{
	float Np = pow(N, 1.0 / 78.84375);
	float L = Np - 0.8359375;
	L = max(L, 0.0);
	L = L / (18.8515625 - 18.6875 * Np);
	L = pow(L, 1.0 / 0.1593017578125);
	return L * 10000.0;
}
float Y_2_ST2084(float C)
{
	float L = C / 10000.0;
	float Lm = pow(L, 0.1593017578125);
	float N = (0.8359375 + 18.8515625 * Lm) / (1.0 + 18.6875 * Lm);
	N = pow(N, 78.84375);
	return N;
}
vec3 ST2084_2_Y(vec3 N)
{
	vec3 Np = pow(N, vec3(1.0 / 78.84375));
	vec3 L = Np - 0.8359375;
	L = max(L, 0.0);
	L = L / (18.8515625 - 18.6875 * Np);
	L = pow(L, vec3(1.0 / 0.1593017578125));
	return L * 10000.0;
}
vec3 Y_2_ST2084(vec3 C)
{
	vec3 L = C / 10000.0;
	vec3 Lm = pow(L, vec3(0.1593017578125));
	vec3 N = (0.8359375 + 18.8515625 * Lm) / (1.0 + 18.6875 * Lm);
	N = pow(N, vec3(78.84375));
	return N;
}

float rgb_2_hue(vec3 rgb)
{
	float hue = 0.0;
	if (rgb.r != rgb.g || rgb.g != rgb.b)
		hue = (180.0 / 3.14159265359) * atan(sqrt(3.0) * (rgb.g - rgb.b), 2.0 * rgb.r - rgb.g - rgb.b);
	if (hue < 0.0) hue += 360.0;
	return hue;
}
float rgb_2_yc(vec3 rgb, float ycRadiusWeight)
{
	float chroma = sqrt(rgb.b * (rgb.b - rgb.g) + rgb.g * (rgb.g - rgb.r) + rgb.r * (rgb.r - rgb.b));
	return (rgb.b + rgb.g + rgb.r + ycRadiusWeight * chroma) / 3.0;
}

// https://github.com/ampas/aces-dev/blob/v1.2/transforms/ctl/lib/ACESlib.RRT_Common.ctl

const float RRT_GLOW_GAIN = 0.05;
const float RRT_GLOW_MID = 0.08;

const float RRT_RED_SCALE = 0.82;
const float RRT_RED_PIVOT = 0.03;
const float RRT_RED_HUE = 0.0;
const float RRT_RED_WIDTH = 135.0;

const float RRT_SAT_FACTOR = 0.96;

// ------- Glow module functions
float glow_fwd(float ycIn, float glowGainIn, float glowMid)
{
	float glowGainOut = 0.0;
	if (ycIn <= (2.0 / 3.0) * glowMid)
		glowGainOut = glowGainIn;
	else if (ycIn < 2.0 * glowMid)
		glowGainOut = glowGainIn * (glowMid / ycIn - 0.5);
	return glowGainOut;
}
float sigmoid_shaper(float x)
{
	float t = max(0.0, 1.0 - abs(x / 2.0));
	float y = 1.0 + sign(x) * (1.0 - t * t);
	return y / 2.0;
}

// ------- Red modifier functions
float cubic_basis_shaper(float x, float w)
{
	const float M[4][4] = float[4][4](
		float[4]( -1.0 / 6.0,  3.0 / 6.0, -3.0 / 6.0,  1.0 / 6.0 ),
		float[4](  3.0 / 6.0, -6.0 / 6.0,  3.0 / 6.0,  0.0 / 6.0 ),
		float[4]( -3.0 / 6.0,  0.0 / 6.0,  3.0 / 6.0,  0.0 / 6.0 ),
		float[4](  1.0 / 6.0,  4.0 / 6.0,  1.0 / 6.0,  0.0 / 6.0 )
	);

	const float knots[5] = float[5](
		-w * 0.50,
		-w * 0.25,
		0.0,
		w * 0.25,
		w * 0.50
	);

	float y = 0.0;
	if (x > knots[0] && x < knots[4])
	{
		float knot_coord = (x - knots[0]) * 4.0 / w;
		int j = int(knot_coord);
		float t = knot_coord - float(j);

		float monomials[4] = float[4]( t * t * t, t * t, t, 1.0 );

		if (j == 3)
		{
			y = monomials[0] * M[0][0] + monomials[1] * M[1][0] +
				monomials[2] * M[2][0] + monomials[3] * M[3][0];
		}
		else if (j == 2)
		{
			y = monomials[0] * M[0][1] + monomials[1] * M[1][1] +
				monomials[2] * M[2][1] + monomials[3] * M[3][1];
		}
		else if (j == 1)
		{
			y = monomials[0] * M[0][2] + monomials[1] * M[1][2] +
				monomials[2] * M[2][2] + monomials[3] * M[3][2];
		}
		else if (j == 0)
		{
			y = monomials[0] * M[0][3] + monomials[1] * M[1][3] +
				monomials[2] * M[2][3] + monomials[3] * M[3][3];
		}
	}
	return y * (3.0 / 2.0);
}
float center_hue(float hue, float centerH)
{
	float hueCentered = hue - centerH;
	if (hueCentered < -180.0) hueCentered += 360.0;
	else if (hueCentered > 180.0) hueCentered -= 360.0;
	return hueCentered;
}

vec3 rrt_sweeteners(vec3 aces)
{
	// --- Glow module --- //
    float saturation = rgb_2_saturation(aces);
    float ycIn = rgb_2_yc(aces, 1.75);
    float s = sigmoid_shaper((saturation - 0.4) / 0.2);
    float addedGlow = 1.0 + glow_fwd(ycIn, RRT_GLOW_GAIN * s, RRT_GLOW_MID);
	aces *= addedGlow;

	// --- Red modifier --- //
    float hue = rgb_2_hue(aces);
    float centeredHue = center_hue(hue, RRT_RED_HUE);
    float hueWeight = cubic_basis_shaper(centeredHue, RRT_RED_WIDTH);
	aces.r = aces.r + hueWeight * saturation * (RRT_RED_PIVOT - aces.r) * (1.0 - RRT_RED_SCALE);

	// --- ACES to RGB rendering space --- //
	vec3 rgbPre = max(max(aces, 0.0) * AP0_2_AP1_MAT, 0.0);

	// --- Global desaturation --- //
	rgbPre = mix(vec3(dot(rgbPre, AP1_2_XYZ_MAT[1])), rgbPre, RRT_SAT_FACTOR);

	return rgbPre;
}

// https://github.com/ampas/aces-dev/blob/v1.2/transforms/ctl/lib/ACESlib.ODT_Common.ctl

const float DIM_SURROUND_GAMMA = 0.9811;

vec3 Y_2_linCV(vec3 Y, float Ymax, float Ymin)
{
	return (Y - Ymin) / (Ymax - Ymin);
}
vec3 linCV_2_Y(vec3 linCV, float Ymax, float Ymin)
{
	return linCV * (Ymax - Ymin) + Ymin;
}

vec3 darkSurround_to_dimSurround(vec3 linearCV)
{
	vec3 XYZ = max(linearCV * AP1_2_XYZ_MAT, 0.0);

	vec3 xyY = XYZ_2_xyY(XYZ);
	xyY[2] = clamp(xyY[2], 0.0, 31744.0);
	xyY[2] = pow(xyY[2], DIM_SURROUND_GAMMA);
	XYZ = xyY_2_XYZ(xyY);

	return max(XYZ * XYZ_2_AP1_MAT, 0.0);
}

// https://github.com/ampas/aces-dev/blob/v1.2/transforms/ctl/lib/ACESlib.SSTS.ctl

const mat3 M1 = mat3( 0.5, -1.0, 0.5,
					 -1.0,  1.0, 0.5,
					  0.5,  0.0, 0.0);

struct TsPoint
{
	float x;
	float y;
	float slope;
};
struct TsParams
{
	TsPoint Min;
	TsPoint Mid;
	TsPoint Max;
	float coefsLow[6];
	float coefsHigh[6];
};

const float MIN_STOP_SDR = -6.5;
const float MAX_STOP_SDR = 6.5;

const float MIN_STOP_RRT = -15.0;
const float MAX_STOP_RRT = 18.0;

const float MIN_LUM_SDR = 0.02;
const float MAX_LUM_SDR = 48.0;

const float MIN_LUM_RRT = 0.0001;
const float MAX_LUM_RRT = 10000.0;

float log10(float x)
{
	return log2(x) * (1.0 / log2(10.0));
}

float interpolate1D(const float table[2][2], float p)
{
	if (p < table[0][0])
		return table[0][1];
	else if (p >= table[1][0])
		return table[1][1];
	float s = (p - table[0][0]) / (table[1][0] - table[0][0]);
	return table[0][1] * (1.0 - s) + table[1][1] * s;
}

float lookup_ACESmin(float minLum)
{
	const float minTable[2][2] = float[2][2](
		float[2]( log10(MIN_LUM_RRT), MIN_STOP_RRT ),
		float[2]( log10(MIN_LUM_SDR), MIN_STOP_SDR ));
	return 0.18 * pow(2.0, interpolate1D(minTable, log10(minLum)));
}
float lookup_ACESmax(float maxLum)
{
	const float maxTable[2][2] = float[2][2](
		float[2]( log10(MAX_LUM_SDR), MAX_STOP_SDR ),
		float[2]( log10(MAX_LUM_RRT), MAX_STOP_RRT ));
	return 0.18 * pow(2.0, interpolate1D(maxTable, log10(maxLum)));
}

float[5] init_coefsLow(TsPoint TsPointLow, TsPoint TsPointMid)
{
	float coefsLow[5];

	float knotIncLow = (log10(TsPointMid.x) - log10(TsPointLow.x)) / 3.0;

	// Determine two lowest coefficients (straddling minPt)
	coefsLow[0] = (TsPointLow.slope * (log10(TsPointLow.x) - 0.5 * knotIncLow)) + (log10(TsPointLow.y) - TsPointLow.slope * log10(TsPointLow.x));
	coefsLow[1] = (TsPointLow.slope * (log10(TsPointLow.x) + 0.5 * knotIncLow)) + (log10(TsPointLow.y) - TsPointLow.slope * log10(TsPointLow.x));
	
	// Determine two highest coefficients (straddling midPt)
	coefsLow[3] = (TsPointMid.slope * (log10(TsPointMid.x) - 0.5 * knotIncLow)) + (log10(TsPointMid.y) - TsPointMid.slope * log10(TsPointMid.x));
	coefsLow[4] = (TsPointMid.slope * (log10(TsPointMid.x) + 0.5 * knotIncLow)) + (log10(TsPointMid.y) - TsPointMid.slope * log10(TsPointMid.x));

	// Middle coefficient (which defines the "sharpness of the bend") is linearly interpolated
	const float bendsLow[2][2] = float[2][2](
		float[2]( MIN_STOP_RRT, 0.18 ),
		float[2]( MIN_STOP_SDR, 0.35 ));
	float pctLow = interpolate1D(bendsLow, log2(TsPointLow.x / 0.18));
	coefsLow[2] = log10(TsPointLow.y) + pctLow * (log10(TsPointMid.y) - log10(TsPointLow.y));

	return coefsLow;
}
float[5] init_coefsHigh(TsPoint TsPointMid, TsPoint TsPointMax)
{
	float coefsHigh[5];

	float knotIncHigh = (log10(TsPointMax.x) - log10(TsPointMid.x)) / 3.0;

	// Determine two lowest coefficients (straddling midPt)
	coefsHigh[0] = (TsPointMid.slope * (log10(TsPointMid.x) - 0.5 * knotIncHigh)) + (log10(TsPointMid.y) - TsPointMid.slope * log10(TsPointMid.x));
	coefsHigh[1] = (TsPointMid.slope * (log10(TsPointMid.x) + 0.5 * knotIncHigh)) + (log10(TsPointMid.y) - TsPointMid.slope * log10(TsPointMid.x));

	// Determine two highest coefficients (straddling maxPt)
	coefsHigh[3] = (TsPointMax.slope * (log10(TsPointMax.x) - 0.5 * knotIncHigh)) + (log10(TsPointMax.y) - TsPointMax.slope * log10(TsPointMax.x));
	coefsHigh[4] = (TsPointMax.slope * (log10(TsPointMax.x) + 0.5 * knotIncHigh)) + (log10(TsPointMax.y) - TsPointMax.slope * log10(TsPointMax.x));
	
	// Middle coefficient (which defines the "sharpness of the bend") is linearly interpolated
	const float bendsHigh[2][2] = float[2][2](
		float[2]( MAX_STOP_SDR, 0.89 ),
		float[2]( MAX_STOP_RRT, 0.90 ));
	float pctHigh = interpolate1D(bendsHigh, log2(TsPointMax.x / 0.18));
	coefsHigh[2] = log10(TsPointMid.y) + pctHigh * (log10(TsPointMax.y) - log10(TsPointMid.y));

	return coefsHigh;
}

float shift(float x, float expShift)
{
	return pow(2.0, (log2(x) - expShift));
}

TsParams init_TsParams(float minLum, float maxLum, float expShift)
{
	TsPoint MIN_PT = TsPoint( lookup_ACESmin(minLum), minLum, 0.0 );
	TsPoint MID_PT = TsPoint( 0.18, 4.8, 1.55 );
	TsPoint MAX_PT = TsPoint( lookup_ACESmax(maxLum), maxLum, 0.0 );
	float cLow[5] = init_coefsLow(MIN_PT, MID_PT);
	float cHigh[5] = init_coefsHigh(MID_PT, MAX_PT);
	MIN_PT.x = shift(lookup_ACESmin(minLum), expShift);
	MID_PT.x = shift(0.18, expShift);
	MAX_PT.x = shift(lookup_ACESmax(maxLum), expShift);

	TsParams P = TsParams(
		TsPoint( MIN_PT.x, MIN_PT.y, MIN_PT.slope ),
		TsPoint( MID_PT.x, MID_PT.y, MID_PT.slope ),
		TsPoint( MAX_PT.x, MAX_PT.y, MAX_PT.slope ),
		float[6]( cLow[0], cLow[1], cLow[2], cLow[3], cLow[4], cLow[4] ),
		float[6]( cHigh[0], cHigh[1], cHigh[2], cHigh[3], cHigh[4], cHigh[4] )
	);

	return P;
}

float ssts(float x, TsParams C)
{
	const int N_KNOTS_LOW = 4;
	const int N_KNOTS_HIGH = 4;

	// Check for negatives or zero before taking the log. If negative or zero,
	// set to HALF_MIN.
	float logx = log10(max(x, 6.10352e-5));

	float logy;
	if (logx <= log10(C.Min.x))
	{
		logy = logx * C.Min.slope + (log10(C.Min.y) - C.Min.slope * log10(C.Min.x));
	}
	else if ((logx > log10(C.Min.x)) && (logx < log10(C.Mid.x)))
	{
		float knot_coord = float(N_KNOTS_LOW - 1) * (logx - log10(C.Min.x)) / (log10(C.Mid.x) - log10(C.Min.x));
		int j = int(knot_coord);
		float t = knot_coord - float(j);

		vec3 cf = vec3(C.coefsLow[j], C.coefsLow[j + 1], C.coefsLow[j + 2]);

		vec3 monomials = vec3(t * t, t, 1.0);
		logy = dot(monomials, M1 * cf);
	}
	else if ((logx >= log10(C.Mid.x)) && (logx < log10(C.Max.x)))
	{
		float knot_coord = float(N_KNOTS_HIGH - 1) * (logx - log10(C.Mid.x)) / (log10(C.Max.x) - log10(C.Mid.x));
		int j = int(knot_coord);
		float t = knot_coord - float(j);

		vec3 cf = vec3(C.coefsHigh[j], C.coefsHigh[j + 1], C.coefsHigh[j + 2]);

		vec3 monomials = vec3(t * t, t, 1.0);
		logy = dot(monomials, M1 * cf);
	}
	else
	{
		logy = logx * C.Max.slope + (log10(C.Max.y) - C.Max.slope * log10(C.Max.x));
	}
	return pow(10.0, logy);
}

float inv_ssts(float y, TsParams C)
{
	const int N_KNOTS_LOW = 4;
	const int N_KNOTS_HIGH = 4;

	float KNOT_INC_LOW = (log10(C.Mid.x) - log10(C.Min.x)) / float(N_KNOTS_LOW - 1);
	float KNOT_INC_HIGH = (log10(C.Max.x) - log10(C.Mid.x)) / float(N_KNOTS_HIGH - 1);

	// KNOT_Y is luminance of the spline at each knot
	float KNOT_Y_LOW[N_KNOTS_LOW];
	for (int i = 0; i < N_KNOTS_LOW; ++i)
	{
		KNOT_Y_LOW[i] = (C.coefsLow[i] + C.coefsLow[i + 1]) * 0.5;
	};

	float KNOT_Y_HIGH[N_KNOTS_HIGH];
	for (int i = 0; i < N_KNOTS_HIGH; ++i)
	{
		KNOT_Y_HIGH[i] = (C.coefsHigh[i] + C.coefsHigh[i + 1]) * 0.5;
	};

	float logy = log10(max(y, 1e-10));

	float logx;
	if (logy <= log10(C.Min.y))
	{
		logx = log10(C.Min.x);
	}
	else if ((logy > log10(C.Min.y)) && (logy <= log10(C.Mid.y)))
	{
		float j;
		vec3 cf;
		if (logy > KNOT_Y_LOW[0] && logy <= KNOT_Y_LOW[1])
		{
			cf[0] = C.coefsLow[0];  cf[1] = C.coefsLow[1];  cf[2] = C.coefsLow[2];  j = 0.0;
		}
		else if (logy > KNOT_Y_LOW[1] && logy <= KNOT_Y_LOW[2])
		{
			cf[0] = C.coefsLow[1];  cf[1] = C.coefsLow[2];  cf[2] = C.coefsLow[3];  j = 1.0;
		}
		else // if (logy > KNOT_Y_LOW[2] && logy <= KNOT_Y_LOW[3])
		{
			cf[0] = C.coefsLow[2];  cf[1] = C.coefsLow[3];  cf[2] = C.coefsLow[4];  j = 2.0;
		}

		vec3 tmp = M1 * cf;

		float a = tmp[0];
		float b = tmp[1];
		float c = tmp[2];
		c = c - logy;

		float d = sqrt(b * b - 4.0 * a * c);
		float t = (2.0 * c) / (-d - b);
		logx = log10(C.Min.x) + (t + j) * KNOT_INC_LOW;
	}
	else if ((logy > log10(C.Mid.y)) && (logy < log10(C.Max.y)))
	{
		float j;
		vec3 cf;
		if (logy >= KNOT_Y_HIGH[0] && logy <= KNOT_Y_HIGH[1])
		{
			cf[0] = C.coefsHigh[0];  cf[1] = C.coefsHigh[1];  cf[2] = C.coefsHigh[2];  j = 0.0;
		}
		else if (logy > KNOT_Y_HIGH[1] && logy <= KNOT_Y_HIGH[2])
		{
			cf[0] = C.coefsHigh[1];  cf[1] = C.coefsHigh[2];  cf[2] = C.coefsHigh[3];  j = 1.0;
		}
		else // if (logy > KNOT_Y_HIGH[2] && logy <= KNOT_Y_HIGH[3])
		{
			cf[0] = C.coefsHigh[2];  cf[1] = C.coefsHigh[3];  cf[2] = C.coefsHigh[4];  j = 2.0;
		}

		vec3 tmp = M1 * cf;

		float a = tmp[0];
		float b = tmp[1];
		float c = tmp[2];
		c = c - logy;

		float d = sqrt(b * b - 4.0 * a * c);
		float t = (2.0 * c) / (-d - b);
		logx = log10(C.Mid.x) + (t + j) * KNOT_INC_HIGH;
	}
	else
	{
		logx = log10(C.Max.x);
	}
	return pow(10.0, logx);
}

// https://github.com/ampas/aces-dev/blob/v1.2/transforms/ctl/lib/ACESlib.OutputTransforms.ctl

vec3 outputTransform(vec3 aces, float Y_MIN, float Y_MID, float Y_MAX, mat3 AP1_2_DISPLAY_MAT, int EOTF, int SURROUND)
{
	TsParams PARAMS_DEFAULT = init_TsParams(Y_MIN, Y_MAX, 0.0);
	float expShift = log2(inv_ssts(Y_MID, PARAMS_DEFAULT)) - log2(0.18);
	TsParams PARAMS = init_TsParams(Y_MIN, Y_MAX, expShift);

	// RRT sweeteners
	vec3 rgbPre = rrt_sweeteners(aces);

	// Apply the tonescale independently in rendering-space RGB
	vec3 rgbPost;
	rgbPost.r = ssts(rgbPre.r, PARAMS);
	rgbPost.g = ssts(rgbPre.g, PARAMS);
	rgbPost.b = ssts(rgbPre.b, PARAMS);

	// At this point data encoded AP1, scaled absolute luminance (cd/m^2)

	/*  Scale absolute luminance to linear code value  */
	vec3 linearCV = Y_2_linCV(rgbPost, Y_MAX, Y_MIN);

	if ((SURROUND == 1) && (EOTF == 1 || EOTF == 2 || EOTF == 3))
	{
		// Dim surround
		linearCV = darkSurround_to_dimSurround(linearCV);
	}

	// AP1 to display encoding primaries
	linearCV = max(linearCV * AP1_2_DISPLAY_MAT, 0.0);

	vec3 outputCV;
	if (EOTF == 0) // ST-2084 (PQ)
	{
		outputCV = Y_2_ST2084(linCV_2_Y(linearCV, Y_MAX, Y_MIN));
	}
	else if (EOTF == 1) // BT.1886 (Rec.709/2020 settings)
	{
		outputCV = bt1886_r(linearCV, 2.4, 1.0, 0.0);
	}
	else if (EOTF == 2) // sRGB (mon_curve w/ presets)
	{
		outputCV = moncurve_r(linearCV, 2.4, 0.055);
	}
	else if (EOTF == 3) // gamma 2.6
	{
		outputCV = pow(linearCV, vec3(1.0 / 2.6));
	}
	else if (EOTF == 4) // linear
	{
		outputCV = linCV_2_Y(linearCV, Y_MAX, Y_MIN);
	}
	return outputCV;
}
