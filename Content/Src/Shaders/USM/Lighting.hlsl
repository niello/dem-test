
// Lighting and related functions

// Naming conventions for this file:
// N - surface normal vector
// L - surface point to the ligth direction
// V - surface point to the viewer direction
// R - reflection vector that is vector opposite to light (L) against surface normal (N)
//     normalize(2.0f * dot(N, L) * N - L) or normalize(-reflect(L, N))
// H - half-vector between viewer (V) and light (L) vectors
//     normalize(L + V)
// CT - Cook-Torrance model
// ON - Oren-Nayar model

#define LIGHT_TYPE_DIR		0
#define LIGHT_TYPE_POINT	1
#define LIGHT_TYPE_SPOT		2

#define AttenuationFunction			AttenuationQuad
#define SpotlightFalloffFunction	SpotlightFalloffLinear

#ifndef MAX_LIGHT_COUNT
#define MAX_LIGHT_COUNT 256
#endif

struct CLight
{
	float3	Color;			// Intensity of r, g and b radiance
	float3	Position;
	float	SqInvRange;
	float4	Params;			// Spot: x - cos inner, y - cos outer
	float3	InvDirection;
	uint	Type;
};

cbuffer LightBuffer: register(b3)
{
	CLight Lights[MAX_LIGHT_COUNT];
}

// Normal mapping

// http://www.thetenthplanet.de/archives/1180
float3x3 CotangentFrame(float3 N, float3 Pos, float2 UV)
{
	// get edge vectors of the pixel triangle
	float3 dp1 = ddx(Pos);
	float3 dp2 = ddy(Pos);
	float2 duv1 = ddx(UV);
	float2 duv2 = ddy(UV);

	// solve the linear system
	float3 dp2perp = cross(dp2, N);
	float3 dp1perp = cross(N, dp1);
	float3 T = dp2perp * duv1.x + dp1perp * duv2.x;
	float3 B = dp2perp * duv1.y + dp1perp * duv2.y;

	// construct a scale-invariant frame 
	float invmax = rsqrt(max(dot(T, T), dot(B, B)));
	return float3x3(T * invmax, B * invmax, N);
}

// http://www.thetenthplanet.de/archives/1180
// N - interpolated vertex normal, V - interpolated vertex to eye vector
float3 PerturbNormal(float3 N, float3 V, float2 UV)
{
//!!!TMP!
	float3 SampledNormal = float3(0, 0, 1);

    //float3 SampledNormal = tex2D(TexNormalMap, UV).xyz;
//#ifdef WITH_NORMALMAP_UNSIGNED
    //SampledNormal = SampledNormal * 255./127. - 128./127.;
//#endif
//#ifdef WITH_NORMALMAP_2CHANNEL
//    SampledNormal.z = sqrt(1.f - dot(SampledNormal.xy, SampledNormal.xy));
//#endif
//#ifdef WITH_NORMALMAP_GREEN_UP
//    SampledNormal.y = -SampledNormal.y;
//#endif

//???use V or -V? EyePos - Pos or inverse? text states Vertex subtracted by Camera (Pos - EyePos)

    float3x3 TBN = CotangentFrame(N, V, UV);
    return normalize(mul(SampledNormal, TBN));
}

// Light attenuation and falloff

float AttenuationLinear(float Distance, float InvRange)
{
	return saturate(1 - Distance * InvRange);
}
//---------------------------------------------------------------------

// The most realistic
float AttenuationQuad(float Distance, float InvRange)
{
	//return saturate(1.f - SqDistance * InvRange * InvRange);

	float Factor = Distance * InvRange;
	return saturate(1.f - Factor * Factor);

	// return 1.f / SqDistance; - not normalized
}
//---------------------------------------------------------------------

float AttenuationExp(float Distance, float InvRange, float Exp)
{
	return pow(saturate(1 - Distance * InvRange), Exp);
}
//---------------------------------------------------------------------

// CosAlpha = dot(LightDir, SurfacePos - LightPos)

// Simple case with a falloff exponent = 1
float SpotlightFalloffLinear(float CosAlpha, float CosHalfTheta, float CosHalfPhi)
{
//!!!can precompute InvDenominator on CPU!
	return saturate((CosAlpha - CosHalfPhi) / (CosHalfTheta - CosHalfPhi));
}
//---------------------------------------------------------------------

float SpotlightFalloffHermite(float CosAlpha, float CosHalfTheta, float CosHalfPhi)
{
	// -2 * x^3 + 3 * x^2 (from shader asm code)
	return smoothstep(CosHalfPhi, CosHalfTheta, CosAlpha);
}
//---------------------------------------------------------------------

float SpotlightFalloffExp(float CosAlpha, float CosHalfTheta, float CosHalfPhi, float Exp)
{
	return pow(SpotlightFalloffLinear(CosAlpha, CosHalfTheta, CosHalfPhi), Exp);
}
//---------------------------------------------------------------------

// Light source handling

