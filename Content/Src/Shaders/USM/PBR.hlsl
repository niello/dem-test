#include "Lighting.hlsl"

#ifndef DEM_PBR_LIGHT_INDEX_COUNT
#define DEM_PBR_LIGHT_INDEX_COUNT MAX_LIGHT_COUNT_PER_OBJECT
#endif

#if DEM_PBR_LIGHT_INDEX_COUNT <= 0
#define DEM_PBR_LIGHT_INDEX_COUNT 1
#endif

struct CInstanceDataVS
{
	matrix	WorldMatrix;
};

struct CInstanceDataPS
{
	int		LightCount;
	int3	_CInstanceDataPS_PAD;
#if DEM_LIGHT_VECTOR_COUNT > 0
	int4	LightIndices[DEM_LIGHT_VECTOR_COUNT];
#endif
};

// Per-material data

Texture2D TexAlbedo: register(t0);		// PS
Texture2D TexNormalMap: register(t1);	// PS
Texture2D TexReflectance: register(t2);	// PS
Texture2D TexRoughness: register(t3);	// PS
sampler LinearSampler: register(s0);	// PS

// Pixel shaders

// Another variant of direct specular (Blinn-Phong normal distribution & implicit geometry term):
// Indirect specular setting depends on what direct specular we use, to match it (in modified cubemapgen)
//SpecularPower = exp2(10 * (1.f - Roughness) + 1); // 10 and 1 are GlossScale and GlossOffset
//or SpecularPower = exp(SpecularPowerMaximum, (1.f - Roughness)), then indirect specular mip level is linear func of roughness
//FresnelSchlick(Reflectivity, VdotH) * ((SpecularPower + 2) / 8 ) * pow(NdotH, SpecularPower) * NdotL;

float3 PSAmbientPBR(float4 Albedo, float3 N, float3 V, float NdotV, float3 Reflectivity, float SqRoughness)
{
	// Sample ambient diffuse irradiance from irradiance cubemap 
	float3 AmbientDiffuse = Albedo.rgb * TexIrradianceMap.Sample(TrilinearCubeSampler, N).rgb;

	// Sample ambient specular radiance from convoluted environment map
	//???select min of calculated and HW mip!?
	float3 R = 2.f * N * NdotV - V;
	float MipIndex = SqRoughness * (RADIANCE_ENVMAP_MIP_COUNT - 1.f);
	float3 EnvColor = TexRadianceEnvMap.SampleLevel(TrilinearCubeSampler, R, MipIndex).rgb;
	float3 AmbientSpecular = EnvColor * FresnelSchlickWithRoughness(Reflectivity, SqRoughness, NdotV);

	return AmbientDiffuse + AmbientSpecular;
}
//---------------------------------------------------------------------

float3 PSDirectPBR(float4 Albedo, float3 N, float3 V, float NdotV, float3 Reflectivity, float SqRoughness, float3 PosWorld, CLight Light)
{
	float3 LightIntensity;
	float3 L;
	GetLightSourceParams(Light, PosWorld, LightIntensity, L);

	float3 H = normalize(L + V);
	float NdotL = max(0.f, dot(N, L));
	float NdotH = max(0.f, dot(N, H));
	float VdotH = max(0.f, dot(V, H));

	// It is essentially a Lambertian BRDF for diffuse term
	// Read this to learn why we don't divide diffuse color by PI:
	// https://seblagarde.wordpress.com/2012/01/08/pi-or-not-to-pi-in-game-lighting-equation/
	float3 DiffuseColor = Albedo.rgb;

	// Turn Lambert to Oren-Nayar for not perfectly smooth surfaces
	if (SqRoughness > 0.f) DiffuseColor *= DiffuseOrenNayar(N, L, V, NdotL, NdotV, SqRoughness);

	float3 SpecularColor = SpecularCookTorrance(Reflectivity, SqRoughness, NdotV, NdotL, NdotH, VdotH);

	return LightIntensity * NdotL * (DiffuseColor * (1.f - SpecularColor) + SpecularColor);
}
//---------------------------------------------------------------------
