#include "Globals.hlsl"
#include "CDLOD.hlsl"
#include "Splatting.hlsl"
#include "Lighting.hlsl"

struct PSInSimple
{
	float4	Pos:		SV_Position;
	float3	PosWorld:	TEXCOORD1;
	float3	Normal:		NORMAL;
	float3	View:		VIEW;
	float2	UV:			TEXCOORD;
	uint4	LightInfo:	LIGHTINFO;
};

struct PSInSplatted
{
	float4 Pos:				SV_Position;
	float4 VertexConsts:	TEXCOORD0;
	float4 SplatDetUV:		TEXCOORD1;
	float4 PosWorld:		TEXCOORD2;
};

struct CInstanceData
{
	matrix	WorldMatrix;
	uint	LightCount;
	uint3	LightIndices;
	//static uint LightIndices[DEM_MAX_LIGHTS] = (uint[DEM_MAX_LIGHTS])array; // for tight packing
};

// Per-material data

Texture2D TexAlbedo: register(t0);
Texture2D TexNormalMap: register(t1);
sampler LinearSampler: register(s0);

// Per-instance data

cbuffer InstanceParams: register(b2)
{
	CInstanceData InstanceData;
}

#ifndef MAX_INSTANCE_COUNT
#define MAX_INSTANCE_COUNT 64
#endif

cbuffer InstanceParams: register(b2)
{
	CInstanceData InstanceDataArray[MAX_INSTANCE_COUNT];
}

// Vertex shaders

PSInSimple StandardVS(float3 Pos, float3 Normal, float2 UV, matrix World, uint LightCount, uint3 LightIndices)
{
	PSInSimple Out = (PSInSimple)0.0;
	float4 WorldPos = mul(float4(Pos, 1), World);
	Out.Pos = mul(WorldPos, ViewProj);
	Out.PosWorld = WorldPos.xyz;
	Out.Normal = mul(Normal, (float3x3)World);
	Out.View = EyePos - WorldPos.xyz;
	Out.UV = UV;
	Out.LightInfo.x = LightCount;
	Out.LightInfo.yzw = LightIndices;
	return Out;
}
//---------------------------------------------------------------------

PSInSimple VSMain(float3 Pos: POSITION, float3 Normal: NORMAL, float2 UV: TEXCOORD)
{
	return StandardVS(Pos, Normal, UV, InstanceData.WorldMatrix, InstanceData.LightCount, InstanceData.LightIndices);
}
//---------------------------------------------------------------------

PSInSimple VSMainInstanced(	float3 Pos: POSITION,
							float3 Normal: NORMAL,
							float2 UV: TEXCOORD,
							float4 World1: TEXCOORD4,
							float4 World2: TEXCOORD5,
							float4 World3: TEXCOORD6,
							float4 World4: TEXCOORD7)
{
	float4x4 InstWorld = float4x4(World1, World2, World3, World4);
	return StandardVS(Pos, Normal, UV, InstWorld, 0, uint3(0, 0, 0));
}
//---------------------------------------------------------------------

PSInSimple VSMainInstancedConst(float3 Pos: POSITION, float3 Normal: NORMAL, float2 UV: TEXCOORD, uint InstanceID: SV_InstanceID)
{
	CInstanceData InstData = InstanceDataArray[InstanceID];
	return StandardVS(Pos, Normal, UV, InstData.WorldMatrix, InstData.LightCount, InstData.LightIndices);
}
//---------------------------------------------------------------------

PSInSplatted VSMainCDLOD(	float2	Pos:			POSITION,
							float4	PatchXZ:		TEXCOORD0,	// xy - scale, zw - offset
							float2	MorphConsts:	TEXCOORD1)	// x - end / (end - start), y - 1 / (end - start)
{
	float3 Vertex;
	Vertex.xz = Pos * PatchXZ.xy + PatchXZ.zw;
	float2 HMapUV = Vertex.xz * VSCDLODParams.WorldToHM.xy + VSCDLODParams.WorldToHM.zw;
	Vertex.y = SampleHeightMap(HMapUV) * VSCDLODParams.TerrainYInvSplat.x + VSCDLODParams.TerrainYInvSplat.y;

	float MorphK  = 1.0f - clamp(MorphConsts.x - distance(Vertex, EyePos) * MorphConsts.y, 0.0f, 1.0f);
	float2 FracPart = frac(Pos * GridConsts.xx) * GridConsts.yy;
	const float2 PosMorphed = Pos - FracPart * MorphK;

	Vertex.xz = PosMorphed * PatchXZ.xy + PatchXZ.zw;
	HMapUV = Vertex.xz * VSCDLODParams.WorldToHM.xy + VSCDLODParams.WorldToHM.zw;
	Vertex.y = SampleHeightMap(HMapUV) * VSCDLODParams.TerrainYInvSplat.x + VSCDLODParams.TerrainYInvSplat.y;

	float DetailMorphK = 0.0;
	float2 DetailUV = float2(0.0, 0.0);
	/* Detail map
	if (g_useDetailMap)
	{
		DetailUV = PosMorphed * GridToDM.xy + GridToDM.zw;
									//LODLevel			// detailLODLevelsAffected
		DetailMorphK = 1 - saturate(g_quadScale.z + 2.0 - DetailConsts.w) * MorphK;
		Vertex.z += DetailMorphK * (SampleHeightMap(VSHeightDetailSampler, DetailUV.xy) - 0.5) * DetailConsts.z;
	}
	*/

	PSInSplatted Out;
	Out.PosWorld = float4(Vertex, 1.0);
	Out.Pos = mul(Out.PosWorld, ViewProj);
	Out.VertexConsts = float4(MorphK, DetailMorphK, Out.Pos.w, distance(Vertex, EyePos));
	Out.SplatDetUV = float4(Vertex.xz * VSCDLODParams.TerrainYInvSplat.zw, DetailUV);
	return Out;
}
//---------------------------------------------------------------------

