#include "Lighting.hlsl"

struct CInstanceDataVS
{
	matrix	WorldMatrix;
};

struct CInstanceDataPS
{
	uint	LightCount;
	uint3	LightIndices;
	//static uint LightIndices[DEM_MAX_LIGHTS] = (uint[DEM_MAX_LIGHTS])array; // for tight packing
};

// Per-material data

Texture2D TexAlbedo: register(t0);		// PS
Texture2D TexNormalMap: register(t1);	// PS
Texture2D TexReflectance: register(t2);	// PS
Texture2D TexRoughness: register(t3);	// PS
sampler LinearSampler: register(s0);	// PS

// Pixel shaders

float4 PSMain(PSInSimple In): SV_Target
{
	// Sample normal map and calculate per-pixel normal
	// We invert Y to load normal maps with +Y (OpenGL-style)
	// NB: In.View must be not normalized
	float4 NM = TexNormalMap.Sample(LinearSampler, In.UV);	
	float3 SampledNormal = NM.xyz * float3(2.f, -2.f, 2.f) - float3(1.f, -1.f, 1.f); // May use (255.f / 127.f) - (128.f / 127.f));
	float3 N = PerturbNormal(SampledNormal, normalize(In.Normal.xyz), In.View, In.UV);

	// Sample albedo
	float4 Albedo = TexAlbedo.Sample(LinearSampler, In.UV);
	float3 AlbedoRGB = Albedo.rgb;

	// Sample reflectivity (common one-channel value 0.02-0.04 for all insulators)
	float3 Reflectivity = TexReflectance.Sample(LinearSampler, In.UV).rgb;
	//float3 Reflectivity = 0.04f.xxx; // Dielectric, in metalness workflow can use uniform float for different dielectrics like gems
	//float3 Reflectivity = float3(1.0f, 0.71f, 0.29f); // Gold
	//float3 Reflectivity = float3(0.95f, 0.64f, 0.54f); // Copper

	// Sample roughness
	float Roughness = TexRoughness.Sample(LinearSampler, In.UV).r;
	float SqRoughness = Roughness * Roughness;

	float3 V = normalize(In.View);
	float NdotV = max(0.f, dot(N, V));

	float3 DirectLighting = float3(0.f, 0.f, 0.f);
	for (uint i = 0; i < In.LightInfo[0]; ++i)
	{
		CLight CurrLight = Lights[In.LightInfo[i + 1]];

		float3 LightIntensity;
		float3 L;
		GetLightSourceParams(CurrLight, In.PosWorld, LightIntensity, L);

		float3 H = normalize(L + V);
		float NdotL = max(0.f, dot(N, L));
		float NdotH = max(0.f, dot(N, H));
		float VdotH = max(0.f, dot(V, H));

		// It is essentially a Lambertian BRDF for diffuse term
		// Read this to learn why we don't divide diffuse color by PI:
		// https://seblagarde.wordpress.com/2012/01/08/pi-or-not-to-pi-in-game-lighting-equation/
		float3 DiffuseColor = AlbedoRGB;

		// Turn Lambert to Oren-Nayar for not perfectly smooth surfaces
		if (SqRoughness > 0.f) DiffuseColor *= DiffuseOrenNayar(N, L, V, NdotL, NdotV, SqRoughness);

		float3 SpecularColor = SpecularCookTorrance(Reflectivity, SqRoughness, NdotV, NdotL, NdotH, VdotH);

		DirectLighting += LightIntensity * NdotL * (DiffuseColor * (1.f - SpecularColor) + SpecularColor);
	}

	// Sample ambient diffuse irradiance from irradiance cubemap 
	float3 AmbientDiffuse = AlbedoRGB * TexIrradianceMap.Sample(TrilinearCubeSampler, N).rgb;

	// Sample ambient specular radiance from convoluted environment map
	//???select min of calculated and HW mip!?
	float3 R = 2.f * N * NdotV - V;
	float MipIndex = SqRoughness * (RADIANCE_ENVMAP_MIP_COUNT - 1.f);
	float3 EnvColor = TexRadianceEnvMap.SampleLevel(TrilinearCubeSampler, R, MipIndex).rgb;
	float3 AmbientSpecular = EnvColor * FresnelSchlickWithRoughness(Reflectivity, SqRoughness, NdotV);

	// Another variant of direct specular (Blinn-Phong normal distribution & implicit geometry term):
	// Indirect specular setting depends on what direct specular we use, to match it (in modified cubemapgen)
	//SpecularPower = exp2(10 * (1.f - Roughness) + 1); // 10 and 1 are GlossScale and GlossOffset
	//or SpecularPower = exp(SpecularPowerMaximum, (1.f - Roughness)), then indirect specular mip level is linear func of roughness
	//FresnelSchlick(Reflectivity, VdotH) * ((SpecularPower + 2) / 8 ) * pow(NdotH, SpecularPower) * NdotL;

	//???how to calc correct alpha?
	//Unity: Adding more reflectivity (as energy must be taken from somewhere) the diffuse level and the transparency will be reduced
	//automatically. Adding transparency will reduce diffuse level.
	// No translucent metals! Can use alpha-test, but not blend!
	return float4(DirectLighting + AmbientDiffuse + AmbientSpecular, Albedo.a);
}
//---------------------------------------------------------------------
