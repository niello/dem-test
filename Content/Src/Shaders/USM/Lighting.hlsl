
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

#define Attenuation			AttenuationQuad
#define SpotlightFalloff	SpotlightFalloffLinear

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
	float Factor = Distance * InvRange;
	return saturate(1 - Factor * Factor);
	// return 1 / (Distance * Distance); - not normalized
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

// BRDF

float DiffuseLambert(float3 N, float3 L)
{
	return max(dot(N, L), 0.0f);
}   
//---------------------------------------------------------------------