void GetLightSourceParams(CLight LightSource, float3 WorldPosition, out float3 Intensity, out float3 L)
{
	if (LightSource.Type == LIGHT_TYPE_DIR)
	{
		L = LightSource.InvDirection;
		Intensity = LightSource.Color;
	}
	else
	{
		L = LightSource.Position - WorldPosition;
		float SqDistanceToLight = dot(L, L);
		L *= rsqrt(SqDistanceToLight);

		// Calculate attenuation and falloff
		//???what attenuation formula to use?
		Intensity = LightSource.Color * saturate(1.f - SqDistanceToLight * LightSource.SqInvRange);
		if (LightSource.Type == LIGHT_TYPE_SPOT)
			Intensity *= SpotlightFalloffFunction(dot(LightSource.InvDirection, L), LightSource.Params.x, LightSource.Params.y);
	}
}
//---------------------------------------------------------------------

// BRDF

// An useful table with roughness values: https://wiki.blender.org/index.php/User:Guiseppe/Oren_Nayar
//NB: doesn't mul NdotL
float DiffuseOrenNayar(float3 N, float3 L, float3 V, float NdotL, float NdotV, float SqRoughness)
{
	// A = 1, B = 0 - simplifies to lambertian model
	//if (SqRoughness == 0) return 1.f;

	float A = 1.0f - 0.5f * (SqRoughness / (SqRoughness + 0.57f));
	float B = 0.45f * (SqRoughness / (SqRoughness + 0.09f));

	float Gamma = dot(V - N * NdotV, L - N * NdotL);

	// Can use lookup texture:
	// float C = tex2D(LookupMap, float2(NdotV, NdotL) * 0.5f + 0.5f).x; // map -1..1 to 0..1 if not saturated
	float2 Angles = acos(float2(NdotV, NdotL)); //???does "float2(NdotV, NdotL)" cost something? mb initially store in float2?
	float C = sin(max(Angles.x, Angles.y)) * tan(min(Angles.x, Angles.y));

	//!!!result = light * (albedo / pi) * result!

	return (A + B * max(Gamma, 0.f) * C);
}
//---------------------------------------------------------------------

// Dot products must be saturated
float GeometricSmithGGX(float SqRoughness, float NdotV, float NdotL)
{
	float SqRoughness2 = SqRoughness * SqRoughness;
	float OneSubSqRoughness2 = 1.0f - SqRoughness2;
	float2 Dots = float2(NdotV, NdotL);
	float2 G = (2.0f * Dots) / (Dots + sqrt(SqRoughness2 + OneSubSqRoughness2 * Dots * Dots));
	return G.x * G.y;
}
//---------------------------------------------------------------------

// Dot products must be saturated
float GeometricSmithSchlickGGX(float SqRoughness, float NdotV, float NdotL)
{
	float K = SqRoughness * 0.5f;
	float OneSubK = 1.0f - K;
	float2 Dots = float2(NdotV, NdotL);
	float2 G = Dots / (Dots * OneSubK + K);
	return G.x * G.y;
}
//---------------------------------------------------------------------

// For conductors
// Dot products must be saturated
float3 FresnelSchlick(float3 Reflectivity, float VdotH)
{
	return Reflectivity + (1.0f - Reflectivity) * pow((1.0f - VdotH), 5);
}
//---------------------------------------------------------------------

// For dielectrics
// Dot products must be saturated
float FresnelSchlickSingleChannel(float Reflectivity, float VdotH)
{
	return Reflectivity + (1.0f - Reflectivity) * pow((1.0f - VdotH), 5);
}
//---------------------------------------------------------------------

//???is correct? produces strange results in PBRViewer!
// Dot products must be saturated
float3 FresnelCookTorrance(float3 Reflectivity, float VdotH)
{
	float3 SqrtReflectivity = sqrt(Reflectivity);
	float3 n = (1.0f + SqrtReflectivity) / (1.0f - SqrtReflectivity);
	float c = VdotH;
	float3 g = sqrt(n * n + c * c - 1.0f);

	float3 GaddC = g + c;
	float3 GsubC = g - c;

	float3 part1 = GsubC / GaddC;
	float3 part2 = (GaddC * c - 1.0f) / (GsubC * c + 1.0f);

	return max(0.0f.xxx, 0.5f * part1 * part1 * ( 1 + part2 * part2));
}
//---------------------------------------------------------------------

// Dot products must be saturated
float NormalDistributionGGX(float SqRoughness, float NdotH)
{
	static const float PI = 3.14159265359f;

	float SqRoughness2 = SqRoughness * SqRoughness;
	float Denominator = NdotH * NdotH * (SqRoughness2 - 1.0f) + 1.0f;
	Denominator *= Denominator;
	Denominator *= PI;

	return SqRoughness2 / Denominator;
}
//---------------------------------------------------------------------

// Dot products must be saturated
float3 SpecularCookTorrance(float3 Reflectivity, float SqRoughness, float NdotV, float NdotL, float NdotH, float VdotH)
{
	float D = NormalDistributionGGX(SqRoughness, NdotH);
	float G = GeometricSmithSchlickGGX(SqRoughness, NdotV, NdotL);
	float3 F = FresnelSchlick(Reflectivity, VdotH);
	return (D * G * F) / (4.0f * NdotL * NdotV);
	//return (D * G * F) / (3.9999f * NdotL * NdotV + 0.0001f);
}
//---------------------------------------------------------------------