// Pixel shaders

float4 PSMain(PSInSimple In): SV_Target
{
	// Sample normal map and calculate per-pixel normal
	// NB: In.View must be not normalized
	float3 InNormal = normalize(In.Normal.xyz);
	float4 NM = TexNormalMap.Sample(LinearSampler, In.UV);
	float3 SampledNormal = normalize(NM.xyz * 2.f - 1.f); // (255.f / 127.f) - (128.f / 127.f));
	SampledNormal.y = -SampledNormal.y; //!!!can save normal maps properly to avoid this!
	float3 N = PerturbNormal(SampledNormal, InNormal, In.View, In.UV);

	// Sample albedo
	float4 Albedo = TexAlbedo.Sample(LinearSampler, In.UV);
	float3 AlbedoRGB = Albedo.rgb;
	//float3 AlbedoRGB = 0.f.xxx;

	// Sample reflectivity (common one-channel value 0.02-0.04 for all insulators)
	float3 Reflectivity = 0.04f.xxx; // Can use uniform material parameter
	//float3 Reflectivity = float3(0.57f, 0.38f, 0.f);

	// Sample roughness
	float Roughness = 0.35f;
	float SqRoughness = Roughness * Roughness;

	float3 V = normalize(In.View);
	float NdotV_Unsaturated = dot(N, V);
	float NdotV = max(0.f, NdotV_Unsaturated);

	float3 LightingResult = float3(0.f, 0.f, 0.f);
	for (uint i = 0; i < In.LightInfo[0]; ++i)
	{
		CLight CurrLight = Lights[In.LightInfo[i + 1]];

		float3 LightIntensity;
		float3 L;
		GetLightSourceParams(CurrLight, In.PosWorld, LightIntensity, L);

		float3 H = normalize(L + V);
		float NdotL_Unsaturated = dot(N, L);
		float NdotL = max(0.f, NdotL_Unsaturated);
		float NdotH = max(0.f, dot(N, H));
		float VdotH = max(0.f, dot(V, H));

		float3 DiffuseColor = AlbedoRGB;
		if (SqRoughness > 0.f) DiffuseColor *= DiffuseOrenNayar(N, L, V, NdotL_Unsaturated, NdotV_Unsaturated, SqRoughness);

		float3 SpecularColor = SpecularCookTorrance(Reflectivity, SqRoughness, NdotV, NdotL, NdotH, VdotH);

		// Read this to learn why we don't divide diffuse color by PI:
		// https://seblagarde.wordpress.com/2012/01/08/pi-or-not-to-pi-in-game-lighting-equation/
		LightingResult += LightIntensity * NdotL * (DiffuseColor * (1.f - SpecularColor) + SpecularColor);
		
	}

	// Calculate or sample from cubemap ambient irradiance
	float3 AmbientIrradiance = 0.25f.xxx;

	// Do image-based lighting from environment map, mipmapped to resemble different roughness
	//float3 R = 2.f * N * NdotV - V; // reflect(-V, N)
	//float MipIndex = SqRoughness * 8.0f; // 8 is mip level count - 1
	float3 EnvColor = 0.4f.xxx; //TexEnvMap.SampleLevel(LinearSampler, R, MipIndex);
	float3 EnvFresnel = FresnelSchlickWithRoughness(Reflectivity, SqRoughness, NdotV);

	return float4(LightingResult + AlbedoRGB * AmbientIrradiance + EnvColor * EnvFresnel, Albedo.a);
}
//---------------------------------------------------------------------

//!!!see old code version for more code!
float4 PSMainSplatted(PSInSplatted In): SV_Target
{
	float2 UV = In.PosWorld.xz * PSCDLODParams.WorldToHM.xy + PSCDLODParams.WorldToHM.zw;
	float3 TexDiffuse = Splatting(SplatMap.Sample(SplatSampler, UV), In.SplatDetUV.xy, LinearSampler);
	return float4(TexDiffuse, 1.f);
}
//---------------------------------------------------------------------
